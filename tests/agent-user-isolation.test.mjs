import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { switchAgentOwner } from "../web/core/agentOwnerState.js";

const ownerA = "11111111-1111-4111-8111-111111111111";
const ownerB = "22222222-2222-4222-8222-222222222222";

test("switching accounts clears in-memory assistant state", () => {
  const prior = {
    ownerId: ownerA,
    sessions: { booking_planner: "session-a", admin_analyst: null, fault_responder: null },
    conversations: { booking_planner: [{ role: "user", message: "A 的预约需求" }] },
    response: { message: "A 的助手回复" },
    submitting: true,
  };

  assert.deepEqual(switchAgentOwner(prior, ownerB), {
    ownerId: ownerB,
    sessions: { booking_planner: null, admin_analyst: null, fault_responder: null },
    conversations: {},
    response: null,
    submitting: false,
  });
});

test("refreshing the same account keeps its in-memory assistant state", () => {
  const state = {
    ownerId: ownerA,
    sessions: { booking_planner: "session-a", admin_analyst: null, fault_responder: null },
    conversations: { booking_planner: [{ role: "user", message: "A 的预约需求" }] },
    response: null,
    submitting: false,
  };

  assert.equal(switchAgentOwner(state, ownerA), state);
});

test("agent records are readable only by their owner", () => {
  const migration = fs.readFileSync(path.resolve("backend/supabase/v3-4-agent-privacy-hardening.sql"), "utf8");
  assert.match(migration, /users read their agent sessions[\s\S]*user_id = auth\.uid\(\)/);
  assert.match(migration, /users read their agent proposals[\s\S]*user_id = auth\.uid\(\)/);
  assert.match(migration, /users read their agent proposal items[\s\S]*p\.user_id = auth\.uid\(\)/);
  assert.match(migration, /users read their agent audit logs[\s\S]*user_id = auth\.uid\(\)/);
  assert.doesNotMatch(migration, /public\.is_admin\(\)/);
});

test("the browser synchronizes assistant state with the signed-in user", () => {
  const app = fs.readFileSync(path.resolve("web/app.js"), "utf8");
  assert.match(app, /syncAgentStateOwner\(profile\.id\)/);
  assert.match(app, /if\(ownerId!==db\.session\)return/);
});
