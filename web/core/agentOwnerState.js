const emptySessions = () => ({ booking_planner: null, admin_analyst: null, fault_responder: null });

export function switchAgentOwner(state, ownerId) {
  if (state?.ownerId === ownerId) return state;
  return { ownerId: ownerId ?? null, sessions: emptySessions(), conversations: {}, response: null, submitting: false };
}
