/** Browser keys for the intentionally local demonstration mode. */
const DATA_KEY = "lab-booking-v2";

/**
 * Loads browser-local student demonstration data.
 * Administrator creation belongs to a protected backend invitation or seed flow;
 * the static website never embeds a usable administrator credential.
 * @returns {{users: Array, bookings: Array, faults: Array, session: string|null}}
 */
export function loadDemoStore() {
  try {
    const parsed = JSON.parse(localStorage.getItem(DATA_KEY) || "{}");
    const state = { users: [], bookings: [], faults: [], session: null, ...parsed };
    state.users = state.users.filter((account) => account.role !== "admin");
    if (state.session && !state.users.some((account) => account.id === state.session)) state.session = null;
    return state;
  } catch {
    return { users: [], bookings: [], faults: [], session: null };
  }
}

/**
 * Persists the browser-only demonstration state.
 * @param {object} state Complete local demonstration data.
 */
export function saveDemoStore(state) {
  localStorage.setItem(DATA_KEY, JSON.stringify(state));
}
