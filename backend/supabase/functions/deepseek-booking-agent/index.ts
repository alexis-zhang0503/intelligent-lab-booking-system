import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

type AgentRequest = {
  message?: string;
  mode: "booking" | "admin";
  language?: "zh" | "en";
  input?: {
    date?: string;
    preferredHour?: number;
    requiredDeviceCount?: number;
    preferredDevices?: number[];
    startDate?: string;
    endDate?: string;
  };
  confirmation?: {
    accepted: boolean;
    date: string;
    startHour: number;
    deviceNumbers: number[];
    idempotencyKey: string;
  };
};

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers": "authorization, x-client-info, apikey, content-type",
};

function json(body: unknown, status = 200) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { ...corsHeaders, "Content-Type": "application/json" },
  });
}

function parseHour(value: unknown, fallback: number | null = null) {
  const hour = Number(value);
  return Number.isInteger(hour) && hour >= 0 && hour <= 23 ? hour : fallback;
}

function parseDeviceCount(value: unknown) {
  const count = Number(value ?? 1);
  if (!Number.isInteger(count) || count < 1 || count > 40) throw new Error("requiredDeviceCount must be an integer from 1 to 40");
  return count;
}

function parseDevices(value: unknown) {
  if (!Array.isArray(value) || value.some((device) => !Number.isInteger(Number(device)) || Number(device) < 1 || Number(device) > 40)) {
    throw new Error("deviceNumbers must contain laboratory device numbers from 1 to 40");
  }
  const devices = [...new Set(value.map(Number))].sort((a, b) => a - b);
  if (!devices.length) throw new Error("At least one device is required");
  return devices;
}

Deno.serve(async (request) => {
  if (request.method === "OPTIONS") return new Response("ok", { headers: corsHeaders });
  if (request.method !== "POST") return json({ error: "Method not allowed" }, 405);

  const supabaseUrl = Deno.env.get("SUPABASE_URL");
  const supabaseAnonKey = Deno.env.get("SUPABASE_ANON_KEY");
  const deepseekApiKey = Deno.env.get("DEEPSEEK_API_KEY");
  const authorization = request.headers.get("Authorization") ?? "";
  if (!supabaseUrl || !supabaseAnonKey) return json({ error: "Supabase function environment is incomplete" }, 500);
  if (!authorization.startsWith("Bearer ")) return json({ error: "Authentication is required" }, 401);

  let body: AgentRequest;
  try {
    body = await request.json();
  } catch {
    return json({ error: "Invalid JSON request body" }, 400);
  }
  if (body.mode !== "booking" && body.mode !== "admin") return json({ error: "mode must be booking or admin" }, 400);
  const respondInChinese = body.language === "zh";
  const fallbackMessage = respondInChinese
    ? "智能服务暂不可用，现已展示基于规则的实时结果。"
    : "AI is unavailable; rule-based real-time results are shown instead.";

  const supabase = createClient(supabaseUrl, supabaseAnonKey, { global: { headers: { Authorization: authorization } } });
  const { data: userData, error: userError } = await supabase.auth.getUser();
  if (userError || !userData.user) return json({ error: "Authentication is required" }, 401);
  const { data: profile, error: profileError } = await supabase
    .from("profiles")
    .select("id,role,active")
    .eq("id", userData.user.id)
    .maybeSingle();
  if (profileError || !profile?.active) return json({ error: "Active profile is required" }, 403);
  if (body.mode === "admin" && profile.role !== "admin") return json({ error: "Administrator access is required" }, 403);

  const requestId = crypto.randomUUID();
  const input = body.input ?? {};
  const audit = async (outcome: "suggested" | "confirmed" | "fallback" | "failed", responsePayload: unknown, errorMessage: string | null = null) => {
    await supabase.rpc("v2_log_agent_interaction", {
      p_request_id: requestId,
      p_agent_type: body.mode,
      p_request_payload: { message: body.message ?? "", input },
      p_response_payload: responsePayload,
      p_outcome: outcome,
      p_error_message: errorMessage,
    });
  };

  if (body.confirmation?.accepted) {
    if (body.mode !== "booking") return json({ error: "Only the booking agent accepts a reservation confirmation" }, 400);
    try {
      const devices = parseDevices(body.confirmation.deviceNumbers);
      const hour = parseHour(body.confirmation.startHour);
      if (!hour || !/^\d{4}-\d{2}-\d{2}$/.test(body.confirmation.date) || !body.confirmation.idempotencyKey) throw new Error("Invalid confirmation payload");
      // This is the only write path: the Agent calls the audited business RPC, never a table mutation.
      const { data, error } = await supabase.rpc("v2_create_reservations", {
        p_date: body.confirmation.date,
        p_start_hour: hour,
        p_device_numbers: devices,
        p_idempotency_key: body.confirmation.idempotencyKey,
      });
      if (error) throw error;
      const response = { requestId, outcome: "confirmed", reservations: data };
      await audit("confirmed", response);
      return json(response);
    } catch (error) {
      const message = error instanceof Error ? error.message : "Reservation confirmation failed";
      await audit("failed", { requestId }, message);
      return json({ requestId, error: message }, 409);
    }
  }

  const runTool = async (name: string, args: Record<string, unknown>) => {
    if (name === "get_schedule_candidates") {
      const date = String(args.date ?? input.date ?? "");
      const preferredHour = parseHour(args.preferredHour ?? input.preferredHour);
      const requiredDeviceCount = parseDeviceCount(args.requiredDeviceCount ?? input.requiredDeviceCount);
      const preferredDevices = Array.isArray(args.preferredDevices ?? input.preferredDevices) ? (args.preferredDevices ?? input.preferredDevices) : [];
      const { data, error } = await supabase.rpc("v2_schedule_candidates", {
        p_date: date,
        p_preferred_hour: preferredHour,
        p_required_device_count: requiredDeviceCount,
        p_preferred_devices: preferredDevices,
        p_limit: 3,
      });
      if (error) throw error;
      await supabase.rpc("v2_log_schedule_candidates", {
        p_date: date,
        p_preferred_hour: preferredHour,
        p_required_device_count: requiredDeviceCount,
        p_candidates: data ?? [],
      });
      return data ?? [];
    }
    if (name === "get_admin_metrics" && profile.role === "admin") {
      const { data, error } = await supabase.rpc("v2_admin_metrics", {
        p_start_date: String(args.startDate ?? input.startDate ?? ""),
        p_end_date: String(args.endDate ?? input.endDate ?? ""),
      });
      if (error) throw error;
      return data;
    }
    throw new Error("Unsupported or unauthorized tool");
  };

  const tools = [
    {
      type: "function",
      function: {
        name: "get_schedule_candidates",
        description: "Read the real-time top three legal reservation candidates. This never creates a reservation.",
        parameters: {
          type: "object",
          additionalProperties: false,
          required: ["date", "requiredDeviceCount"],
          properties: {
            date: { type: "string", description: "Reservation date in YYYY-MM-DD" },
            preferredHour: { type: "integer", minimum: 0, maximum: 23 },
            requiredDeviceCount: { type: "integer", minimum: 1, maximum: 40 },
            preferredDevices: { type: "array", items: { type: "integer", minimum: 1, maximum: 40 } },
          },
        },
      },
    },
    ...(profile.role === "admin" ? [{
      type: "function",
      function: {
        name: "get_admin_metrics",
        description: "Read booking, check-in usage, no-show, cancellation, popular-hour, and data-completeness metrics. This is read-only.",
        parameters: {
          type: "object",
          additionalProperties: false,
          required: ["startDate", "endDate"],
          properties: { startDate: { type: "string" }, endDate: { type: "string" } },
        },
      },
    }] : []),
  ];

  const fallback = async (reason: string) => {
    try {
      if (body.mode === "booking") {
        const candidates = await runTool("get_schedule_candidates", {
          date: input.date,
          preferredHour: input.preferredHour,
          requiredDeviceCount: input.requiredDeviceCount,
          preferredDevices: input.preferredDevices,
        });
        const response = { requestId, outcome: "fallback", message: fallbackMessage, candidates };
        await audit("fallback", response, reason);
        return json(response);
      }
      const metrics = await runTool("get_admin_metrics", { startDate: input.startDate, endDate: input.endDate });
      const response = { requestId, outcome: "fallback", message: respondInChinese ? "智能服务暂不可用，现已展示实时指标。" : "AI is unavailable; live metrics are shown without interpretation.", metrics };
      await audit("fallback", response, reason);
      return json(response);
    } catch (error) {
      const message = error instanceof Error ? error.message : reason;
      await audit("failed", { requestId }, message);
      return json({ requestId, error: message }, 503);
    }
  };

  if (!deepseekApiKey) return fallback("DEEPSEEK_API_KEY is not configured");

  const responseLanguage = respondInChinese ? "Chinese" : "English";
  const system = body.mode === "booking"
    ? `You are an appointment assistant. Use get_schedule_candidates for availability. Never claim a reservation exists, never request secrets, and state that the user must explicitly confirm before booking. Respond entirely in ${responseLanguage}.`
    : `You are a read-only laboratory administrator assistant. Use get_admin_metrics, distinguish booking rate from check-in usage rate, and do not make operational changes. Respond entirely in ${responseLanguage}.`;
  const messages: Array<Record<string, unknown>> = [
    { role: "system", content: system },
    { role: "user", content: body.message || JSON.stringify(input) },
  ];

  try {
    for (let round = 0; round < 3; round += 1) {
      const response = await fetch("https://api.deepseek.com/chat/completions", {
        method: "POST",
        headers: { Authorization: `Bearer ${deepseekApiKey}`, "Content-Type": "application/json" },
        body: JSON.stringify({ model: "deepseek-chat", temperature: 0.2, messages, tools, tool_choice: "auto" }),
      });
      if (!response.ok) throw new Error(`DeepSeek request failed with HTTP ${response.status}`);
      const completion = await response.json();
      const message = completion?.choices?.[0]?.message;
      if (!message) throw new Error("DeepSeek returned no assistant message");
      const toolCalls = Array.isArray(message.tool_calls) ? message.tool_calls : [];
      messages.push({ role: "assistant", content: message.content ?? "", tool_calls: toolCalls });
      if (!toolCalls.length) {
        const result = { requestId, outcome: "suggested", message: message.content ?? "No recommendation was generated." };
        await audit("suggested", result);
        return json(result);
      }
      for (const call of toolCalls) {
        let args: Record<string, unknown> = {};
        try { args = JSON.parse(call.function.arguments ?? "{}"); } catch { throw new Error("DeepSeek returned invalid tool arguments"); }
        const result = await runTool(call.function.name, args);
        messages.push({ role: "tool", tool_call_id: call.id, content: JSON.stringify(result) });
      }
    }
    throw new Error("DeepSeek exceeded the allowed tool-call rounds");
  } catch (error) {
    return fallback(error instanceof Error ? error.message : "DeepSeek agent failed");
  }
});
