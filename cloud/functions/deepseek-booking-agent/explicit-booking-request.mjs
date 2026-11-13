function addDays(date, days) {
  const value = new Date(`${date}T00:00:00Z`);
  value.setUTCDate(value.getUTCDate() + days);
  return value.toISOString().slice(0, 10);
}

function monthDayDate(text, today) {
  const match = text.match(/(?:\b(20\d{2})\s*[-年]\s*)?(\d{1,2})月(\d{1,2})日?/);
  if (!match) return null;
  const year = Number(match[1] ?? today.slice(0, 4));
  const month = Number(match[2]);
  const day = Number(match[3]);
  const value = `${year}-${String(month).padStart(2, "0")}-${String(day).padStart(2, "0")}`;
  const date = new Date(`${value}T00:00:00Z`);
  return !Number.isNaN(date.getTime()) && date.toISOString().slice(0, 10) === value ? value : null;
}

export function explicitBookingRequest(messages, today) {
  const text = Array.isArray(messages) ? messages.filter(value => typeof value === "string").join(" ") : "";
  const date = text.match(/\b(20\d{2}-\d{2}-\d{2})\b/)?.[1] ?? monthDayDate(text, today) ?? (/(?:明天|tomorrow)/i.test(text) ? addDays(today, 1) : null);
  const startHour = Number(text.match(/\b([01]?\d|2[0-3])\s*(?::|：)\s*\d{2}|\b([01]?\d|2[0-3])\s*点(?:钟)?/)?.slice(1).find(Boolean));
  const durationHours = Number(text.match(/(?:使用|用|for)?\s*(\d+)\s*(?:小时|小時|hours?|h)/i)?.[1]);
  const count = text.match(/(\d+|两|兩|二)\s*(?:台|个|個|臺|devices?)\s*(?:设备|設備)?/i)?.[1];
  const requiredDeviceCount = /^(两|兩|二)$/.test(count) ? 2 : Number(count);
  if (!date || !Number.isInteger(startHour) || !Number.isInteger(durationHours) || !Number.isInteger(requiredDeviceCount)) return null;
  if (startHour < 0 || startHour > 23 || durationHours < 1 || durationHours > 9 || requiredDeviceCount < 1 || requiredDeviceCount > 40) return null;
  return { date, startHours: [startHour], durationHours, requiredDeviceCount, preferredDevices: [] };
}
