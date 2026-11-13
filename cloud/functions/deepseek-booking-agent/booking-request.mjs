function record(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function date(value) {
  return typeof value === "string" && /^\d{4}-\d{2}-\d{2}$/.test(value) ? value : null;
}

function integer(value, minimum, maximum) {
  const parsed = typeof value === "string" && /^\d+$/.test(value.trim()) ? Number(value) : value;
  return Number.isInteger(parsed) && parsed >= minimum && parsed <= maximum ? parsed : null;
}

function integers(values, minimum, maximum) {
  if (!Array.isArray(values) || values.length === 0) return null;
  const result = [...new Set(values.map(value => integer(value, minimum, maximum)))];
  return result.includes(null) ? null : result.sort((left, right) => left - right);
}

export function resolveBookingPlanInput(decision) {
  if (!record(decision) || decision.ready === false || !record(decision.request)) return null;
  const request = decision.request;
  const singleDate = date(request.date);
  const startDate = date(request.startDate) ?? singleDate;
  const endDate = date(request.endDate) ?? singleDate;
  const startHours = integers(Array.isArray(request.startHours) ? request.startHours : [request.startHour], 0, 23);
  const durationHours = integer(request.durationHours ?? request.duration, 1, 9);
  const requiredDeviceCount = integer(request.requiredDeviceCount ?? request.deviceCount, 1, 40);
  const preferredDevices = request.preferredDevices == null || (Array.isArray(request.preferredDevices) && request.preferredDevices.length === 0)
    ? [] : integers(request.preferredDevices, 1, 40);
  const weekdays = request.weekdays == null ? undefined : integers(request.weekdays, 0, 6);
  if (!startDate || !endDate || endDate < startDate || !startHours || !durationHours || !requiredDeviceCount || !preferredDevices || (request.weekdays != null && !weekdays)) return null;
  return { ...(singleDate ? { date: singleDate } : { startDate, endDate }), startHours, durationHours, requiredDeviceCount, preferredDevices, ...(weekdays ? { weekdays } : {}) };
}

export function parseBookingDecision(content) {
  if (typeof content !== "string") return null;
  const json = content.trim().replace(/^```(?:json)?\s*/i, "").replace(/\s*```$/, "");
  try { return JSON.parse(json); } catch { return null; }
}

export function bookingContext(input) {
  if (!record(input) || !record(input.bookingSelection)) return null;
  return resolveBookingPlanInput({ ready: true, request: input.bookingSelection });
}
