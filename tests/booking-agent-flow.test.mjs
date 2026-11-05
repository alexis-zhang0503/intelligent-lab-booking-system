import test from "node:test";
import assert from "node:assert/strict";
import { bookingContext, parseBookingDecision, resolveBookingPlanInput } from "../backend/supabase/functions/deepseek-booking-agent/booking-request.mjs";
import { conversationHistory } from "../backend/supabase/functions/deepseek-booking-agent/conversation-history.mjs";

test("a default page state is not treated as a selected booking", () => {
  assert.equal(bookingContext({ date: "2026-11-05", startHours: [8], durationHours: 1, requiredDeviceCount: 1 }), null);
});

test("an incomplete model interpretation cannot reach schedule planning", () => {
  assert.equal(resolveBookingPlanInput({ ready: false, message: "What time tomorrow afternoon and how many devices do you need?" }), null);
});

test("an explicit selected booking is safe to plan", () => {
  const context = bookingContext({ bookingSelection: { date: "2026-11-05", startHours: [14], durationHours: 1, requiredDeviceCount: 2, preferredDevices: [1, 2] } });
  assert.deepEqual(resolveBookingPlanInput({ ready: true, request: context }), context);
});

test("a complete natural-language interpretation is safe to plan", () => {
  assert.deepEqual(resolveBookingPlanInput({ ready: true, request: { date: "2026-11-05", startHours: [14], durationHours: 1, requiredDeviceCount: 2 } }), { date: "2026-11-05", startHours: [14], durationHours: 1, requiredDeviceCount: 2, preferredDevices: [] });
});

test("common model field aliases still become a valid booking plan", () => {
  assert.deepEqual(resolveBookingPlanInput({ ready: true, request: { date: "2026-11-06", startHour: "14", duration: "2", deviceCount: "2" } }), { date: "2026-11-06", startHours: [14], durationHours: 2, requiredDeviceCount: 2, preferredDevices: [] });
});

test("a complete booking request remains valid when the model omits ready", () => {
  assert.deepEqual(resolveBookingPlanInput({ request: { date: "2026-11-06", startHours: [14], durationHours: 2, requiredDeviceCount: 2 } }), { date: "2026-11-06", startHours: [14], durationHours: 2, requiredDeviceCount: 2, preferredDevices: [] });
});

test("a fenced JSON decision is parsed before validation", () => {
  assert.deepEqual(parseBookingDecision('```json\n{"request":{"date":"2026-11-06","startHours":[14],"durationHours":2,"requiredDeviceCount":2}}\n```').request.startHours, [14]);
});

test("follow-up details retain the prior booking request for DeepSeek", () => {
  assert.deepEqual(conversationHistory([
    { role: "user", message: "明天，需要 2 台设备" },
    { role: "assistant", message: "请补充具体开始时间和使用时长。" },
    { role: "user", message: "14:00 开始，用 2 小时" },
  ]), [
    { role: "user", message: "明天，需要 2 台设备" },
    { role: "assistant", message: "请补充具体开始时间和使用时长。" },
    { role: "user", message: "14:00 开始，用 2 小时" },
  ]);
});
