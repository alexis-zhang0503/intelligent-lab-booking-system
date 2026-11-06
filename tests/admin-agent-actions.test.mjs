import test from "node:test";
import assert from "node:assert/strict";
import { prepareAdminAction } from "../backend/supabase/functions/deepseek-booking-agent/admin-agent-actions.mjs";

const call = args => [{ function: { name: "prepare_lab_action", arguments: JSON.stringify(args) } }];

test("one open fault creates a confirmation proposal for the actual record", async () => {
  const result = await prepareAdminAction(call({ action: "repair_fault", deviceNumber: 1 }), {
    findOpenFaults: async number => { assert.equal(number, 1); return [{ id: "record-one" }]; },
    prepareRepair: async id => { assert.equal(id, "record-one"); return { proposalId: "confirmation" }; },
  });
  assert.deepEqual(result, { kind: "repair", deviceNumber: 1, proposal: { proposalId: "confirmation" } });
});

test("missing or ambiguous fault records produce no confirmation", async () => {
  const callbacks = { prepareRepair: async () => assert.fail("must not create a proposal") };
  const tool = call({ action: "repair_fault", deviceNumber: 1 });
  assert.equal((await prepareAdminAction(tool, { ...callbacks, findOpenFaults: async () => [] })).kind, "no_fault");
  assert.equal((await prepareAdminAction(tool, { ...callbacks, findOpenFaults: async () => [{ id: 1 }, { id: 2 }] })).kind, "ambiguous");
  assert.equal((await prepareAdminAction(call({ action: "repair_fault", deviceNumber: 99 }), callbacks)).kind, "needs_device");
});

test("exports return allowlisted identifiers and never invoke a write tool", async () => {
  assert.deepEqual(await prepareAdminAction(call({ action: "export_all" }), {}), { kind: "export", files: ["reservations", "issues"] });
  assert.deepEqual(await prepareAdminAction(call({ action: "export_issues" }), {}), { kind: "export", files: ["issues"] });
  assert.deepEqual(await prepareAdminAction(call({ action: "export_reservations" }), {}), { kind: "export", files: ["reservations"] });
  assert.deepEqual(await prepareAdminAction(call({ action: "delete_all" }), {}), { kind: "invalid" });
  assert.equal(await prepareAdminAction([], {}), null);
});
