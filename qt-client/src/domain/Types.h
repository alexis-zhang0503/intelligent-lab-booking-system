#pragma once

#include <QDate>
#include <QDateTime>
#include <QTimeZone>
#include <QString>

/**
 * @brief Describes the two application roles.
 *
 * Students book equipment and report issues. Administrators maintain devices,
 * users, and exports. The value is persisted as text in the local demo store.
 */
enum class UserRole { Student, Administrator };

/** @brief Stores one authenticated account's display and login information. */
struct User {
    QString id;          ///< Stable local or backend identifier.
    QString name;        ///< Display name shown in the desktop interface.
    QString loginId;     ///< Student ID, staff ID, or administrator login name.
    QString password;    ///< Demo-only password; production uses remote authentication.
    UserRole role;       ///< Access level for this account.
};

/** @brief Stores the permanent condition of one of the forty lab devices. */
struct Device {
    int number = 0;      ///< Physical computer number, constrained to 1 through 40.
    bool faulty = false; ///< True when a student must not reserve this computer.
};

/** @brief Represents exactly one assignment-defined one-hour reservation. */
struct Booking {
    QString id;              ///< Stable booking identifier.
    int deviceNumber = 0;    ///< Reserved physical computer number.
    QDate date;              ///< Calendar date selected by the student.
    int startHour = 8;       ///< Start hour from 8 through 16; each booking lasts one hour.
    QString userId;          ///< Identifier of the student who owns this booking.
    bool cancelled = false;  ///< Whether the student cancelled the reservation.
    QDateTime checkedInAt;
    QDateTime checkedOutAt;
};

/** @brief Records a student or administrator report about a device. */
struct IssueReport {
    QString id;              ///< Stable issue identifier.
    int deviceNumber = 0;    ///< Affected computer number.
    QString reporterId;      ///< Account that created this report.
    QString description;     ///< Human-readable fault description.
    bool resolved = false;   ///< True after an administrator restores the device.
};

/**
 * @brief Formats one valid hourly range for visible controls and exports.
 * @param startHour An integer from 8 through 16.
 * @return Text such as "08:00–09:00".
 */
inline QString hourlyLabel(const int startHour)
{
    return QStringLiteral("%1:00–%2:00")
        .arg(startHour, 2, 10, QLatin1Char('0'))
        .arg(startHour + 1, 2, 10, QLatin1Char('0'));
}

/** The lab schedule always uses China Standard Time, even on computers abroad. */
inline QDateTime bookingStart(const Booking &booking)
{
    return QDateTime(booking.date, QTime(booking.startHour, 0), QTimeZone("Asia/Shanghai"));
}
inline bool canCheckIn(const Booking &booking, const QDateTime &now = QDateTime::currentDateTime())
{
    return !booking.cancelled && !booking.checkedInAt.isValid() && !booking.checkedOutAt.isValid()
        && now >= bookingStart(booking) && now < bookingStart(booking).addSecs(3600);
}
inline bool canCheckOut(const Booking &booking)
{
    return !booking.cancelled && booking.checkedInAt.isValid() && !booking.checkedOutAt.isValid();
}

inline QDateTime labNow()
{
    return QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone("Asia/Shanghai"));
}
