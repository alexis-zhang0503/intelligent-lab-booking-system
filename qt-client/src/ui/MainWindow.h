#pragma once

#include "services/LabRepository.h"

#include <QMainWindow>
#include <QSet>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTableWidget;
class QToolButton;
class QPushButton;

/**
 * @brief Native Windows interface for the C++ version of the lab reservation system.
 *
 * The window keeps rendering responsibilities in the UI layer and delegates
 * validation, conflict checks, and persistence to LabRepository.
 */
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    /** @brief Constructs the main window and shows the authentication view. */
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /** @brief Authenticates an existing account using the currently entered credentials. */
    void signIn();
    /** @brief Creates a public student account using the registration fields. */
    void registerStudent();
    /** @brief Returns to the authentication view after ending the local session. */
    void signOut();
    /** @brief Updates seat appearance after the student changes date or time. */
    void refreshStudentSeats();
    /** @brief Adds every selected available device to one hourly reservation. */
    void reserveSelectedSeats();
    /** @brief Cancels the booking selected in the student's reservation list. */
    void cancelSelectedBooking();
    void checkInSelectedBooking();
    void checkOutSelectedBooking();
    /** @brief Opens a small prompt that records a fault report for the selected device. */
    void reportSelectedIssue();
    /** @brief Toggles one device fault state from the administrator's plan. */
    void toggleAdministratorDevice();
    /** @brief Writes the daily usage report to a chosen count.txt destination. */
    void exportCountFile();
    /** @brief Writes issue records to a chosen error.txt destination. */
    void exportErrorFile();
    /** @brief Deactivates the student selected in the administrator account table. */
    void deactivateSelectedStudent();
    /** @brief Resolves the issue selected in the administrator issue table. */
    void resolveSelectedIssue();

private:
    /** @brief Builds the sign-in and public-student-registration page. */
    void showAuthenticationPage();
    /** @brief Builds the student or administrator dashboard after successful sign-in. */
    void showDashboard();
    /** @brief Creates the shared 40-device plan with the supplied click behavior. */
    QWidget *createDevicePlan(bool administratorMode);
    /** @brief Applies text, color, enabled state, and accessible names to one device button. */
    void styleDeviceButton(QToolButton *button, int deviceNumber, bool administratorMode);
    /** @brief Rebuilds the visible reservation list for the signed-in student. */
    void refreshBookingList();
    /** @brief Displays a consistent short status message to the user. */
    void showMessage(const QString &message, bool error = false);

    LabRepository m_repository;               ///< Owns demo data and domain rules.
    QStackedWidget *m_pages = nullptr;        ///< Switches cleanly between authentication and dashboard views.
    QLineEdit *m_nameInput = nullptr;         ///< Public-registration name field.
    QLineEdit *m_loginInput = nullptr;        ///< Shared student ID or administrator ID field.
    QLineEdit *m_passwordInput = nullptr;     ///< Shared password field.
    QDateEdit *m_dateInput = nullptr;         ///< Student-selected calendar date.
    QComboBox *m_hourInput = nullptr;          ///< Student-selected one-hour start time.
    QList<QToolButton *> m_deviceButtons;     ///< Forty buttons in physical-number order.
    QSet<int> m_selectedDevices;               ///< Student's current multiple-device selection.
    QPushButton *m_checkInButton = nullptr;
    QPushButton *m_checkOutButton = nullptr;
    void updateAttendanceButtons();
    QListWidget *m_bookingList = nullptr;      ///< Student's active reservations for cancellation.
    QTableWidget *m_userTable = nullptr;       ///< Administrator account list with the selected student identifier.
    QTableWidget *m_issueTable = nullptr;      ///< Administrator issue list with the selected report identifier.
    QLabel *m_statusLabel = nullptr;           ///< Non-modal validation and success feedback.
};
