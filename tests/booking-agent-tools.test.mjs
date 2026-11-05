import test from "node:test";
import assert from "node:assert/strict";
import { runBookingAgent } from "../backend/supabase/functions/deepseek-booking-agent/booking-agent-tools.mjs";

const today = "2026-11-05";
const tomorrow = "2026-11-06";
const search = (id, args, name = "search_availability") => ({ role: "assistant", tool_calls: [
  { id, type: "function", function: { name, arguments: JSON.stringify(args) } },
] });
const reply = content => ({ role: "assistant", content });
const candidate = request => ({
  allocation_strategy: "fixed_device",
  items: [{ reservation_date: request.date, start_hour: request.startHours[0], device_numbers: [1] }],
});

function fixture(replies, unavailableHours = [], duplicateAllocation = false) {
  const queries = [];
  const modelInputs = [];
  const run = options => runBookingAgent({
    today, language: "zh", history: [],
    ...options,
    complete: async (messages, toolsEnabled) => {
      modelInputs.push({ messages: structuredClone(messages), toolsEnabled });
      assert.ok(replies.length, "unexpected model call");
      return { message: replies.shift() };
    },
    plan: async request => {
      queries.push(request);
      return unavailableHours.includes(request.startHours[0])
        ? { candidates: [], failures: [{ reason: "insufficient_available_devices" }] }
        : { candidates: duplicateAllocation
          ? [candidate(request), { ...candidate(request), allocation_strategy: "rotation" }]
          : [candidate(request)], failures: [] };
    },
  });
  return { run, queries, modelInputs };
}

test("a delegated recommendation searches real slots before answering", async () => {
  const f = fixture([search("a", { date: tomorrow, durationHours: 1, requiredDeviceCount: 1 }), reply("有可用时段。")]);
  const result = await f.run({ message: "我也不知道，你帮我推荐一下" });
  assert.ok(f.queries.length > 1);
  assert.ok(f.queries.every(query => query.startHours.length === 1));
  assert.equal(result.candidates.length, 3);
  const toolResult = f.modelInputs[1].messages.at(-1);
  assert.equal(toolResult.tool_call_id, "a");
  assert.deepEqual(JSON.parse(toolResult.content).candidates, result.candidates);
});

test("the model can react to a failed exact search and check neighboring times", async () => {
  const request = { date: tomorrow, startHours: [9], durationHours: 2, requiredDeviceCount: 2 };
  const f = fixture([search("a", request), search("b", { ...request, startHours: [10, 11] }), reply("9点不可用，可以选10点或11点。")], [9]);
  const result = await f.run({ message: "明天9点，2h，两个设备" });
  assert.deepEqual(f.queries.map(q => q.startHours), [[9], [10], [11]]);
  assert.deepEqual(result.candidates.map(c => c.items[0].start_hour), [10, 11]);
  assert.equal(result.context.requiredDeviceCount, 2);
});

test("follow-up receives the session context and only changes the requested time", async () => {
  const previousSearch = { date: tomorrow, startHours: [9], durationHours: 2, requiredDeviceCount: 2 };
  const f = fixture([search("a", { ...previousSearch, startHours: [10] }), reply("改到10点。")]);
  await f.run({ message: "晚一点，其他不变", previousSearch });
  assert.deepEqual(JSON.parse(f.modelInputs[0].messages.at(-1).content).previousSearch, previousSearch);
  assert.deepEqual(f.queries, [{ ...previousSearch, startHours: [10], preferredDevices: [] }]);
});

test("only the allowlisted search can run and explicit series stays a single plan", async () => {
  const series = { startDate: tomorrow, endDate: "2026-11-07", startHours: [9, 12], durationHours: 2, requiredDeviceCount: 2 };
  const f = fixture([search("a", series, "v3_create_reservation_plan"), search("b", { ...series, mode: "series" }), reply("已找到系列方案。")]);
  await f.run({ message: "两天都要，两个时段" });
  assert.deepEqual(f.queries, [{ ...series, preferredDevices: [] }]);
  assert.equal(JSON.parse(f.modelInputs[1].messages.at(-1).content).error, "Unknown tool");
});

test("invalid dates cannot reach the database and an empty reply retains exact shorthand support", async () => {
  const f = fixture([search("a", { date: "2026-11-31", durationHours: 2, requiredDeviceCount: 2 }), reply("")]);
  const result = await f.run({ message: "明天9点，2h，两个设备" });
  assert.deepEqual(f.queries, [{ date: tomorrow, startHours: [9], durationHours: 2, requiredDeviceCount: 2, preferredDevices: [] }]);
  assert.equal(result.candidates.length, 1);
});

test("a blocker is a structured handoff, while verified options are delivered first", async () => {
  const question = "你说的是设备预约，还是要报修？";
  const ask = () => search("ask", { question }, "request_user_decision");
  const blocked = fixture([ask()]);
  const handoff = await blocked.run({ message: "帮我处理一下" });
  assert.equal(handoff.report, "needs_user");
  assert.equal(handoff.message, question);
  assert.equal(blocked.queries.length, 0);
  const ready = fixture([search("a", { date: tomorrow, startHours: [9], durationHours: 1, requiredDeviceCount: 1 }), ask(), reply("找到可用方案。")]);
  const result = await ready.run({ message: "帮我查一下明天9点" });
  assert.equal(result.report, "ready");
  assert.equal(result.candidates.length, 1);
});

test("identical device assignments are offered as a single option", async () => {
  const f = fixture([search("a", { date: tomorrow, startHours: [9], durationHours: 1, requiredDeviceCount: 1 }), reply("有一个方案。")], [], true);
  const result = await f.run({ message: "明天9点约一台设备" });
  assert.equal(result.candidates.length, 1);
});
