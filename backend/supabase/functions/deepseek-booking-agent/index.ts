import { createClient } from "https://esm.sh/@supabase/supabase-js@2";
import { bookingContext, resolveBookingPlanInput } from "./booking-request.mjs";
import { conversationHistory } from "./conversation-history.mjs";

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
  admin_analyst: ["v3_admin_usage_metrics", "v2_fault_response_metrics"],
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
  const deepseekReply = async (facts: Record<string, unknown>, proposalCount: number, operation?: string) => {
    if (!deepseekApiKey) return { error: { code: "DEEPSEEK_KEY_UNAVAILABLE", message: text(body.language, "智能助手暂不可用，请联系管理员配置 DeepSeek API。", "The assistant is unavailable. Ask an administrator to configure the DeepSeek API."), status: 503 } };
    try {
      const modelResponse = await fetch("https://api.deepseek.com/chat/completions", {
        method: "POST", signal: AbortSignal.timeout(10_000),
        headers: { Authorization: `Bearer ${deepseekApiKey}`, "Content-Type": "application/json" },
        body: JSON.stringify({ model: "deepseek-chat", temperature: 0.2, messages: [
          { role: "system", content: `You are the user-facing ${agentType} assistant. Explain only the supplied facts in concise, natural ${body.language === "zh" ? "Chinese" : "English"}. Never show JSON, database fields, implementation details, or internal error codes. Do not invent availability, execute actions, request secrets, or claim a proposal is confirmed unless the supplied operation says it is complete. If historical capacity data is incomplete, state that the result is only a guide and do not judge efficiency.` },
          { role: "user", content: JSON.stringify({ userMessage: body.message ?? "", facts, proposalCount, operation }) },
        ] }),
      });
      if (modelResponse.status === 402) return { error: { code: "DEEPSEEK_BALANCE_INSUFFICIENT", message: text(body.language, "DeepSeek API 额度不足，请充值后继续使用智能助手。", "Your DeepSeek API balance is insufficient. Top up to continue using the assistant."), status: 402 } };
      if (!modelResponse.ok) return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
      const completion = await modelResponse.json();
      const content = completion?.choices?.[0]?.message?.content;
      if (typeof content !== "string" || !content.trim()) return { error: { code: "DEEPSEEK_EMPTY_RESPONSE", message: text(body.language, "智能助手没有返回内容，请稍后重试。", "The assistant did not return a response. Please try again later."), status: 503 } };
      return { message: content.trim() };
    } catch {
      return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
    }
  };
  const deepseekBookingDecision = async () => {
    if (!deepseekApiKey) return { error: { code: "DEEPSEEK_KEY_UNAVAILABLE", message: text(body.language, "智能助手暂不可用，请联系管理员配置 DeepSeek API。", "The assistant is unavailable. Ask an administrator to configure the DeepSeek API."), status: 503 } };
    try {
      const modelResponse = await fetch("https://api.deepseek.com/chat/completions", {
        method: "POST", signal: AbortSignal.timeout(10_000), headers: { Authorization: `Bearer ${deepseekApiKey}`, "Content-Type": "application/json" },
        body: JSON.stringify({ model: "deepseek-chat", temperature: 0.1, response_format: { type: "json_object" }, messages: [
          { role: "system", content: `You are the intake step for a laboratory booking assistant. Today is ${shanghaiDate()} in Asia/Shanghai. Return one JSON object only: {"ready":boolean,"request":object|null,"message":string|null}. Use the user's message and, only when present, the explicitly selected booking context. Set ready=true only when date, an exact start hour, duration, and device count are all known; resolve relative dates to YYYY-MM-DD. When ready=false, request the missing details in one concise, helpful ${body.language === "zh" ? "Chinese" : "English"} sentence. Never check or mention availability, capacity, database fields, defaults, internal state, or an earlier time slot. Do not invent missing booking details.` },
          { role: "user", content: JSON.stringify({ history: conversationHistory(body.history), userMessage: body.message ?? "", selectedBooking: bookingContext(input) }) },
        ] }),
      });
      if (modelResponse.status === 402) return { error: { code: "DEEPSEEK_BALANCE_INSUFFICIENT", message: text(body.language, "DeepSeek API 额度不足，请充值后继续使用智能助手。", "Your DeepSeek API balance is insufficient. Top up to continue using the assistant."), status: 402 } };
      if (!modelResponse.ok) return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
      const completion = await modelResponse.json();
      const content = completion?.choices?.[0]?.message?.content;
      if (typeof content !== "string") return { error: { code: "DEEPSEEK_EMPTY_RESPONSE", message: text(body.language, "智能助手没有返回内容，请稍后重试。", "The assistant did not return a response. Please try again later."), status: 503 } };
      const decision = JSON.parse(content);
      const planRequest = resolveBookingPlanInput(decision);
      const message = typeof decision?.message === "string" ? decision.message.trim() : "";
      if (!planRequest && !message) return { error: { code: "DEEPSEEK_INVALID_RESPONSE", message: text(body.language, "智能助手没有理解这条需求，请换种说法试试。", "The assistant could not understand that request. Please try phrasing it differently."), status: 503 } };
      return { planRequest, message };
    } catch {
      return { error: { code: "DEEPSEEK_UNAVAILABLE", message: text(body.language, "智能助手暂时不可用，请稍后重试。", "The assistant is temporarily unavailable. Please try again later."), status: 503 } };
    }
  };

  const { data: session, error: sessionError } = await supabase.rpc("v3_save_agent_session", {
    p_session_id: validUuid(body.sessionId) ? body.sessionId : null,
    p_agent_type: agentType,
    p_constraints: input,
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
    const reply = await deepseekReply(facts, 0, "The confirmed operation has completed successfully.");
    const payload = "error" in reply
      ? response("confirmed", facts, reply.error.message, [], [{ code: reply.error.code, message: reply.error.message }])
      : response("confirmed", facts, reply.message);
    await audit(payload, "error" in reply ? reply.error.message : null);
    return json(payload);
  }

  try {
    let facts: Record<string, unknown> = {};
    let proposals: unknown[] = [];
    if (agentType === "booking_planner") {
      const decision = await deepseekBookingDecision();
      if ("error" in decision) return fail(decision.error.code, decision.error.message, decision.error.status);
      if (!decision.planRequest) {
        const payload = response("suggested", { guidance: true }, decision.message);
        await audit(payload);
        return json(payload);
      }
      const { data: schedule, error } = await supabase.rpc("v3_plan_schedule", { p_request: decision.planRequest });
      if (error) return fail("PLAN_UNAVAILABLE", text(body.language, "暂时无法生成预约方案，请稍后重试。", "A reservation plan could not be prepared just now. Please try again later."), 503);
      const candidates = Array.isArray(schedule?.candidates) ? schedule.candidates.slice(0, 3) : [];
      for (const candidate of candidates) {
        const proposalResult = await supabase.rpc("v3_create_booking_proposal", { p_session_id: sessionId, p_candidate: candidate });
        if (proposalResult.error) throw proposalResult.error;
        proposals.push({ ...proposalResult.data, candidate });
      }
      facts = { schedule: { ...schedule, candidates }, timezone: "Asia/Shanghai", contract: "Candidates are deterministic database facts; confirmation accepts proposalId only." };
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
      facts = { usageMetrics: usage.data ?? {}, faultMetrics: faults.data ?? {}, timezone: "Asia/Shanghai" };
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

    const reply = await deepseekReply(facts, proposals.length);
    if ("error" in reply) return fail(reply.error.code, reply.error.message, reply.error.status);
    const payload = response("suggested", facts, reply.message, proposals);
    await audit(payload);
    return json(payload);
  } catch {
    return fail("AGENT_DATA_UNAVAILABLE", text(body.language, "暂时无法获取所需信息，请稍后重试。", "The assistant could not retrieve what it needs just now. Please try again later."), 503);
  }
});
