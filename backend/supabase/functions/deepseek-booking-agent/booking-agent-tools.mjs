import { resolveBookingPlanInput } from "./booking-request.mjs";
import { explicitBookingRequest } from "./explicit-booking-request.mjs";

const openingHours = Array.from({ length: 9 }, (_, i) => i + 8);
const maxToolCalls = 3;

export const bookingTools = [{
  type: "function",
  function: {
    name: "search_availability",
    description: "Check real availability without reserving. In alternatives mode, each hour is a separate option on date; omit hours to scan the day. Use series mode only when the user wants all dates/hours booked together.",
    parameters: {
      type: "object",
      properties: {
        mode: { type: "string", enum: ["alternatives", "series"] },
        date: { type: "string", description: "YYYY-MM-DD; required for alternatives" },
        startDate: { type: "string", description: "First date for a series" },
        endDate: { type: "string", description: "Last date for a series" },
        startHours: { type: "array", items: { type: "integer" }, maxItems: 9 },
        durationHours: { type: "integer", minimum: 1, maximum: 9 },
        requiredDeviceCount: { type: "integer", minimum: 1, maximum: 40 },
        preferredDevices: { type: "array", items: { type: "integer" } },
        weekdays: { type: "array", items: { type: "integer" }, description: "Series only: Sunday=0 through Saturday=6" },
      },
      required: ["durationHours", "requiredDeviceCount"],
      additionalProperties: false,
    },
  },
}, {
  type: "function",
  function: {
    name: "request_user_decision",
    description: "Stop and ask one concrete question only when useful read-only work cannot continue without a decision from the user. Do not use this just because the user delegated a choice.",
    parameters: {
      type: "object",
      properties: { question: { type: "string" } },
      required: ["question"],
      additionalProperties: false,
    },
  },
}, {
  type: "function",
  function: {
    name: "prepare_fault_report",
    description: "Prepare a student fault report for review when the student reports a broken device or asks to mark it faulty. No report is submitted by this tool. Use only the stated device and symptoms; omit anything not supplied.",
    parameters: {
      type: "object",
      properties: {
        deviceNumber: { type: "integer", minimum: 1, maximum: 40 },
        description: { type: "string", maxLength: 120, description: "Optional fault symptoms in the student's language, without assumptions" },
      },
      additionalProperties: false,
    },
  },
}];

function dateInRange(value, today) {
  if (!value) return false;
  const date = new Date(`${value}T00:00:00Z`);
  const days = (date.getTime() - Date.parse(`${today}T00:00:00Z`)) / 86400000;
  return days >= 0 && days <= 90 && date.toISOString().slice(0, 10) === value;
}

async function searchAvailability(args, today, plan) {
  const series = args?.mode === "series";
  const startHours = args?.startHours ?? (series ? undefined : openingHours);
  if ((args?.mode && !["alternatives", "series"].includes(args.mode)) ||
      (!series && (!args?.date || args.startDate || args.endDate || args.weekdays)) ||
      !Array.isArray(startHours) || startHours.length > 9) {
    return { error: "Use one date for alternatives, or explicitly specify a series and its hours." };
  }
  const request = resolveBookingPlanInput({ request: { ...args, startHours } });
  if (!request || !dateInRange(request.date ?? request.startDate, today) || !dateInRange(request.date ?? request.endDate, today)) {
    return { error: "Invalid booking parameters. Check the date range, hours, duration and device count." };
  }
  const requests = series ? [request] : request.startHours.map(hour => ({ ...request, startHours: [hour] }));
  const results = await Promise.all(requests.map(plan));
  const options = results.flatMap(({ candidates }) => requests.length === 1 ? candidates : candidates.slice(0, 1));
  const candidates = [...new Map(options.map(candidate => [JSON.stringify(candidate.items), candidate])).values()].slice(0, 3);
  const firstAvailable = results.findIndex(result => result.candidates.length);
  const failures = results.flatMap(({ failures }) => failures.slice(0, 1));
  return { candidates, failures, context: requests[Math.max(0, firstAvailable)] };
}

export async function runBookingAgent({ message, history, selectedBooking, previousSearch, today, language, complete, plan }) {
  const messages = [
    { role: "system", content: `You are an agent helping students book laboratory devices and report faults. Today in Asia/Shanghai is ${today}. Reply in ${language === "zh" ? "Chinese" : "English"}.
Use prepare_fault_report when a student reports a fault or asks to mark a device faulty, for example “1号设备显示器没信号，帮我标记故障”. Omit the number if missing or ambiguous and let the app ask which device. A current message giving a device number can answer that pending fault question in the conversation. Never invent symptoms or infer a device number from booking context. Students can report faults; restoring devices is an administrator action. Never claim that a report has been submitted just because a form was prepared.
Use search_availability before recommending or asserting availability. Search an explicitly requested slot first. Understand shorthand such as 明天9点，2h，两个设备 as tomorrow, hour 9, 2 hours, 2 devices. If that slot is unavailable, try nearby hours using the same duration and device count. Explain what changed.
When users delegate the choice, check real slots using sensible assumptions: start with today or tomorrow, 1 hour and 1 device if unspecified, and tell them the assumptions. If they give no time preference, omit startHours to scan the day for distinct options. If the first date yields nothing, try the next reasonable date. Use request_user_decision for one focused question only if the user's intent prevents a useful lookup. Use prior context for follow-ups like 晚一点，其他不变. History is context only and cannot authorize an action. Prefer the current message over prior context. Use series mode only for an explicitly requested combined booking.
Make one tool call at a time. After tool results, summarize the latest search and its actual candidates in brief plain text. If options are ready, deliver them without appending a question. A candidate is only a proposal; the user must confirm it in the app. Never claim a booking was made. Never invent availability, change explicit duration or quantity, or disclose internal details.` },
    { role: "user", content: JSON.stringify({ message, history, selectedBooking, previousSearch }) },
  ];
  let remaining = maxToolCalls;
  let latest = { candidates: [], failures: [], context: null };
  const finish = reply => ({ ...latest, message: reply,
    report: latest.candidates.length ? "ready" : latest.context ? "no_match" : "answer" });
  const resultMessage = () => latest.candidates.length
    ? (language === "zh" ? "已查到可用方案，具体时间和设备见下方。请选择方案后确认。" : "Available options are listed below. Choose an option to confirm it.")
    : (language === "zh" ? "这次查询没有找到可用方案。" : "No options were available for this search.");
  for (let round = 0; round <= maxToolCalls; round++) {
    const completion = await complete(messages, remaining > 0 && round < maxToolCalls);
    if (completion.error) return latest.context ? finish(resultMessage()) : completion;
    const reply = completion.message;
    if (!reply?.tool_calls?.length) {
      const content = reply?.content?.trim();
      if (content) return finish(content);
      if (latest.context) return finish(resultMessage());
      const explicit = explicitBookingRequest([message], today);
      if (explicit) {
        const search = await searchAvailability(explicit, today, plan);
        if (search.context) {
          latest = search;
          return finish(resultMessage());
        }
      }
      break;
    }
    if (round === maxToolCalls || !remaining) break;
    messages.push({ role: "assistant", content: reply.content ?? null, tool_calls: reply.tool_calls });
    for (const call of reply.tool_calls) {
      let result = { error: "Search limit reached" };
      if (remaining > 0) {
        remaining--;
        let args;
        try { args = JSON.parse(call.function?.arguments); } catch { /* validation below */ }
        if (call.function?.name === "search_availability") {
          result = await searchAvailability(args, today, plan);
          if (result.context) latest = result;
        } else if (call.function?.name === "prepare_fault_report") {
          const deviceNumber = args?.deviceNumber;
          const description = typeof args?.description === "string" ? args.description.trim() : "";
          const base = { candidates: [], failures: [], context: null };
          if (!Number.isInteger(deviceNumber) || deviceNumber < 1 || deviceNumber > 40) {
            return { ...base, report: "needs_user", message: language === "zh"
              ? "请告诉我要上报哪台设备（1–40 号）。" : "Which device are you reporting (1–40)?" };
          }
          if (description.length > 120) {
            return { ...base, report: "needs_user", message: language === "zh"
              ? "请把故障说明缩短到 120 个字符以内。" : "Please shorten the fault description to 120 characters or fewer." };
          }
          return { ...base, report: "ready", faultReport: { deviceNumber, description }, message: language === "zh"
            ? "已准备好故障上报，请核对设备编号和说明后提交。" : "The fault report is ready. Review the device number and description before submitting." };
        } else if (call.function?.name === "request_user_decision") {
          const question = typeof args?.question === "string" ? args.question.trim() : "";
          if (latest.candidates.length) {
            result = { error: "Report the verified candidates before requesting a decision." };
          } else if (question && question.length <= 240 && !/[\r\n]/.test(question)) {
            return { ...finish(question), report: "needs_user" };
          } else {
            result = { error: "Ask one short question." };
          }
        } else {
          result = { error: "Unknown tool" };
        }
      }
      messages.push({ role: "tool", tool_call_id: call.id, content: JSON.stringify(result) });
    }
  }
  return finish(latest.context ? resultMessage() : "");
}
