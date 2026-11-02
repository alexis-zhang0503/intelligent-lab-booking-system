#include "MainWindow.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

/** @brief Produces a rounded primary or secondary action button. */
QPushButton *actionButton(const QString &text, const bool primary = false)
{
    auto *button = new QPushButton(text);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(42);
    button->setStyleSheet(primary
        ? QStringLiteral("QPushButton { background:#0071e3; color:white; border:0; border-radius:21px; font-weight:600; padding:0 18px; } QPushButton:hover { background:#0077ed; }")
        : QStringLiteral("QPushButton { background:white; color:#0071e3; border:1px solid #d2d2d7; border-radius:21px; font-weight:600; padding:0 18px; } QPushButton:hover { background:#f5f5f7; }"));
    return button;
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_repository.load();
    setWindowTitle(QStringLiteral("NJUPT Intelligent Lab Reservation"));
    resize(1180, 760);
    setMinimumSize(920, 620);
    setStyleSheet(QStringLiteral("QMainWindow { background:#f5f5f7; color:#1d1d1f; } QLabel { font-family:'Segoe UI'; } QLineEdit,QDateEdit,QComboBox { min-height:38px; padding:0 10px; border:1px solid #d2d2d7; border-radius:10px; background:white; } QLineEdit:focus,QDateEdit:focus,QComboBox:focus { border:2px solid #0071e3; }"));
    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);
    showAuthenticationPage();
}

void MainWindow::showAuthenticationPage()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(72, 64, 72, 64);
    layout->setSpacing(64);

    auto *copy = new QVBoxLayout;
    auto *eyebrow = new QLabel(QStringLiteral("NJUPT · LAB RESERVATION"));
    eyebrow->setStyleSheet(QStringLiteral("color:#0071e3; font-weight:700; letter-spacing:1px;"));
    auto *title = new QLabel(QStringLiteral("Reserve the lab\nMake time for discovery"));
    title->setStyleSheet(QStringLiteral("font-size:44px; font-weight:600; letter-spacing:-1px;"));
    auto *description = new QLabel(QStringLiteral("A C++ desktop companion for hourly lab reservations, device maintenance, and course-required exports."));
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color:#6e6e73; font-size:16px; line-height:1.5;"));
    copy->addWidget(eyebrow);
    copy->addSpacing(16);
    copy->addWidget(title);
    copy->addSpacing(16);
    copy->addWidget(description);
    copy->addStretch();

    auto *card = new QGroupBox(QStringLiteral("Sign in or create a student account"));
    card->setStyleSheet(QStringLiteral("QGroupBox { background:white; border:1px solid #e5e5ea; border-radius:22px; font-size:18px; font-weight:600; padding:28px 24px 24px 24px; } QGroupBox::title { subcontrol-origin: margin; left:24px; top:12px; }"));
    auto *form = new QFormLayout(card);
    form->setSpacing(14);
    form->setContentsMargins(16, 26, 16, 16);
    m_nameInput = new QLineEdit;
    m_nameInput->setPlaceholderText(QStringLiteral("Required when creating a student account"));
    m_loginInput = new QLineEdit;
    m_loginInput->setPlaceholderText(QStringLiteral("Student ID or administrator ID"));
    m_passwordInput = new QLineEdit;
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setPlaceholderText(QStringLiteral("At least four characters"));
    form->addRow(QStringLiteral("Name"), m_nameInput);
    form->addRow(QStringLiteral("ID"), m_loginInput);
    form->addRow(QStringLiteral("Password"), m_passwordInput);
    auto *signInButton = actionButton(QStringLiteral("Sign in"), true);
    auto *registerButton = actionButton(QStringLiteral("Create student account"));
    form->addRow(signInButton);
    form->addRow(registerButton);
    auto *adminHint = new QLabel(QStringLiteral("Administrator accounts are pre-seeded or issued by invitation. Public registration creates students only."));
    adminHint->setWordWrap(true);
    adminHint->setStyleSheet(QStringLiteral("color:#6e6e73; font-size:12px;"));
    form->addRow(adminHint);
    connect(signInButton, &QPushButton::clicked, this, &MainWindow::signIn);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::registerStudent);

    layout->addLayout(copy, 3);
    layout->addWidget(card, 2);
    m_pages->addWidget(page);
    m_pages->setCurrentWidget(page);
}

void MainWindow::signIn()
{
    QString message;
    if (!m_repository.login(m_loginInput->text(), m_passwordInput->text(), &message)) {
        QMessageBox::warning(this, QStringLiteral("Sign in"), message);
        return;
    }
    showDashboard();
}

void MainWindow::registerStudent()
{
    QString message;
    if (!m_repository.registerStudent(m_nameInput->text(), m_loginInput->text(), m_passwordInput->text(), &message)) {
        QMessageBox::warning(this, QStringLiteral("Create account"), message);
        return;
    }
    showDashboard();
}

void MainWindow::signOut()
{
    m_repository.logout();
    showAuthenticationPage();
}

void MainWindow::showDashboard()
{
    m_selectedDevices.clear();
    m_deviceButtons.clear();
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(42, 28, 42, 28);
    layout->setSpacing(18);

    const User *sessionUser = m_repository.currentUser();
    auto *header = new QHBoxLayout;
    auto *identity = new QLabel(QStringLiteral("%1\n%2 · %3").arg(sessionUser->name, sessionUser->role == UserRole::Administrator ? QStringLiteral("Administrator") : QStringLiteral("Student"), sessionUser->loginId));
    identity->setStyleSheet(QStringLiteral("font-size:16px; font-weight:600;"));
    auto *signOutButton = actionButton(QStringLiteral("Sign out"));
    connect(signOutButton, &QPushButton::clicked, this, &MainWindow::signOut);
    header->addWidget(identity);
    header->addStretch();
    header->addWidget(signOutButton);
    layout->addLayout(header);

    auto *title = new QLabel(m_repository.isAdministrator() ? QStringLiteral("Lab operations") : QStringLiteral("Reserve the lab"));
    title->setStyleSheet(QStringLiteral("font-size:34px; font-weight:600; letter-spacing:-1px;"));
    layout->addWidget(title);

    if (m_repository.isAdministrator()) {
        auto *top = new QHBoxLayout;
        auto *stats = new QLabel(QStringLiteral("Current lab users: %1    This week: %2 reservations    Devices: 40").arg(m_repository.currentLabUsers()).arg(m_repository.currentWeekReservations()));
        stats->setStyleSheet(QStringLiteral("color:#6e6e73; font-size:15px;"));
        top->addWidget(stats);
        top->addStretch();
        auto *countButton = actionButton(QStringLiteral("Export count.txt"));
        auto *errorButton = actionButton(QStringLiteral("Export error.txt"));
        connect(countButton, &QPushButton::clicked, this, &MainWindow::exportCountFile);
        connect(errorButton, &QPushButton::clicked, this, &MainWindow::exportErrorFile);
        top->addWidget(countButton);
        top->addWidget(errorButton);
        layout->addLayout(top);
        layout->addWidget(createDevicePlan(true), 2);

        m_userTable = new QTableWidget(m_repository.users().size(), 4);
        m_userTable->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Role"), QStringLiteral("ID"), QStringLiteral("Completed hours")});
        m_userTable->verticalHeader()->hide();
        m_userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        for (int row = 0; row < m_repository.users().size(); ++row) {
            const User &account = m_repository.users().at(row);
            auto *nameItem = new QTableWidgetItem(account.name);
            nameItem->setData(Qt::UserRole, account.id);
            m_userTable->setItem(row, 0, nameItem);
            m_userTable->setItem(row, 1, new QTableWidgetItem(account.role == UserRole::Administrator ? QStringLiteral("Administrator") : QStringLiteral("Student")));
            m_userTable->setItem(row, 2, new QTableWidgetItem(account.loginId));
            m_userTable->setItem(row, 3, new QTableWidgetItem(QString::number(m_repository.bookedHoursForUser(account.id))));
        }
        m_userTable->horizontalHeader()->setStretchLastSection(true);
        layout->addWidget(m_userTable, 1);
        auto *deactivateButton = actionButton(QStringLiteral("Deactivate selected student"));
        connect(deactivateButton, &QPushButton::clicked, this, &MainWindow::deactivateSelectedStudent);
        layout->addWidget(deactivateButton, 0, Qt::AlignRight);

        auto *issueGroup = new QGroupBox(QStringLiteral("Issue reports"));
        auto *issueLayout = new QVBoxLayout(issueGroup);
        m_issueTable = new QTableWidget(m_repository.issues().size(), 4);
        m_issueTable->setHorizontalHeaderLabels({QStringLiteral("Device"), QStringLiteral("Description"), QStringLiteral("Reporter"), QStringLiteral("Status")});
        m_issueTable->verticalHeader()->hide();
        m_issueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        for (int row = 0; row < m_repository.issues().size(); ++row) {
            const IssueReport &issue = m_repository.issues().at(row);
            auto *deviceItem = new QTableWidgetItem(QString::number(issue.deviceNumber));
            deviceItem->setData(Qt::UserRole, issue.id);
            m_issueTable->setItem(row, 0, deviceItem);
            m_issueTable->setItem(row, 1, new QTableWidgetItem(issue.description));
            m_issueTable->setItem(row, 2, new QTableWidgetItem(issue.reporterId));
            m_issueTable->setItem(row, 3, new QTableWidgetItem(issue.resolved ? QStringLiteral("Resolved") : QStringLiteral("Open")));
        }
        m_issueTable->horizontalHeader()->setStretchLastSection(true);
        auto *resolveButton = actionButton(QStringLiteral("Resolve selected issue"));
        connect(resolveButton, &QPushButton::clicked, this, &MainWindow::resolveSelectedIssue);
        issueLayout->addWidget(m_issueTable);
        issueLayout->addWidget(resolveButton, 0, Qt::AlignRight);
        layout->addWidget(issueGroup, 1);
    } else {
        auto *tools = new QHBoxLayout;
        m_dateInput = new QDateEdit(QDate::currentDate());
        m_dateInput->setMinimumDate(QDate::currentDate());
        m_dateInput->setCalendarPopup(true);
        m_hourInput = new QComboBox;
        for (int hour = 8; hour <= 16; ++hour) m_hourInput->addItem(hourlyLabel(hour), hour);
        auto *reserveButton = actionButton(QStringLiteral("Confirm reservation"), true);
        auto *reportButton = actionButton(QStringLiteral("Report selected device"));
        connect(m_dateInput, &QDateEdit::dateChanged, this, &MainWindow::refreshStudentSeats);
        connect(m_hourInput, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::refreshStudentSeats);
        connect(reserveButton, &QPushButton::clicked, this, &MainWindow::reserveSelectedSeats);
        connect(reportButton, &QPushButton::clicked, this, &MainWindow::reportSelectedIssue);
        tools->addWidget(new QLabel(QStringLiteral("Date")));
        tools->addWidget(m_dateInput);
        tools->addWidget(new QLabel(QStringLiteral("Hourly slot")));
        tools->addWidget(m_hourInput);
        tools->addStretch();
        tools->addWidget(reportButton);
        tools->addWidget(reserveButton);
        layout->addLayout(tools);
        layout->addWidget(createDevicePlan(false), 2);

        m_bookingList = new QListWidget;
        auto *cancelButton = actionButton(QStringLiteral("Cancel selected reservation"));
        connect(cancelButton, &QPushButton::clicked, this, &MainWindow::cancelSelectedBooking);
        auto *bookingGroup = new QGroupBox(QStringLiteral("My reservations"));
        auto *bookingLayout = new QVBoxLayout(bookingGroup);
        bookingLayout->addWidget(m_bookingList);
        bookingLayout->addWidget(cancelButton);
        layout->addWidget(bookingGroup, 1);
        refreshBookingList();
        auto *statusTimer = new QTimer(page);
        connect(statusTimer, &QTimer::timeout, this, [this] { refreshStudentSeats(); refreshBookingList(); });
        statusTimer->start(30000);
    }

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet(QStringLiteral("color:#006b3c; font-weight:600;"));
    layout->addWidget(m_statusLabel);
    m_pages->addWidget(page);
    m_pages->setCurrentWidget(page);
}

QWidget *MainWindow::createDevicePlan(const bool administratorMode)
{
    auto *group = new QGroupBox(QStringLiteral("40-device lab plan  ·  * available   # reserved   ! faulty"));
    group->setStyleSheet(QStringLiteral("QGroupBox { background:white; border:1px solid #e5e5ea; border-radius:18px; font-weight:600; padding:24px 16px 16px 16px; } QGroupBox::title { subcontrol-origin: margin; left:16px; top:7px; }"));
    auto *grid = new QGridLayout(group);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(12);
    for (int number = 1; number <= 40; ++number) {
        auto *button = new QToolButton;
        button->setProperty("deviceNumber", number);
        button->setMinimumSize(92, 54);
        button->setCursor(Qt::PointingHandCursor);
        styleDeviceButton(button, number, administratorMode);
        if (administratorMode) connect(button, &QToolButton::clicked, this, &MainWindow::toggleAdministratorDevice);
        else connect(button, &QToolButton::clicked, this, [this, button, number] {
            if (m_repository.isUnavailable(number, m_dateInput->date(), m_hourInput->currentData().toInt())) return;
            if (m_selectedDevices.contains(number)) m_selectedDevices.remove(number); else m_selectedDevices.insert(number);
            styleDeviceButton(button, number, false);
        });
        grid->addWidget(button, (number - 1) / 8, (number - 1) % 8);
        m_deviceButtons.append(button);
    }
    return group;
}

void MainWindow::styleDeviceButton(QToolButton *button, const int deviceNumber, const bool administratorMode)
{
    bool faulty = false;
    for (const Device &device : m_repository.devices()) {
        if (device.number == deviceNumber) faulty = device.faulty;
    }
    const bool unavailable = !administratorMode && m_repository.isUnavailable(deviceNumber, m_dateInput->date(), m_hourInput->currentData().toInt());
    const bool selected = !administratorMode && m_selectedDevices.contains(deviceNumber);
    const QString symbol = faulty ? QStringLiteral("!") : unavailable ? QStringLiteral("#") : selected ? QStringLiteral("●") : QStringLiteral("*");
    const QString state = faulty ? QStringLiteral("faulty") : unavailable ? QStringLiteral("reserved") : selected ? QStringLiteral("selected") : QStringLiteral("available");
    const QString color = faulty ? QStringLiteral("#d92d20") : unavailable ? QStringLiteral("#6e6e73") : selected ? QStringLiteral("#0071e3") : QStringLiteral("#168554");
    const QString background = faulty ? QStringLiteral("#fff1f0") : unavailable ? QStringLiteral("#ededf0") : selected ? QStringLiteral("#edf6ff") : QStringLiteral("#f4fbf7");
    button->setText(QStringLiteral("%1  %2").arg(deviceNumber, 2, 10, QLatin1Char('0')).arg(symbol));
    button->setAccessibleName(QStringLiteral("Device %1, %2").arg(deviceNumber).arg(state));
    button->setEnabled(administratorMode || (!faulty && !unavailable));
    button->setStyleSheet(QStringLiteral("QToolButton { color:%1; background:%2; border:1px solid %3; border-radius:12px; font-weight:700; font-size:15px; } QToolButton:hover:enabled { border-color:#0071e3; background:#ffffff; }")
        .arg(color, background, color));
}

void MainWindow::refreshStudentSeats()
{
    m_selectedDevices.clear();
    for (int index = 0; index < m_deviceButtons.size(); ++index) styleDeviceButton(m_deviceButtons.at(index), index + 1, false);
}

void MainWindow::reserveSelectedSeats()
{
    QString message;
    if (m_repository.reserve(m_selectedDevices.values(), m_dateInput->date(), m_hourInput->currentData().toInt(), &message)) {
        m_selectedDevices.clear();
        refreshStudentSeats();
        refreshBookingList();
        showMessage(message);
    } else showMessage(message, true);
}

void MainWindow::refreshBookingList()
{
    if (!m_bookingList || !m_repository.currentUser()) return;
    m_bookingList->clear();
    for (const Booking &booking : m_repository.bookingsForUser(m_repository.currentUser()->id)) {
        auto *item = new QListWidgetItem(QStringLiteral("Device %1 · %2 · %3").arg(booking.deviceNumber, 2, 10, QLatin1Char('0')).arg(booking.date.toString(Qt::ISODate), hourlyLabel(booking.startHour)));
        item->setData(Qt::UserRole, booking.id);
        m_bookingList->addItem(item);
    }
}

void MainWindow::cancelSelectedBooking()
{
    if (!m_bookingList || !m_bookingList->currentItem()) { showMessage(QStringLiteral("Select one of your reservations to cancel."), true); return; }
    QString message;
    if (m_repository.cancel(m_bookingList->currentItem()->data(Qt::UserRole).toString(), &message)) {
        refreshBookingList();
        refreshStudentSeats();
        showMessage(message);
    } else showMessage(message, true);
}

void MainWindow::reportSelectedIssue()
{
    if (m_selectedDevices.size() != 1) { showMessage(QStringLiteral("Select exactly one device to report an issue."), true); return; }
    bool accepted = false;
    const QString description = QInputDialog::getText(this, QStringLiteral("Report device issue"), QStringLiteral("Description"), QLineEdit::Normal, QString(), &accepted);
    if (!accepted) return;
    QString message;
    if (m_repository.reportIssue(*m_selectedDevices.begin(), description, &message)) showMessage(message); else showMessage(message, true);
}

void MainWindow::toggleAdministratorDevice()
{
    auto *button = qobject_cast<QToolButton *>(sender());
    if (!button) return;
    const int deviceNumber = button->property("deviceNumber").toInt();
    bool faulty = false;
    for (const Device &device : m_repository.devices()) if (device.number == deviceNumber) faulty = device.faulty;
    QString message;
    if (m_repository.setFault(deviceNumber, !faulty, &message)) {
        for (int index = 0; index < m_deviceButtons.size(); ++index) styleDeviceButton(m_deviceButtons.at(index), index + 1, true);
        showMessage(message);
    } else showMessage(message, true);
}

void MainWindow::exportCountFile()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export daily usage"), QStringLiteral("count.txt"), QStringLiteral("Text files (*.txt)"));
    if (path.isEmpty()) return;
    QString message;
    if (m_repository.exportCountFile(path, QDate::currentDate(), &message)) showMessage(message); else showMessage(message, true);
}

void MainWindow::exportErrorFile()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export issue reports"), QStringLiteral("error.txt"), QStringLiteral("Text files (*.txt)"));
    if (path.isEmpty()) return;
    QString message;
    if (m_repository.exportErrorFile(path, &message)) showMessage(message); else showMessage(message, true);
}

void MainWindow::deactivateSelectedStudent()
{
    if (!m_userTable || !m_userTable->currentItem()) { showMessage(QStringLiteral("Select a student account first."), true); return; }
    const int row = m_userTable->currentRow();
    const QString userId = m_userTable->item(row, 0)->data(Qt::UserRole).toString();
    QString message;
    if (m_repository.deactivateStudent(userId, &message)) showDashboard(); else showMessage(message, true);
}

void MainWindow::resolveSelectedIssue()
{
    if (!m_issueTable || !m_issueTable->currentItem()) { showMessage(QStringLiteral("Select an issue report first."), true); return; }
    const QString issueId = m_issueTable->item(m_issueTable->currentRow(), 0)->data(Qt::UserRole).toString();
    QString message;
    if (m_repository.resolveIssue(issueId, &message)) showDashboard(); else showMessage(message, true);
}

void MainWindow::showMessage(const QString &message, const bool error)
{
    if (!m_statusLabel) return;
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(error ? QStringLiteral("color:#d92d20; font-weight:600;") : QStringLiteral("color:#006b3c; font-weight:600;"));
}
