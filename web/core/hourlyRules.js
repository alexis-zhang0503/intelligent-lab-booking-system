/**
 * Assignment-defined start hours. Each value represents one whole hour and
 * produces a single booking ending one hour later.
 */
export const HOURLY_SLOTS = Object.freeze(Array.from({ length: 9 }, (_, index) => 8 + index));

/** Physical computer numbers displayed in the 40-seat laboratory plan. */
export const DEVICE_NUMBERS = Object.freeze(Array.from({ length: 40 }, (_, index) => index + 1));

/**
 * Converts a valid hour into the minute offset used when comparing Date values.
 * @param {number} hour An assignment-defined start hour from 8 through 16.
 * @returns {number} Minutes after midnight.
 */
export function hourToMinutes(hour) {
  return Number(hour) * 60;
}

/**
 * Formats a whole-hour reservation interval for the UI and export files.
 * @param {number} hour An assignment-defined start hour from 8 through 16.
 * @returns {string} A label such as "08:00–09:00".
 */
export function hourlyRange(hour) {
  const start = String(Number(hour)).padStart(2, "0");
  const end = String(Number(hour) + 1).padStart(2, "0");
  return `${start}:00–${end}:00`;
}

/**
 * Checks whether a value belongs to the nine permitted reservation hours.
 * @param {number} hour Candidate reservation start hour.
 * @returns {boolean} True only for integer values from 8 through 16.
 */
export function isHourlySlot(hour) {
  return HOURLY_SLOTS.includes(Number(hour));
}
