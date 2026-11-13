// Match the shared Supabase password policy before submitting a registration.
export const MIN_PASSWORD_LENGTH = 6;
export function registrationError(name, password, language = 'zh') {
  const en = language === 'en';
  if (!name.trim() || name.trim().length > 16) return en ? 'Enter a name of 1–16 characters.' : '姓名需要填写 1–16 个字符。';
  if (password.length < MIN_PASSWORD_LENGTH) return en ? 'Use a password with at least 6 characters.' : '密码至少需要 6 个字符。';
  return '';
}
export function authErrorMessage(error, language = 'zh') {
  const en = language === 'en';
  const code = error?.code || error?.error_code || '';
  const message = error?.message || '';
  const text = (zh, english) => en ? english : zh;
  if (code === 'weak_password' || /password.*at least|password.*short/i.test(message))
    return text('密码不符合要求，请至少使用 6 个字符，并避免常见或已泄露的密码。', 'Use at least 6 characters and avoid common or compromised passwords.');
  if (['user_already_exists', 'email_exists'].includes(code) || /already.*registered|already.*exists/i.test(message))
    return text('该账号已注册，请直接登录。', 'This account is already registered. Please sign in.');
  if (code === 'invalid_credentials') return text('账号或密码不正确。', 'Incorrect ID or password.');
  if (['over_request_rate_limit', 'over_email_send_rate_limit'].includes(code) || error?.status === 429)
    return text('操作过于频繁，请稍后再试，不要连续提交。', 'Too many attempts. Wait before trying again.');
  if (['signup_disabled', 'email_provider_disabled'].includes(code))
    return text('当前服务未开放注册，请联系项目管理员。', 'Registration is disabled. Contact the project administrator.');
  if (code === 'email_address_invalid') return text('账号格式不正确，请检查学号或邮箱。', 'Check your student ID or email address.');
  if (code === 'unexpected_failure' || /database error saving new user/i.test(message))
    return text('云端创建账号失败，请联系项目管理员检查注册数据库配置。', 'Account creation failed on the server. Contact the project administrator.');
  if (/fetch|network|timeout|load failed/i.test(message) || error?.name === 'AuthRetryableFetchError')
    return text('无法连接云端，请检查网络及代理连接后重试。', 'Cannot reach the server. Check your network and proxy connection.');
  return text('请求未完成，请稍后重试。', 'The request failed. Please try again.');
}
