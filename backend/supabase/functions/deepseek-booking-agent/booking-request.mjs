function record(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function date(value) {
  return typeof value === "string" && /^\d{4}-\d{2}-\d{2}$/.test(value) ? value : null;
}

function integer(value, minimum, maximum) {
  return Number.isInteger(value) && value >= minimum && value <= maximum ? value : null;
}

function integers(values, minimum, maximum) {
  if (!Array.isArray(values) || values.length === 0) return null;
  const result = [...new Set(values.map(value => integer(value, minimum, maximum)))];
  return result.includes(null) ? null : result.sort((left, right) => left - right);
}

export function resolveBookingPlanInput(decision) {
  if (!record(decision) || decision.ready !== true || !record(decision.request)) return null;
  const request = decision.request;
  const singleDate = date(request.date);
  const startDate = date(request.startDate) ?? singleDate;
  const endDate = date(request.endDate) ?? singleDate;
  const startHours = integers(request.startHours, 0, 23);
  const durationHours = integer(request.durationHours, 1, 9);
  const requiredDeviceCount = integer(request.requiredDeviceCount, 1, 40);
  const preferredDevices = request.preferredDevices == null ? [] : integers(request.preferredDevices, 1, 40);
  const weekdays = request.weekdays == null ? undefined : integers(request.weekdays, 0, 6);
  if (!startDate || !endDate || endDate < startDate || !startHours || !durationHours || !requiredDeviceCount || !preferredDevices || (request.weekdays != null && !weekdays)) return null;
  return { ...(singleDate ? { date: singleDate } : { startDate, endDate }), startHours, durationHours, requiredDeviceCount, preferredDevices, ...(weekdays ? { weekdays } : {}) };
}

export function bookingContext(input) {
  if (!record(input) || !record(input.bookingSelection)) return null;
  return resolveBookingPlanInput({ ready: true, request: input.bookingSelection });
}
