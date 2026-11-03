#pragma once

#include "services/LabRepository.h"

#include <QMainWindow>
#include <QSet>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTimer;
class QToolButton;
class QVBoxLayout;

/** Native, local-data companion to the web reservation interface. */
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void signIn();
    void registerStudent();
    void signOut();
    void refreshDashboard();
    void refreshStudentSeats();
    void reserveSelectedSeats();
    void cancelSelectedBooking();
    void checkInSelectedBooking();
    void checkOutSelectedBooking();
    void reportSelectedIssue();
    void toggleAdministratorDevice();
    void exportCountFile();
    void exportErrorFile();
    void deactivateSelectedStudent();
    void resolveSelectedIssue();

private:
    // A session owns one dashboard and one timer. Retire both before clearing pointers.
    void clearPages();
    void showAuthenticationPage();
    void showDashboard();
    QVBoxLayout *addSection(const QString &title, const QString &description);
    void navigateTo(int index);
    void buildReservationPage();
    void buildHoursPage();
    void buildIssuesPage();
    void buildProfilePage();
    void buildOverviewPage();
    void buildDevicesPage();
    void buildUsersPage();
    QWidget *createDevicePlan(bool administratorMode);
    void styleDeviceButton(QToolButton *button, int deviceNumber, bool administratorMode);
    void refreshBookingList();
    void refreshProfileSummary();
    void refreshIssueTable();
    void refreshUserTable();
    void refreshOverview();
    void updateAttendanceButtons();
    void updateSelectionSummary();
    QList<Booking> personalBookings() const;
    QString selectedBookingId() const;
    void showMessage(const QString &message, bool error = false);

    LabRepository m_repository;
    QStackedWidget *m_pages = nullptr;
    QStackedWidget *m_sections = nullptr;
    QTimer *m_refreshTimer = nullptr;
    QList<QPushButton *> m_navigation;
    int m_profileIndex = -1;
    QLineEdit *m_nameInput = nullptr;
    QLineEdit *m_loginInput = nullptr;
    QLineEdit *m_passwordInput = nullptr;
    QDateEdit *m_dateInput = nullptr;
    QComboBox *m_hourInput = nullptr;
    QList<QToolButton *> m_deviceButtons;
    QSet<int> m_selectedDevices;
    QLabel *m_selectionSummary = nullptr;
    QLabel *m_labSummary = nullptr;
    QLabel *m_todaySummary = nullptr;
    QLabel *m_preferenceSummary = nullptr;
    QLabel *m_hoursSummary = nullptr;
    QLabel *m_overviewSummary = nullptr;
    QLabel *m_bookingEmpty = nullptr;
    QLabel *m_issueEmpty = nullptr;
    QLabel *m_attendanceHint = nullptr;
    QTableWidget *m_bookingTable = nullptr;
    QTableWidget *m_overviewTable = nullptr;
    QTableWidget *m_userTable = nullptr;
    QTableWidget *m_issueTable = nullptr;
    QPushButton *m_reserveButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_checkInButton = nullptr;
    QPushButton *m_checkOutButton = nullptr;
    QPushButton *m_resolveButton = nullptr;
    QPushButton *m_deactivateButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};
