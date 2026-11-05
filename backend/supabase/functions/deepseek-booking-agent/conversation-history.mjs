export function conversationHistory(value) {
  if (!Array.isArray(value)) return [];
  return value
    .filter(item => item && (item.role === "user" || item.role === "assistant") && typeof item.message === "string" && item.message.trim())
    .slice(-8)
    .map(item => ({ role: item.role, message: item.message.trim().slice(0, 500) }));
}
