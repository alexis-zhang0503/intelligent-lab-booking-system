#include "LabRepository.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTime>
#include <QUuid>

namespace {

/** @brief Converts an enum role into the stable JSON representation. */
QString roleText(const UserRole role)
{
    return role == UserRole::Administrator ? QStringLiteral("admin") : QStringLiteral("student");
}

/** @brief Converts persisted role text into a safe application role. */
UserRole roleFromText(const QString &role)
{
    return role == QStringLiteral("admin") ? UserRole::Administrator : UserRole::Student;
}

/** @brief Creates a portable identifier for local demonstration records. */
QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace

LabRepository::LabRepository() = default;

void LabRepository::load()
{
    m_users.clear();
    m_devices.clear();
    m_bookings.clear();
    m_issues.clear();

    QFile input(storagePath());
    if (input.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(input.readAll()).object();
        for (const QJsonValue &value : root.value(QStringLiteral("users")).toArray()) {
            const QJsonObject object = value.toObject();
            m_users.append({object.value(QStringLiteral("id")).toString(), object.value(QStringLiteral("name")).toString(),
                            object.value(QStringLiteral("loginId")).toString(), object.value(QStringLiteral("password")).toString(),
                            roleFromText(object.value(QStringLiteral("role")).toString())});
        }
        for (const QJsonValue &value : root.value(QStringLiteral("devices")).toArray()) {
            const QJsonObject object = value.toObject();
            m_devices.append({object.value(QStringLiteral("number")).toInt(), object.value(QStringLiteral("faulty")).toBool()});
        }
        for (const QJsonValue &value : root.value(QStringLiteral("bookings")).toArray()) {
            const QJsonObject object = value.toObject();
            m_bookings.append({object.value(QStringLiteral("id")).toString(), object.value(QStringLiteral("deviceNumber")).toInt(),
                               QDate::fromString(object.value(QStringLiteral("date")).toString(), Qt::ISODate),
                               object.value(QStringLiteral("startHour")).toInt(), object.value(QStringLiteral("userId")).toString(),
                               object.value(QStringLiteral("cancelled")).toBool(),
                               QDateTime::fromString(object.value(QStringLiteral("checkedInAt")).toString(), Qt::ISODateWithMs),
                               QDateTime::fromString(object.value(QStringLiteral("checkedOutAt")).toString(), Qt::ISODateWithMs)});
        }
        for (const QJsonValue &value : root.value(QStringLiteral("issues")).toArray()) {
            const QJsonObject object = value.toObject();
            m_issues.append({object.value(QStringLiteral("id")).toString(), object.value(QStringLiteral("deviceNumber")).toInt(),
                             object.value(QStringLiteral("reporterId")).toString(), object.value(QStringLiteral("description")).toString(),
                             object.value(QStringLiteral("resolved")).toBool()});
        }
    }

    if (m_devices.size() != 40) {
        m_devices.clear();
        for (int number = 1; number <= 40; ++number) {
            m_devices.append({number, false});
        }
    }

    // A local administrator is seeded only when an operator supplies its password at launch.
    const QString demoAdminPassword = qEnvironmentVariable("LAB_DEMO_ADMIN_PASSWORD");
    if (m_users.isEmpty() && !demoAdminPassword.isEmpty()) {
        m_users.append({newId(), QStringLiteral("实验室管理员"), QStringLiteral("admin"),
                        demoAdminPassword, UserRole::Administrator});
        save(nullptr);
    }
}

const User *LabRepository::currentUser() const
{
    for (const User &candidate : m_users) {
        if (candidate.id == m_sessionUserId) {
            return &candidate;
        }
    }
    return nullptr;
}

bool LabRepository::isAdministrator() const
{
    const User *sessionUser = currentUser();
    return sessionUser != nullptr && sessionUser->role == UserRole::Administrator;
}

const QList<Device> &LabRepository::devices() const { return m_devices; }
const QList<User> &LabRepository::users() const { return m_users; }
const QList<Booking> &LabRepository::bookings() const { return m_bookings; }
const QList<IssueReport> &LabRepository::issues() const { return m_issues; }

bool LabRepository::registerStudent(const QString &name, const QString &loginId, const QString &password, QString *message)
{
    const QString trimmedName = name.trimmed();
    const QString trimmedId = loginId.trimmed();
    if (trimmedName.isEmpty() || trimmedId.isEmpty() || password.size() < 4) {
        if (message) *message = QStringLiteral("请输入姓名、学号和至少 4 位的密码。");
        return false;
    }
    for (const User &candidate : m_users) {
        if (candidate.loginId.compare(trimmedId, Qt::CaseInsensitive) == 0) {
            if (message) *message = QStringLiteral("该学号已注册。");
            return false;
        }
    }
    const User student{newId(), trimmedName, trimmedId, password, UserRole::Student};
    m_users.append(student);
    m_sessionUserId = student.id;
    if (!save(message)) return false;
    if (message) *message = QStringLiteral("学生账号已创建。");
    return true;
}

bool LabRepository::login(const QString &loginId, const QString &password, QString *message)
{
    for (const User &candidate : m_users) {
        if (candidate.loginId == loginId.trimmed() && candidate.password == password) {
            m_sessionUserId = candidate.id;
            if (message) *message = QStringLiteral("登录成功。");
            return true;
        }
    }
    if (message) *message = QStringLiteral("账号或密码不正确。");
    return false;
}

void LabRepository::logout() { m_sessionUserId.clear(); }

bool LabRepository::isActive(const Booking &booking) { return !booking.cancelled; }

bool LabRepository::isUnavailable(const int deviceNumber, const QDate &date, const int startHour) const
{
    for (const Device &device : m_devices) {
        if (device.number == deviceNumber && device.faulty) return true;
    }
    for (const Booking &booking : m_bookings) {
        const bool ended = booking.date < labNow().date()
            || (booking.date == labNow().date() && labNow().time() >= QTime(booking.startHour + 1, 0));
        if (isActive(booking) && !ended && booking.deviceNumber == deviceNumber && booking.date == date && booking.startHour == startHour) return true;
    }
    return false;
}

bool LabRepository::reserve(const QList<int> &deviceNumbers, const QDate &date, const int startHour, QString *message)
{
    const User *sessionUser = currentUser();
    if (!sessionUser || sessionUser->role != UserRole::Student) {
        if (message) *message = QStringLiteral("请使用学生账号登录后预约。");
        return false;
    }
    const QTime now = labNow().time();
    const bool startedToday = date == labNow().date() && startHour <= now.hour();
    if (deviceNumbers.isEmpty() || date < labNow().date() || startedToday || startHour < 8 || startHour > 16) {
        if (message) *message = QStringLiteral("请选择尚未开始的日期和时段，并至少选择一台空闲设备。");
        return false;
    }
    for (const int deviceNumber : deviceNumbers) {
        if (isUnavailable(deviceNumber, date, startHour)) {
            if (message) *message = QStringLiteral("所选设备存在故障或已被预约，请重新选择。");
            return false;
        }
    }
    for (const int deviceNumber : deviceNumbers) {
        m_bookings.append({newId(), deviceNumber, date, startHour, sessionUser->id, false});
    }
    if (!save(message)) return false;
    if (message) *message = QStringLiteral("预约成功，可在“我的”中查看和签到。");
    return true;
}

bool LabRepository::cancel(const QString &bookingId, QString *message)
{
    const User *sessionUser = currentUser();
    for (Booking &booking : m_bookings) {
        if (booking.id == bookingId && sessionUser && booking.userId == sessionUser->id && isActive(booking) && !booking.checkedInAt.isValid() && QDateTime::currentDateTime() < bookingStart(booking)) {
            booking.cancelled = true;
            if (!save(message)) { booking.cancelled = false; return false; }
            if (message) *message = QStringLiteral("预约已取消，设备时段已释放。");
            return true;
        }
    }
    if (message) *message = QStringLiteral("只能取消本人尚未开始且未签到的预约。");
    return false;
}

bool LabRepository::checkIn(const QString &bookingId, QString *message, const QDateTime &now)
{
    const User *user = currentUser();
    for (Booking &booking : m_bookings) {
        if (booking.id != bookingId || !user || booking.userId != user->id) continue;
        if (booking.checkedInAt.isValid() && !booking.checkedOutAt.isValid() && !booking.cancelled) {
            if (message) *message = QStringLiteral("该预约已签到。");
            return true;
        }
        if (!canCheckIn(booking, now)) {
            if (message) *message = QStringLiteral("请在预约时段内签到（北京时间）。");
            return false;
        }
        const Booking before = booking;
        booking.checkedInAt = now.toUTC();
        if (!save(message)) { booking = before; return false; }
        if (message) *message = QStringLiteral("签到成功。");
        return true;
    }
    if (message) *message = QStringLiteral("请选择属于你的预约。");
    return false;
}

bool LabRepository::checkOut(const QString &bookingId, QString *message, const QDateTime &now)
{
    const User *user = currentUser();
    for (Booking &booking : m_bookings) {
        if (booking.id != bookingId || !user || booking.userId != user->id || booking.cancelled) continue;
        if (booking.checkedOutAt.isValid()) {
            if (message) *message = QStringLiteral("该预约已签退。");
            return true;
        }
        if (!canCheckOut(booking) || now < booking.checkedInAt) {
            if (message) *message = QStringLiteral("请先签到，再进行签退。");
            return false;
        }
        const Booking before = booking;
        booking.checkedOutAt = now.toUTC();
        if (!save(message)) { booking = before; return false; }
        if (message) *message = QStringLiteral("签退成功，实际使用记录已保存。");
        return true;
    }
    if (message) *message = QStringLiteral("请选择属于你的预约。");
    return false;
}

Device *LabRepository::deviceByNumber(const int deviceNumber)
{
    for (Device &device : m_devices) {
        if (device.number == deviceNumber) return &device;
    }
    return nullptr;
}

bool LabRepository::setFault(const int deviceNumber, const bool faulty, QString *message)
{
    if (!isAdministrator()) {
        if (message) *message = QStringLiteral("此操作需要管理员权限。");
        return false;
    }
    Device *device = deviceByNumber(deviceNumber);
    if (!device) {
        if (message) *message = QStringLiteral("设备编号不存在。");
        return false;
    }
    device->faulty = faulty;
    if (!save(message)) return false;
    if (message) *message = faulty ? QStringLiteral("设备已标记为故障。") : QStringLiteral("设备已恢复可用。");
    return true;
}

bool LabRepository::deactivateStudent(const QString &userId, QString *message)
{
    if (!isAdministrator()) {
        if (message) *message = QStringLiteral("此操作需要管理员权限。");
        return false;
    }
    for (qsizetype index = 0; index < m_users.size(); ++index) {
        if (m_users.at(index).id == userId && m_users.at(index).role == UserRole::Student) {
            m_users.removeAt(index);
            for (qsizetype bookingIndex = m_bookings.size() - 1; bookingIndex >= 0; --bookingIndex) {
                if (m_bookings.at(bookingIndex).userId == userId) m_bookings.removeAt(bookingIndex);
            }
            if (!save(message)) return false;
            if (message) *message = QStringLiteral("学生账号已注销。");
            return true;
        }
    }
    if (message) *message = QStringLiteral("请选择有效的学生账号。");
    return false;
}

bool LabRepository::reportIssue(const int deviceNumber, const QString &description, QString *message)
{
    const User *sessionUser = currentUser();
    if (!sessionUser || description.trimmed().isEmpty()) {
        if (message) *message = QStringLiteral("请填写故障现象后提交。");
        return false;
    }
    m_issues.append({newId(), deviceNumber, sessionUser->id, description.trimmed(), false});
    if (!save(message)) return false;
    if (message) *message = QStringLiteral("故障已上报，可在故障记录中查看处理进度。");
    return true;
}

bool LabRepository::resolveIssue(const QString &issueId, QString *message)
{
    if (!isAdministrator()) {
        if (message) *message = QStringLiteral("此操作需要管理员权限。");
        return false;
    }
    for (IssueReport &issue : m_issues) {
        if (issue.id == issueId && !issue.resolved) {
            issue.resolved = true;
            if (Device *device = deviceByNumber(issue.deviceNumber)) device->faulty = false;
            if (!save(message)) return false;
            if (message) *message = QStringLiteral("故障已处理，设备已恢复可用。");
            return true;
        }
    }
    if (message) *message = QStringLiteral("请选择一条待处理的故障记录。");
    return false;
}

QList<Booking> LabRepository::bookingsForUser(const QString &userId) const
{
    QList<Booking> result;
    for (const Booking &booking : m_bookings) if (booking.userId == userId && isActive(booking)) result.append(booking);
    return result;
}

int LabRepository::bookedHoursForUser(const QString &userId) const
{
    int completedHours = 0;
    for (const Booking &booking : bookingsForUser(userId)) {
        const bool completed = booking.date < labNow().date()
            || (booking.date == labNow().date() && labNow().time() >= QTime(booking.startHour + 1, 0));
        if (completed) ++completedHours;
    }
    return completedHours;
}

int LabRepository::currentLabUsers() const
{
    const int currentHour = labNow().time().hour();
    const QDate currentDate = labNow().date();
    int count = 0;
    for (const Booking &booking : m_bookings) {
        if (isActive(booking) && canCheckOut(booking) && booking.date == currentDate && booking.startHour == currentHour) ++count;
    }
    return count;
}

int LabRepository::currentWeekReservations() const
{
    const QDate today = labNow().date();
    const QDate monday = today.addDays(1 - today.dayOfWeek());
    const QDate sunday = monday.addDays(6);
    int total = 0;
    for (const Booking &booking : m_bookings) {
        if (isActive(booking) && booking.date >= monday && booking.date <= sunday) ++total;
    }
    return total;
}

bool LabRepository::exportCountFile(const QString &path, const QDate &date, QString *message) const
{
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (message) *message = QStringLiteral("无法打开使用统计文件。");
        return false;
    }
    QString report = QStringLiteral("Intelligent Lab Reservation System - Daily Usage\nDate: %1\n\n").arg(date.toString(Qt::ISODate));
    for (int hour = 8; hour <= 16; ++hour) {
        int count = 0;
        for (const Booking &booking : m_bookings) if (isActive(booking) && booking.date == date && booking.startHour == hour) ++count;
        report += QStringLiteral("%1: %2 reservation(s)\n").arg(hourlyLabel(hour)).arg(count);
    }
    output.write(report.toUtf8());
    if (!output.commit()) {
        if (message) *message = QStringLiteral("无法保存使用统计文件。");
        return false;
    }
    if (message) *message = QStringLiteral("count.txt 已导出。");
    return true;
}

bool LabRepository::exportErrorFile(const QString &path, QString *message) const
{
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (message) *message = QStringLiteral("无法打开故障报告文件。");
        return false;
    }
    QString report = QStringLiteral("Intelligent Lab Reservation System - Device Issues\n\n");
    for (const IssueReport &issue : m_issues) {
        report += QStringLiteral("Device %1 | %2 | %3\n")
                      .arg(issue.deviceNumber, 2, 10, QLatin1Char('0'))
                      .arg(issue.resolved ? QStringLiteral("resolved") : QStringLiteral("open"))
                      .arg(issue.description);
    }
    output.write(report.toUtf8());
    if (!output.commit()) {
        if (message) *message = QStringLiteral("无法保存故障报告文件。");
        return false;
    }
    if (message) *message = QStringLiteral("error.txt 已导出。");
    return true;
}

QString LabRepository::storagePath() const
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    return directory + QStringLiteral("/lab-demo.json");
}

bool LabRepository::save(QString *message) const
{
    const QString path = storagePath();
    QLockFile lock(path + QStringLiteral(".lock"));
    lock.setStaleLockTime(3000);
    if (!lock.tryLock(500)) {
        if (message) *message = QStringLiteral("本地数据正在更新，请稍后重试。");
        return false;
    }

    QJsonArray userArray;
    for (const User &user : m_users) userArray.append(QJsonObject{{"id", user.id}, {"name", user.name}, {"loginId", user.loginId}, {"password", user.password}, {"role", roleText(user.role)}});
    QJsonArray deviceArray;
    for (const Device &device : m_devices) deviceArray.append(QJsonObject{{"number", device.number}, {"faulty", device.faulty}});
    QJsonArray bookingArray;
    for (const Booking &booking : m_bookings) bookingArray.append(QJsonObject{{"id", booking.id}, {"deviceNumber", booking.deviceNumber}, {"date", booking.date.toString(Qt::ISODate)}, {"startHour", booking.startHour}, {"userId", booking.userId}, {"cancelled", booking.cancelled}, {"checkedInAt", booking.checkedInAt.toUTC().toString(Qt::ISODateWithMs)}, {"checkedOutAt", booking.checkedOutAt.toUTC().toString(Qt::ISODateWithMs)}});
    QJsonArray issueArray;
    for (const IssueReport &issue : m_issues) issueArray.append(QJsonObject{{"id", issue.id}, {"deviceNumber", issue.deviceNumber}, {"reporterId", issue.reporterId}, {"description", issue.description}, {"resolved", issue.resolved}});

    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(QJsonObject{{"users", userArray}, {"devices", deviceArray}, {"bookings", bookingArray}, {"issues", issueArray}}).toJson()) < 0 || !output.commit()) {
        if (message) *message = QStringLiteral("无法保存本地演示数据。");
        return false;
    }
    return true;
}
