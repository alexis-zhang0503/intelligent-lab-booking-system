export const adminTools = [{
  type: "function",
  function: {
    name: "prepare_lab_action",
    description: "Prepare one explicitly requested administrator action for review. For example, 我已经解决了1号设备的故障，帮我取消标记 means repair_fault on device 1. Exports contain complete records, so do not use this tool for a filtered export unless the user accepts the full export.",
    parameters: {
      type: "object",
      properties: {
        action: { type: "string", enum: ["repair_fault", "export_reservations", "export_issues", "export_all"] },
        deviceNumber: { type: "integer", minimum: 1, maximum: 40, description: "For repair only. Omit if the user did not identify a device." },
      },
      required: ["action"],
      additionalProperties: false,
    },
  },
}];

const exportFiles = { export_reservations: ["reservations"], export_issues: ["issues"], export_all: ["reservations", "issues"] };

export async function prepareAdminAction(calls, { findOpenFaults, prepareRepair }) {
  if (!calls?.length) return null;
  if (calls.length > 2 || calls.some(call => call?.function?.name !== "prepare_lab_action")) return { kind: "invalid" };
  let actions;
  try { actions = calls.map(call => JSON.parse(call.function.arguments)); } catch { return { kind: "invalid" }; }
  if (actions.every(item => typeof item?.action === "string" && Object.hasOwn(exportFiles, item.action))) {
    const requested = new Set(actions.flatMap(item => exportFiles[item.action]));
    return { kind: "export", files: exportFiles.export_all.filter(key => requested.has(key)) };
  }
  if (actions.length !== 1) return { kind: "invalid" };
  const [args] = actions;
  if (args?.action !== "repair_fault") return { kind: "invalid" };
  const { deviceNumber } = args;
  if (!Number.isInteger(deviceNumber) || deviceNumber < 1 || deviceNumber > 40) return { kind: "needs_device" };
  const issues = await findOpenFaults(deviceNumber);
  if (!issues.length) return { kind: "no_fault", deviceNumber };
  if (issues.length > 1) return { kind: "ambiguous", deviceNumber };
  return { kind: "repair", deviceNumber, proposal: await prepareRepair(issues[0].id) };
}
