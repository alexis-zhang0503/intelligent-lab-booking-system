#pragma once

#include "domain/Types.h"

#include <QList>
#include <QString>

/**
 * @brief Owns local demo data and all assignment-level reservation rules.
 *
 * The repository is intentionally independent of widgets. This keeps the Qt
 * interface replaceable and establishes the contract that a future network
 * repository will implement against the shared Supabase schema.
 */
class LabRepository final
{
public:
    /** @brief Creates a repository backed by a JSON file in the app-data directory. */
    LabRepository();

    /** @brief Loads stored data, seeds forty devices, and creates a demo administrator if needed. */
    void load();

    /** @brief Returns the currently logged-in user or nullptr when signed out. */
    const User *currentUser() const;

    /** @brief Returns whether the current session belongs to an administrator. */
    bool isAdministrator() const;

    /** @brief Exposes all device states for rendering the forty-seat plan. */
    const QList<Device> &devices() const;

    /** @brief Exposes all accounts for the administrator's user table. */
    const QList<User> &users() const;

    /** @brief Exposes all reservation records for dashboards and exports. */
    const QList<Booking> &bookings() const;

    /** @brief Exposes submitted issue reports for administrator maintenance. */
    const QList<IssueReport> &issues() const;

    /**
     * @brief Registers and signs in a public student account.
     * @param name Non-empty visible name.
     * @param loginId Unique student identifier.
     * @param password Demo password with at least four characters.
     * @param message Receives a precise success or validation message.
     * @return True only when the student was stored successfully.
     */
    bool registerStudent(const QString &name, const QString &loginId, const QString &password, QString *message);

    /**
     * @brief Validates credentials and starts a local session.
     * @param loginId Account identifier entered by the user.
     * @param password Password entered by the user.
     * @param message Receives a precise success or failure message.
     * @return True only when credentials matched an existing account.
     */
    bool login(const QString &loginId, const QString &password, QString *message);

    /** @brief Ends the local session without deleting stored records. */
    void logout();

    /**
     * @brief Returns whether a device is unavailable at a requested date and hour.
     * @param deviceNumber Physical device number.
     * @param date Requested calendar date.
     * @param startHour Requested one-hour start value.
     * @return True for a faulty computer or an active conflicting booking.
     */
    bool isUnavailable(int deviceNumber, const QDate &date, int startHour) const;

    /**
     * @brief Reserves all requested computers for one hourly slot.
     * @param deviceNumbers Selected computer numbers.
     * @param date Requested date, which cannot be in the past.
     * @param startHour One integer from 8 through 16.
     * @param message Receives a validation or success result.
     * @return True only when every requested device was reserved.
     */
    bool reserve(const QList<int> &deviceNumbers, const QDate &date, int startHour, QString *message);

    /**
     * @brief Cancels an owned reservation and releases its device-hour pair.
     * @param bookingId Reservation identifier chosen in the student's list.
     * @param message Receives the operation result.
     * @return True only when the current student owned the record.
     */
    bool cancel(const QString &bookingId, QString *message);

    /** Records local attendance only during the booked hour; repeated requests are idempotent. */
    bool checkIn(const QString &bookingId, QString *message, const QDateTime &now = QDateTime::currentDateTime());
    /** Completes an owned checked-in booking and persists the checkout timestamp. */
    bool checkOut(const QString &bookingId, QString *message, const QDateTime &now = QDateTime::currentDateTime());

    /**
     * @brief Changes one device between working and faulty states.
     * @param deviceNumber Physical device number.
     * @param faulty Desired persistent fault state.
     * @param message Receives the operation result.
     * @return True only for an authenticated administrator.
     */
    bool setFault(int deviceNumber, bool faulty, QString *message);

    /**
     * @brief Deactivates a student and removes that student's reservations.
     * @param userId Identifier selected by the administrator.
     * @param message Receives the operation result.
     * @return True only when an administrator removes an existing student.
     */
    bool deactivateStudent(const QString &userId, QString *message);

    /**
     * @brief Stores a fault report for later administrator maintenance.
     * @param deviceNumber Affected computer number.
     * @param description User-visible issue description.
     * @param message Receives the operation result.
     * @return True when a signed-in user supplied a non-empty description.
     */
    bool reportIssue(int deviceNumber, const QString &description, QString *message);

    /**
     * @brief Marks an issue as resolved and returns its device to available status.
     * @param issueId Issue identifier selected by the administrator.
     * @param message Receives the operation result.
     * @return True only when an administrator resolves an existing open report.
     */
    bool resolveIssue(const QString &issueId, QString *message);

    /** @brief Returns active reservations for a student, sorted by insertion order. */
    QList<Booking> bookingsForUser(const QString &userId) const;

    /** @brief Calculates completed experiment hours for the supplied student. */
    int bookedHoursForUser(const QString &userId) const;

    /** @brief Counts active reservations that cover the current local hour. */
    int currentLabUsers() const;

    /** @brief Counts active reservations in the current Monday-through-Sunday week. */
    int currentWeekReservations() const;

    /** @brief Writes the assignment-required daily usage report to a user-selected path. */
    bool exportCountFile(const QString &path, const QDate &date, QString *message) const;

    /** @brief Writes all fault reports to the assignment-required error file path. */
    bool exportErrorFile(const QString &path, QString *message) const;

private:
    /** @brief Persists current lists after acquiring a short file lock. */
    bool save(QString *message) const;
    /** @brief Builds the storage path without exposing file-system details to widgets. */
    QString storagePath() const;
    /** @brief Returns a mutable device pointer or nullptr for an invalid number. */
    Device *deviceByNumber(int deviceNumber);
    /** @brief Checks whether a booking is still active for seat-conflict purposes. */
    static bool isActive(const Booking &booking);

    QList<User> m_users;            ///< Persisted student and administrator accounts.
    QList<Device> m_devices;        ///< Exactly forty physical computers.
    QList<Booking> m_bookings;      ///< One-hour reservations, including cancellations.
    QList<IssueReport> m_issues;    ///< Fault reports retained for administrator review.
    QString m_sessionUserId;        ///< Identifier of the active local session, if any.
};
