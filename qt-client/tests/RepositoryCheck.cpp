#include "services/LabRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

/**
 * @brief Stops the executable with a visible message when a required invariant fails.
 * @param condition The invariant result.
 * @param message The concise reason shown to the test runner.
 * @return Zero when the invariant passes; one otherwise.
 */
int require(const bool condition, const char *message)
{
    if (condition) return 0;
    qCritical("%s", message);
    return 1;
}

/**
 * @brief Exercises the rules that make the C++ client comply with the assignment.
 * @param argc Process argument count supplied by the test runner.
 * @param argv Process arguments supplied by the test runner.
 * @return Zero only when registration, conflicts, faults, and role checks pass.
 */
int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("IntelligentLabReservationQtCheck"));
    QStandardPaths::setTestModeEnabled(true);
    qputenv("LAB_DEMO_ADMIN_PASSWORD", "test-admin-password");
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();

    LabRepository repository;
    repository.load();
    QString message;
    if (require(repository.users().size() == 1 && repository.isAdministrator() == false, "Expected one seeded administrator without a session.")) return 1;
    if (require(repository.registerStudent(QStringLiteral("Student One"), QStringLiteral("B25012413"), QStringLiteral("pass1234"), &message), "Student registration failed.")) return 1;
    if (require(!repository.isAdministrator(), "Public registration must create a student.")) return 1;

    const QDate tomorrow = labNow().date().addDays(1);
    if (require(repository.reserve({1, 2}, tomorrow, 8, &message), "A valid hourly reservation failed.")) return 1;
    if (require(!repository.reserve({1}, tomorrow, 8, &message), "A duplicate device-hour reservation was accepted.")) return 1;
    if (require(!repository.setFault(3, true, &message), "A student changed a device state.")) return 1;
    if (require(repository.reportIssue(4, QStringLiteral("Monitor has no signal"), &message), "Student issue report failed.")) return 1;

    const QString firstId = repository.bookings().first().id;
    const QDateTime start(tomorrow, QTime(8, 0), QTimeZone("Asia/Shanghai"));
    if (require(!repository.checkIn(firstId, &message, start.addSecs(-1)), "Early check-in was accepted.")) return 1;
    if (require(!repository.checkOut(firstId, &message, start), "Check-out before check-in was accepted.")) return 1;
    if (require(!repository.checkIn(firstId, &message, start.addSecs(3600)), "Check-in at exact end was accepted.")) return 1;
    if (require(repository.checkIn(firstId, &message, start), "Check-in at exact start failed.")) return 1;
    if (require(repository.checkIn(firstId, &message, start.addSecs(10)), "Repeated check-in was not idempotent.")) return 1;
    if (require(repository.bookings().first().checkedInAt == start, "Repeated check-in changed the timestamp.")) return 1;
    if (require(!repository.cancel(firstId, &message), "In-use reservation was cancelled.")) return 1;
    repository.load();
    if (require(repository.bookings().first().checkedInAt == start, "Check-in was not persisted.")) return 1;
    if (require(repository.checkOut(firstId, &message, start.addSecs(600)), "Check-out failed.")) return 1;
    if (require(repository.checkOut(firstId, &message, start.addSecs(900)), "Repeated check-out failed.")) return 1;
    if (require(!repository.checkIn(firstId, &message, start.addSecs(1000)), "Completed booking accepted check-in.")) return 1;
    repository.load();
    if (require(repository.bookings().first().checkedOutAt == start.addSecs(600), "Check-out timestamp was not persisted or was overwritten.")) return 1;
    const QString secondId = repository.bookings().last().id;
    if (require(repository.registerStudent(QStringLiteral("Other Student"), QStringLiteral("B25012414"), QStringLiteral("pass1234"), &message), "Second student registration failed.")) return 1;
    if (require(!repository.checkIn(secondId, &message, start), "Another student's reservation accepted check-in.")) return 1;
    const QString otherId = repository.currentUser()->id;
    repository.logout();
    if (require(repository.login(QStringLiteral("admin"), QStringLiteral("test-admin-password"), &message), "Seeded administrator sign-in failed.")) return 1;
    if (require(repository.setFault(3, true, &message), "Administrator fault maintenance failed.")) return 1;
    if (require(repository.isUnavailable(3, tomorrow, 8), "Faulty device remained bookable.")) return 1;
    if (require(repository.resolveIssue(repository.issues().first().id, &message), "Administrator could not resolve the issue.")) return 1;
    if (require(repository.deactivateStudent(otherId, &message), "Could not clean up second student.")) return 1;
    if (require(repository.deactivateStudent(repository.users().last().id, &message), "Administrator could not deactivate the student.")) return 1;
    if (require(repository.users().size() == 1, "Student account remained after deactivation.")) return 1;
    return 0;
}
