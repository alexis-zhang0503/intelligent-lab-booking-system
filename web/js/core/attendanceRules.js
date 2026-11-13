// Reservation dates and hours are laboratory wall-clock times, independent of the browser timezone.
export function reservationStart(booking) {
  return Date.parse(`${booking.date}T${String(Number(booking.hour)).padStart(2, '0')}:00:00+08:00`);
}
export function attendanceState(booking, now = Date.now()) {
  if (booking.status === 'in_use') return 'check_out';
  if (!['reserved', 'confirmed'].includes(booking.status)) return 'unavailable';
  const start = reservationStart(booking);
  if (!Number.isFinite(start)) return 'unavailable';
  if (now < start) return 'early';
  if (now >= start + 3600000) return 'expired';
  return 'check_in';
}
export function attendanceError(error, language = 'zh') {
  const text = String(error?.message || '');
  const en = language === 'en';
  const messages = [
    [/PGRST202|42883/.test(error?.code || '') || /could not find.*function|function.*does not exist/i.test(text), '签到服务尚未安装，请管理员执行签到修复 SQL。', 'Attendance service is unavailable. Ask the administrator to apply the attendance SQL migration.'],
    [/42P01|42703/.test(error?.code || '') || /schema cache|does not exist/i.test(text), '签到数据结构尚未升级，请管理员完成 V2 数据库迁移。', 'Attendance schema is unavailable. Ask the administrator to complete the V2 database migration.'],
    [/during the booked hour/i.test(text), '仅可在预约时段内签到（北京时间），请检查预约开始时间。', 'Check in during the booked hour only (China Standard Time).'],
    [/confirmed reservation/i.test(text), '当前预约状态不支持签到，请刷新记录。', 'This reservation cannot be checked in. Refresh your records.'],
    [/Check-in is required/i.test(text), '请先签到，再签退。', 'Check in before checking out.'],
    [/not found/i.test(text), '预约不存在或不属于当前账号，请刷新记录。', 'Reservation unavailable for this account. Refresh your records.'],
    [/authentication|JWT|session/i.test(text) || error?.code === '42501', '登录状态失效或无权操作，请重新登录。', 'Sign in again or check your reservation permissions.'],
    [/fetch|network|timeout|load failed/i.test(text), '网络连接失败，请稍后重试；可刷新记录确认签到状态。', 'Network error. Refresh your records to check whether attendance was recorded.']
  ];
  const found = messages.find(item => item[0]);
  return found ? found[en ? 2 : 1] : (en ? 'Attendance could not be saved. Refresh your records and try again.' : '签到操作未完成，请刷新记录后重试。');
}
