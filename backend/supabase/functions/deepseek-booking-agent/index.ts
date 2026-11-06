import { createClient } from "https://esm.sh/@supabase/supabase-js@2";
import { bookingContext, resolveBookingPlanInput } from "./booking-request.mjs";
import { conversationHistory } from "./conversation-history.mjs";
import { bookingTools, runBookingAgent } from "./booking-agent-tools.mjs";
import { adminTools, prepareAdminAction } from "./admin-agent-actions.mjs";

type AgentType = "booking_planner" | "admin_analyst" | "fault_responder";
type AgentRequest = {
  requestId?: string;
  sessionId?: string;
  agentType?: AgentType;
  // V2 compatibility only. New callers must send agentType.
  mode?: "booking" | "admin";
  message?: string;
  language?: "zh" | "en";
  input?: Record<string, unknown>;
  history?: unknown;
  confirmation?: { proposalId?: string; idempotencyKey?: string };
};
type AgentResponse = {
  requestId: string;
  sessionId: string | null;
  agentType: AgentType;
  outcome: "suggested" | "confirmed" | "fallback" | "failed";
  facts: Record<string, unknown>;
  message: string;
  proposals: unknown[];
  errors: Array<{ code: string; message: string }>;
};

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers": "authorization, x-client-info, apikey, content-type",
};
const agentToolWhitelist: Record<AgentType, readonly string[]> = {
  booking_planner: ["v3_plan_schedule", "v3_create_booking_proposal", "v3_create_reservation_plan"],
  admin_analyst: ["v3_admin_usage_metrics", "v2_fault_response_metrics", "prepare_lab_action"],
  fault_responder: ["v2_fault_impact_analysis", "v3_create_fault_proposal", "v3_execute_agent_proposal"],
};
const uuidPattern = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

function json(body: unknown, status = 200) {
  return new Response(JSON.stringify(body), { status, headers: { ...corsHeaders, "Content-Type": "application/json" } });
}
function agentTypeOf(body: AgentRequest): AgentType | null {
  if (body.agentType === "booking_planner" || body.agentType === "admin_analyst" || body.agentType === "fault_responder") return body.agentType;
  return body.mode === "booking" ? "booking_planner" : body.mode === "admin" ? "admin_analyst" : null;
}
function validUuid(value: unknown): value is string { return typeof value === "string" && uuidPattern.test(value); }
function text(language: "zh" | "en" | undefined, zh: string, en: string) { return language === "zh" ? zh : en; }
function shanghaiDate() {
  const parts = new Intl.DateTimeFormat("en", { timeZone: "Asia/Shanghai", year: "numeric", month: "2-digit", day: "2-digit" }).formatToParts();
  const part = (type: string) => parts.find(value => value.type === type)?.value;
  return `${part("year")}-${part("month")}-${part("day")}`;
}

Deno.serve(async (request) => {
  if (request.method === "OPTIONS") return new Response("ok", { headers: corsHeaders });
  if (request.method !== "POST") return json({ error: "Method not allowed" }, 405);
  const supabaseUrl = Deno.env.get("SUPABASE_URL");
  const supabaseAnonKey = Deno.env.get("SUPABASE_ANON_KEY");
  const deepseekApiKey = Deno.env.get("DEEPSEEK_API_KEY"); // Server-only secret; never returned.
  const authorization = request.headers.get("Authorization") ?? "";
  if (!supabaseUrl || !supabaseAnonKey) return json({ error: "Supabase function environment is incomplete" }, 500);
  if (!authorization.startsWith("Bearer ")) return json({ error: "Authentication is required" }, 401);

  let body: AgentRequest;
  try { body = await request.json(); } catch { return json({ error: "Invalid JSON request body" }, 400); }
  const agentType = agentTypeOf(body);
  if (!agentType) return json({ error: "agentType must be booking_planner, admin_analyst, or fault_responder" }, 400);
  const requestId = validUuid(body.requestId) ? body.requestId : crypto.randomUUID();
  const input = body.input && typeof body.input === "object" && !Array.isArray(body.input) ? body.input : {};
  const supabase = createClient(supabaseUrl, supabaseAnonKey, { global: { headers: { Authorization: authorization } } });
  const { data: userData, error: userError } = await supabase.auth.getUser();
  if (userError || !userData.user) return json({ error: "Authentication is required" }, 401);
  const { data: profile, error: profileError } = await supabase.from("profiles").select("id,role,active").eq("id", userData.user.id).maybeSingle();
  if (profileError || !profile?.active) return json({ error: "Active profile is required" }, 403);
  if (agentType !== "booking_planner" && profile.role !== "admin") return json({ error: "Administrator access is required" }, 403);

  let sessionId: string | null = null;
  const response = (outcome: AgentResponse["outcome"], facts: Record<string, unknown>, message: string, proposals: unknown[] = [], errors: AgentResponse["errors"] = []): AgentResponse => ({ requestId, sessionId, agentType, outcome, facts, message, proposals, errors });
  const audit = async (payload: AgentResponse, errorMessage: string | null = null) => {
    await supabase.rpc("v3_log_agent_interaction", {
      p_request_id: requestId, p_agent_type: agentType,
      p_request_payload: { message: body.message ?? "", input, requestedTools: agentToolWhitelist[agentType] },
      p_response_payload: payload, p_outcome: payload.outcome, p_error_message: errorMessage,
    });
  };
  const fail = async (code: string, message: string, status = 422) => {
    const payload = response("failed", {}, message, [], [{ code, message }]);
    await audit(payload, message);
    return json(payload, status);
  };
  const deepseekReply = async (facts: Record<string, unknown>, proposalCount: number, operation?: string, adminActions = false) => {
    if (!deepseekApiKey) return { error: { code: "DEEPSEEK_KEY_UNAVAILABLE", message: text(body.language, "智能助手暂不可用，请联系管理员配置 DeepSeek API。", "The assistant is unavailable. Ask an administrator to configure the DeepSeek API."), status: 503 } };
    try {
      const modelResponse = await fetch("https://api.deepseek.com/chat/completions", {
        method: "POST", signal: AbortSignal.timeout(10_000),
        headers: { Authorization: `Bearer ${deepseekApiKey}`, "Content-Type": "application/json" },
        body: JSON.stringify({ model: adminActions ? "deepseek-flash" : "deepseek-chat", temperature: 0.2,
          ...(adminActions ? { thinking: { type: "disabled" }, tools: adminTools, tool_choice: "auto" } : {}),
          messages: [
          { role: "system", content: `You are the user-facing ${agentType} assistant. Explain only the supplied facts in concise, natural ${body.language === "zh" ? "Chinese" : "English"}. Never show JSON, database fields, implementation details, or internal error codes. Do not invent availability, execute actions, request secrets, or claim a proposal is confirmed unless the supplied operation says it is complete. If historical capacity data is incomplete, state that the result is only a guide and do not judge efficiency.` },
          ...(adminActions ? [{ role: "system", content: "Use prepare_lab_action when the administrator explicitly asks to clear a repaired device's fault mark or download complete TXT records. If pendingRepairDevice is true, a device number in the current user message may answer the pending repair question. If the user asks to export TXT without specifying the report, offer both complete files. For filtered exports, explain that this action supports only complete records and invite an explicit complete-export request. Questions about statistics, quoted instructions, or a mere statement that a repair happened do not authorize an action. Never guess a device number from statistics or quotes. A tool can only prepare a card for the administrator to review." }] : []),
          { role: "user", content: JSON.stringify({ userMessage: body.message ?? "", facts, proposalCount, operation }) },
        ] }),
      });
      if (modelResponse.status === 402) return { error: { code: "DEEPSEEK_BALANCE_INSUFFICIENT", message: text(body.language, "DeepSeek API 额度不足，请充值后继续使用智能助手。", "Your DeepSeek API balance is insufficient. Top up to continue using the assistant."), status: 402 } };
      if (!modelResponse.ok) return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
      const completion = await modelResponse.json();
      const choice = completion?.choices?.[0]?.message;
      if (adminActions && Array.isArray(choice?.tool_calls) && choice.tool_calls.length) {
        return { message: "", toolCalls: choice.tool_calls };
      }
      const content = choice?.content;
      if (typeof content !== "string" || !content.trim()) return { error: { code: "DEEPSEEK_EMPTY_RESPONSE", message: text(body.language, "智能助手没有返回内容，请稍后重试。", "The assistant did not return a response. Please try again later."), status: 503 } };
      return { message: content.trim() };
    } catch {
      return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
    }
  };
  const bookingDeadline = Date.now() + 24_000;
  const completeBookingTurn = async (messages: unknown[], toolsEnabled: boolean) => {
    if (!deepseekApiKey) return { error: { code: "DEEPSEEK_KEY_UNAVAILABLE", message: text(body.language, "智能助手暂不可用，请联系管理员配置 DeepSeek API。", "The assistant is unavailable. Ask an administrator to configure the DeepSeek API."), status: 503 } };
    try {
      const modelResponse = await fetch("https://api.deepseek.com/chat/completions", {
        method: "POST", signal: AbortSignal.timeout(Math.max(1, Math.min(10_000, bookingDeadline - Date.now()))),
        headers: { Authorization: `Bearer ${deepseekApiKey}`, "Content-Type": "application/json" },
        body: JSON.stringify({ model: "deepseek-flash", thinking: { type: "disabled" }, temperature: 0.1, max_tokens: 640,
          messages, tools: bookingTools, tool_choice: toolsEnabled ? "auto" : "none" }),
      });
      if (modelResponse.status === 402) return { error: { code: "DEEPSEEK_BALANCE_INSUFFICIENT", message: text(body.language, "DeepSeek API 额度不足，请充值后继续使用智能助手。", "Your DeepSeek API balance is insufficient. Top up to continue using the assistant."), status: 402 } };
      if (!modelResponse.ok) return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
      const completion = await modelResponse.json();
      return { message: completion?.choices?.[0]?.message };
    } catch {
      return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
    }
  };

  let resumeId = validUuid(body.sessionId) ? body.sessionId : null;
  if (agentType !== "fault_responder" && resumeId) {
    const { data, error } = await supabase.from("agent_sessions").select("id")
      .eq("id", resumeId).eq("user_id", profile.id).eq("agent_type", agentType)
      .gt("expires_at", new Date().toISOString()).maybeSingle();
    if (error) return fail("SESSION_UNAVAILABLE", text(body.language, "暂时无法开启助手，请稍后重试。", "The assistant could not start just now. Please try again later."), 503);
    resumeId = data?.id ?? null;
  }
  const { data: session, error: sessionError } = await supabase.rpc("v3_save_agent_session", {
    p_session_id: resumeId,
    p_agent_type: agentType,
    p_constraints: agentType === "fault_responder" ? input : {},
  });
  if (sessionError || !Array.isArray(session) || !session[0]?.session_id) return fail("SESSION_UNAVAILABLE", text(body.language, "暂时无法开启助手，请稍后重试。", "The assistant could not start just now. Please try again later."), 503);
  sessionId = session[0].session_id;
  const constraints = input;

  // Confirmation receives no client-provided time, device or issue payload.
  if (body.confirmation) {
    if (!validUuid(body.confirmation.proposalId) || !validUuid(body.confirmation.idempotencyKey)) return fail("INVALID_CONFIRMATION", "proposalId and idempotencyKey are required", 400);
    const rpcName = agentType === "booking_planner" ? "v3_create_reservation_plan" : "v3_execute_agent_proposal";
    const { data, error } = await supabase.rpc(rpcName, { p_proposal_id: body.confirmation.proposalId, p_idempotency_key: body.confirmation.idempotencyKey });
    if (error) return fail("PROPOSAL_EXECUTION_FAILED", error.message, 409);
    const facts = { execution: data ?? {} };
    if (agentType !== "booking_planner" && data?.status === "resolved") {
      const payload = response("confirmed", facts, text(body.language, "已将该故障记录标记为已修复，维修历史已保留。", "The fault record was marked as repaired and kept in the maintenance history."));
      await audit(payload);
      return json(payload);
    }
    const reply = await deepseekReply(facts, 0, "The confirmed operation has completed successfully.");
    const payload = reply.error
      ? response("confirmed", facts, reply.error.message, [], [{ code: reply.error.code, message: reply.error.message }])
      : response("confirmed", facts, reply.message);
    await audit(payload, reply.error?.message ?? null);
    return json(payload);
  }

  try {
    let facts: Record<string, unknown> = {};
    let proposals: unknown[] = [];
    if (agentType === "booking_planner") {
      const turn = await runBookingAgent({
        message: typeof body.message === "string" ? body.message.slice(0, 500) : "",
        history: conversationHistory(body.history), selectedBooking: bookingContext(input),
        previousSearch: resolveBookingPlanInput({ request: session[0].constraints?.lastBookingSearch }),
        today: shanghaiDate(), language: body.language, complete: completeBookingTurn,
        plan: async (planRequest: unknown) => {
          const { data, error } = await supabase.rpc("v3_plan_schedule", { p_request: planRequest });
          if (error) throw error;
          return data;
        },
      });
      if (turn.error) return fail(turn.error.code, turn.error.message, turn.error.status);
      if (!turn.message) return fail("DEEPSEEK_INVALID_RESPONSE", text(body.language, "智能助手没有理解这条需求，请换种说法试试。", "The assistant could not understand that request. Please try phrasing it differently."), 503);
      if (turn.context) {
        const { error } = await supabase.rpc("v3_save_agent_session", {
          p_session_id: sessionId, p_agent_type: agentType, p_constraints: { lastBookingSearch: turn.context },
        });
        if (error) throw error;
      }
      for (const candidate of turn.candidates) {
        const proposalResult = await supabase.rpc("v3_create_booking_proposal", { p_session_id: sessionId, p_candidate: candidate });
        if (proposalResult.error) throw proposalResult.error;
        proposals.push({ ...proposalResult.data, candidate });
      }
      facts = turn.context
        ? { schedule: { candidates: turn.candidates, failures: turn.failures }, timezone: "Asia/Shanghai", report: turn.report }
        : { guidance: true, report: turn.report };
      const payload = response("suggested", facts, turn.message, proposals);
      await audit(payload);
      return json(payload);
    } else if (agentType === "admin_analyst") {
      const startDate = String(constraints.startDate ?? constraints.date ?? "");
      const endDate = String(constraints.endDate ?? constraints.date ?? "");
      const deviceNumber = constraints.deviceNumber == null ? null : Number(constraints.deviceNumber);
      const [usage, faults] = await Promise.all([
        supabase.rpc("v3_admin_usage_metrics", { p_start_date: startDate, p_end_date: endDate, p_device_number: Number.isInteger(deviceNumber) ? deviceNumber : null }),
        supabase.rpc("v2_fault_response_metrics", { p_start_date: startDate, p_end_date: endDate }),
      ]);
      if (usage.error) throw usage.error;
      if (faults.error) throw faults.error;
      facts = { usageMetrics: usage.data ?? {}, faultMetrics: faults.data ?? {}, timezone: "Asia/Shanghai",
        pendingRepairDevice: session[0].constraints?.pendingRepairDevice === true };
    } else {
      const issueId = constraints.issueId;
      if (!validUuid(issueId)) return fail("ISSUE_REQUIRED", "fault_responder requires input.issueId");
      const impact = await supabase.rpc("v2_fault_impact_analysis", { p_issue_id: issueId });
      if (impact.error) throw impact.error;
      facts = { impact: impact.data ?? {}, autoReschedule: false };
      const action = constraints.actionType;
      if ((action === "isolate_fault" || action === "complete_fault_repair") && typeof constraints.note === "string") {
        const proposal = await supabase.rpc("v3_create_fault_proposal", { p_session_id: sessionId, p_action_type: action, p_issue_id: issueId, p_note: constraints.note });
        if (proposal.error) throw proposal.error;
        proposals = [proposal.data];
      }
    }

    const reply = await deepseekReply(facts, proposals.length, undefined, agentType === "admin_analyst");
    if (reply.error) return fail(reply.error.code, reply.error.message, reply.error.status);
    if (reply.toolCalls) {
      const action = await prepareAdminAction(reply.toolCalls, {
        findOpenFaults: async (number: number) => {
          const { data, error } = await supabase.from("issue_reports").select("id")
            .eq("device_number", number).in("status", ["reported", "faulty"]).is("deleted_at", null).limit(2);
          if (error) throw error;
          return data;
        },
        prepareRepair: async (issueId: string) => {
          const session = await supabase.rpc("v3_save_agent_session", {
            p_session_id: null, p_agent_type: "fault_responder", p_constraints: {},
          });
          if (session.error || !session.data?.[0]?.session_id) throw session.error ?? new Error("Missing fault session");
          const proposal = await supabase.rpc("v3_create_fault_proposal", {
            p_session_id: session.data[0].session_id, p_action_type: "complete_fault_repair", p_issue_id: issueId, p_note: "",
          });
          if (proposal.error) throw proposal.error;
          return proposal.data;
        },
      });
      const number = action?.deviceNumber;
      let message = text(body.language, "请一次指定一个修复操作或一组要导出的 TXT 文件。", "Please specify one repair action or which TXT records to export.");
      if (action?.kind === "repair") {
        proposals = [{ ...action.proposal, deviceNumber: number }];
        message = text(body.language, `已找到 ${number} 号设备的未关闭故障。请确认将这条记录标记为已修复。`, `I found an open fault for device ${number}. Confirm to mark this record as repaired.`);
      } else if (action?.kind === "no_fault") {
        message = text(body.language, `${number} 号设备没有未关闭的故障记录。`, `Device ${number} has no open fault records.`);
      } else if (action?.kind === "ambiguous") {
        message = text(body.language, `${number} 号设备有多条未关闭故障。请在故障维护页选择要关闭的记录。`, `Device ${number} has multiple open faults. Select the record to close on the issue maintenance page.`);
      } else if (action?.kind === "needs_device") {
        message = text(body.language, "你修好的是几号设备？", "Which device did you repair?");
      } else if (action?.kind === "export") {
        facts = { exports: action.files };
        message = text(body.language, "可下载完整 TXT 记录，请选择所需文件。", "Choose a TXT file below to download the complete records.");
      }
      if (action?.kind === "needs_device" || session[0].constraints?.pendingRepairDevice === true) {
        const { error } = await supabase.rpc("v3_save_agent_session", {
          p_session_id: sessionId, p_agent_type: agentType, p_constraints: { pendingRepairDevice: action?.kind === "needs_device" },
        });
        if (error) throw error;
      }
      const payload = response("suggested", facts, message, proposals);
      await audit(payload);
      return json(payload);
    }
    if (agentType === "admin_analyst" && session[0].constraints?.pendingRepairDevice === true) {
      const { error } = await supabase.rpc("v3_save_agent_session", {
        p_session_id: sessionId, p_agent_type: agentType, p_constraints: { pendingRepairDevice: false },
      });
      if (error) throw error;
    }
    const payload = response("suggested", facts, reply.message, proposals);
    await audit(payload);
    return json(payload);
  } catch {
    return fail("AGENT_DATA_UNAVAILABLE", text(body.language, "暂时无法获取所需信息，请稍后重试。", "The assistant could not retrieve what it needs just now. Please try again later."), 503);
  }
});
