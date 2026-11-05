function addDays(date, days) {
  const value = new Date(`${date}T00:00:00Z`);
  value.setUTCDate(value.getUTCDate() + days);
  return value.toISOString().slice(0, 10);
}

export function explicitBookingRequest(messages, today) {
  const text = Array.isArray(messages) ? messages.filter(value => typeof value === "string").join(" ") : "";
  const date = text.match(/\b(20\d{2}-\d{2}-\d{2})\b/)?.[1] ?? (/(?:明天|tomorrow)/i.test(text) ? addDays(today, 1) : null);
  const startHour = Number(text.match(/\b([01]?\d|2[0-3])\s*[:：]\s*\d{2}/)?.[1]);
  const durationHours = Number(text.match(/(?:使用|用|for)?\s*(\d+)\s*(?:小时|小時|hours?|h)/i)?.[1]);
  const requiredDeviceCount = Number(text.match(/(\d+)\s*(?:台|个|臺|devices?)\s*(?:设备|設備)?/i)?.[1]);
  if (!date || !Number.isInteger(startHour) || !Number.isInteger(durationHours) || !Number.isInteger(requiredDeviceCount)) return null;
  if (startHour < 0 || startHour > 23 || durationHours < 1 || durationHours > 9 || requiredDeviceCount < 1 || requiredDeviceCount > 40) return null;
  return { date, startHours: [startHour], durationHours, requiredDeviceCount, preferredDevices: [] };
}
