#include "MainWindow.h"
#include "ui/BrandLogo.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QPushButton *actionButton(const QString &text, const bool primary = false)
{
    auto *button = new QPushButton(text);
    button->setProperty("kind", primary ? "primary" : "secondary");
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(40);
    return button;
}

QLabel *textLabel(const QString &text, const QString &kind = QStringLiteral("body"))
{
    auto *label = new QLabel(text);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setProperty("kind", kind);
    return label;
}

QFrame *card(const QString &title, const QString &description = QString())
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(14);
    if (!title.isEmpty()) layout->addWidget(textLabel(title, QStringLiteral("cardTitle")));
    if (!description.isEmpty()) layout->addWidget(textLabel(description, QStringLiteral("muted")));
    return frame;
}

QVBoxLayout *cardLayout(QFrame *frame)
{
    return qobject_cast<QVBoxLayout *>(frame->layout());
}

QTableWidget *recordTable(const QStringList &headers)
{
    auto *table = new QTableWidget(0, static_cast<int>(headers.size()));
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->setMinimumHeight(240);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->verticalHeader()->setDefaultSectionSize(44);
    return table;
}

QString selectedRecord(QTableWidget *table)
{
    if (!table || table->currentRow() < 0) return {};
    const auto *item = table->item(table->currentRow(), 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

// Refresh table contents without jumping to the first record or changing its scroll position.
struct TablePosition {
    QString id;
    int vertical;
    int horizontal;
    explicit TablePosition(QTableWidget *table)
        : id(selectedRecord(table)), vertical(table->verticalScrollBar()->value()),
          horizontal(table->horizontalScrollBar()->value()) {}
    void restore(QTableWidget *table) const
    {
        table->clearSelection();
        table->setCurrentCell(-1, -1);
        for (int row = 0; row < table->rowCount(); ++row) {
            if (!id.isEmpty() && table->item(row, 0)->data(Qt::UserRole).toString() == id) {
                table->setCurrentCell(row, 0);
                break;
            }
        }
        table->verticalScrollBar()->setValue(vertical);
        table->horizontalScrollBar()->setValue(horizontal);
    }
};

void setRecord(QTableWidget *table, const int row, const QString &id, const QStringList &values)
{
    for (int column = 0; column < values.size(); ++column) {
        auto *item = new QTableWidgetItem(values.at(column));
        item->setToolTip(values.at(column));
        if (column == 0) item->setData(Qt::UserRole, id);
        table->setItem(row, column, item);
    }
}

void warningDialog(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox dialog(QMessageBox::Warning, title, message, QMessageBox::NoButton, parent);
    dialog.setTextFormat(Qt::PlainText);
    dialog.addButton(QStringLiteral("确定"), QMessageBox::AcceptRole);
    dialog.exec();
}

QString seatLabel(const int number)
{
    return QStringLiteral("#%1").arg(number, 2, 10, QLatin1Char('0'));
}

QString attendanceTime(const QDateTime &time)
{
    return time.isValid() ? time.toTimeZone(QTimeZone("Asia/Shanghai")).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                          : QStringLiteral("—");
}

qint64 completedSeconds(const Booking &booking)
{
    return booking.checkedInAt.isValid() && booking.checkedOutAt.isValid()
        ? qMax<qint64>(0, booking.checkedInAt.secsTo(booking.checkedOutAt)) : 0;
}

QString durationText(const qint64 seconds)
{
    return QStringLiteral("%1 小时 %2 分 %3 秒").arg(seconds / 3600).arg(seconds / 60 % 60).arg(seconds % 60);
}

QString actualDuration(const Booking &booking)
{
    if (booking.checkedOutAt.isValid() && booking.checkedInAt.isValid()) return durationText(completedSeconds(booking));
    if (booking.checkedInAt.isValid()) return QStringLiteral("进行中，待签退");
    return QStringLiteral("—");
}

QString bookingState(const Booking &booking)
{
    if (booking.cancelled) return QStringLiteral("已取消");
    if (booking.checkedOutAt.isValid()) return QStringLiteral("已完成");
    if (booking.checkedInAt.isValid()) return QStringLiteral("使用中");
    if (labNow() >= bookingStart(booking).addSecs(3600)) return QStringLiteral("已结束 · 未签到");
    if (canCheckIn(booking)) return QStringLiteral("待签到");
    return QStringLiteral("已确认");
}

bool canCancel(const Booking &booking)
{
    return !booking.cancelled && !booking.checkedInAt.isValid() && !booking.checkedOutAt.isValid()
        && labNow() < bookingStart(booking);
}

QString favoriteText(const QMap<int, int> &counts, const bool device)
{
    if (counts.isEmpty()) return QStringLiteral("暂未形成");
    int favorite = counts.firstKey();
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (it.value() > counts.value(favorite)) favorite = it.key();
    }
    return QStringLiteral("%1 · %2 次").arg(device ? seatLabel(favorite) : hourlyLabel(favorite)).arg(counts.value(favorite));
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_repository.load();
    setWindowTitle(QStringLiteral("实验室预约 · 桌面端"));
    resize(1280, 860);
    setMinimumSize(980, 680);
    setLocale(QLocale(QLocale::Chinese, QLocale::China));
    setStyleSheet(QStringLiteral(
        "QMainWindow, QStackedWidget { background:#f7f9fc; }"
        "QWidget { font-family:'Microsoft YaHei UI','Noto Sans SC','Segoe UI'; font-size:13px; color:#18243a; }"
        "QFrame#topBar, QFrame#card { background:#ffffff; border:1px solid #dfe7f1; border-radius:16px; }"
        "QFrame#hero { background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #eef5ff,stop:1 #ffffff); border:1px solid #dfe7f1; border-radius:20px; }"
        "QLabel { background:transparent; border:0; }"
        "QLabel[kind='title'] { font-size:30px; font-weight:700; }"
        "QLabel[kind='cardTitle'] { font-size:18px; font-weight:600; }"
        "QLabel[kind='muted'] { color:#64748b; font-size:12px; }"
        "QLabel[kind='metric'] { color:#3568d4; font-size:18px; font-weight:600; }"
        "QLineEdit,QDateEdit,QComboBox { min-height:38px; padding:0 10px; border:1px solid #dfe7f1; border-radius:10px; background:white; selection-background-color:#5686ff; }"
        "QLineEdit:focus,QDateEdit:focus,QComboBox:focus { border:1px solid #5686ff; }"
        "QPushButton { padding:8px 16px; border:1px solid #dfe7f1; border-radius:10px; background:#ffffff; color:#3568d4; font-weight:600; }"
        "QPushButton:hover { background:#eef4ff; border-color:#b8cdfa; }"
        "QPushButton[kind='primary'] { color:#ffffff; background:#5686ff; border-color:#5686ff; }"
        "QPushButton[kind='primary']:hover { background:#4474ed; }"
        "QPushButton[kind='nav'] { border:0; color:#64748b; background:transparent; padding:9px 14px; }"
        "QPushButton[kind='nav']:checked { color:#3568d4; background:#eaf1ff; }"
        "QPushButton:disabled { color:#94a3b8; background:#f0f4fa; border-color:#e6ecf4; }"
        "QTableWidget { background:#ffffff; alternate-background-color:#f7f9fc; border:1px solid #e6edf5; border-radius:10px; selection-background-color:#e5eeff; selection-color:#254f9e; }"
        "QTableWidget::item { padding:8px; border:0; }"
        "QHeaderView::section { background:#f0f4fa; color:#64748b; border:0; padding:11px 9px; font-weight:600; }"
        "QScrollArea { border:0; background:transparent; }"
        "QScrollBar:vertical { width:10px; background:#f0f4fa; margin:0; }"
        "QScrollBar::handle:vertical { background:#c8d5e6; border-radius:5px; min-height:28px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }"));
    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);
    showAuthenticationPage();
}

void MainWindow::clearPages()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        m_refreshTimer->disconnect(this);
    }
    // deleteLater is necessary when logout was triggered by a button in the old page.
    // Disconnect retired controls immediately so queued UI work cannot touch the next session.
    while (m_pages->count()) {
        QWidget *page = m_pages->widget(0);
        for (QObject *child : page->findChildren<QObject *>()) QObject::disconnect(child, nullptr, this, nullptr);
        m_pages->removeWidget(page);
        page->hide();
        page->deleteLater();
    }
    m_sections = nullptr;
    m_refreshTimer = nullptr;
    m_navigation.clear();
    m_profileIndex = -1;
    m_nameInput = nullptr;
    m_loginInput = nullptr;
    m_passwordInput = nullptr;
    m_dateInput = nullptr;
    m_hourInput = nullptr;
    m_deviceButtons.clear();
    m_selectedDevices.clear();
    m_selectionSummary = nullptr;
    m_labSummary = nullptr;
    m_todaySummary = nullptr;
    m_preferenceSummary = nullptr;
    m_hoursSummary = nullptr;
    m_overviewSummary = nullptr;
    m_bookingEmpty = nullptr;
    m_issueEmpty = nullptr;
    m_attendanceHint = nullptr;
    m_bookingTable = nullptr;
    m_overviewTable = nullptr;
    m_userTable = nullptr;
    m_issueTable = nullptr;
    m_reserveButton = nullptr;
    m_cancelButton = nullptr;
    m_checkInButton = nullptr;
    m_checkOutButton = nullptr;
    m_resolveButton = nullptr;
    m_deactivateButton = nullptr;
    m_statusLabel = nullptr;
}

void MainWindow::showAuthenticationPage()
{
    clearPages();
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(60, 64, 60, 64);
    layout->setSpacing(48);
    auto *intro = new QVBoxLayout;
    auto *logo = new QLabel;
    logo->setPixmap(labBrandIcon().pixmap(56, 56));
    logo->setAccessibleName(QStringLiteral("实验室预约标识"));
    intro->addStretch();
    intro->addWidget(logo);
    intro->addWidget(textLabel(QStringLiteral("实验室预约 · 桌面端"), QStringLiteral("metric")));
    intro->addSpacing(14);
    intro->addWidget(textLabel(QStringLiteral("把时间留给探索"), QStringLiteral("title")));
    intro->addWidget(textLabel(QStringLiteral("选定设备，约好时间。\n设备、时段和你的记录，都在这里。")));
    intro->addSpacing(18);
    intro->addWidget(textLabel(QStringLiteral("本机数据存储 · 北京时间（UTC+8）\n支持预约、签到签退与设备维护。"), QStringLiteral("muted")));
    intro->addStretch();
    auto *authCard = card(QStringLiteral("欢迎回来"), QStringLiteral("登录账号，或创建学生账号开始预约。"));
    authCard->setMinimumWidth(360);
    authCard->setMaximumWidth(450);
    auto *form = new QFormLayout;
    form->setSpacing(16);
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    m_nameInput = new QLineEdit;
    m_nameInput->setPlaceholderText(QStringLiteral("注册时填写姓名"));
    m_loginInput = new QLineEdit;
    m_loginInput->setPlaceholderText(QStringLiteral("请输入学号或管理员账号"));
    m_passwordInput = new QLineEdit;
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setPlaceholderText(QStringLiteral("至少 4 个字符"));
    form->addRow(QStringLiteral("姓名（仅注册需要）"), m_nameInput);
    form->addRow(QStringLiteral("学号 / 工号"), m_loginInput);
    form->addRow(QStringLiteral("密码"), m_passwordInput);
    cardLayout(authCard)->addLayout(form);
    auto *login = actionButton(QStringLiteral("登录并继续"), true);
    auto *registration = actionButton(QStringLiteral("创建学生账号"));
    cardLayout(authCard)->addWidget(login);
    cardLayout(authCard)->addWidget(registration);
    cardLayout(authCard)->addWidget(textLabel(QStringLiteral("管理员账号由项目维护者配置；公开注册仅创建学生账号。"), QStringLiteral("muted")));
    connect(login, &QPushButton::clicked, this, &MainWindow::signIn);
    connect(registration, &QPushButton::clicked, this, &MainWindow::registerStudent);
    connect(m_passwordInput, &QLineEdit::returnPressed, this, &MainWindow::signIn);
    layout->addLayout(intro, 1);
    layout->addWidget(authCard, 1, Qt::AlignVCenter);
    m_pages->addWidget(page);
    m_pages->setCurrentWidget(page);
    m_loginInput->setFocus();
}

void MainWindow::signIn()
{
    if (!m_loginInput || !m_passwordInput) return;
    QString message;
    if (!m_repository.login(m_loginInput->text(), m_passwordInput->text(), &message)) {
        warningDialog(this, QStringLiteral("登录失败"), message);
        return;
    }
    showDashboard();
}

void MainWindow::registerStudent()
{
    if (!m_nameInput || !m_loginInput || !m_passwordInput) return;
    QString message;
    if (!m_repository.registerStudent(m_nameInput->text(), m_loginInput->text(), m_passwordInput->text(), &message)) {
        warningDialog(this, QStringLiteral("创建账号"), message);
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
    if (!m_repository.currentUser()) return;
    clearPages();
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(22, 16, 22, 12);
    layout->setSpacing(12);
    auto *bar = new QFrame;
    bar->setObjectName(QStringLiteral("topBar"));
    auto *header = new QHBoxLayout(bar);
    header->setContentsMargins(18, 10, 18, 10);
    header->setSpacing(8);
    header->addWidget(textLabel(QStringLiteral("实验室预约"), QStringLiteral("cardTitle")));
    header->addSpacing(18);
    const bool administrator = m_repository.isAdministrator();
    const QStringList sections = administrator
        ? QStringList{QStringLiteral("总览"), QStringLiteral("设备"), QStringLiteral("用户"), QStringLiteral("故障"), QStringLiteral("我的")}
        : QStringList{QStringLiteral("预约"), QStringLiteral("时长"), QStringLiteral("故障"), QStringLiteral("我的")};
    for (int index = 0; index < sections.size(); ++index) {
        auto *button = actionButton(sections.at(index));
        button->setProperty("kind", "nav");
        button->setCheckable(true);
        button->setAccessibleName(sections.at(index) + QStringLiteral("导航"));
        connect(button, &QPushButton::clicked, this, [this, index] { navigateTo(index); });
        header->addWidget(button);
        m_navigation.append(button);
    }
    header->addStretch();
    const User &user = *m_repository.currentUser();
    auto *identity = textLabel(user.name);
    identity->setMaximumWidth(140);
    identity->setToolTip(QStringLiteral("%1 · %2").arg(user.name, user.loginId));
    header->addWidget(identity);
    auto *logout = actionButton(QStringLiteral("退出"));
    connect(logout, &QPushButton::clicked, this, &MainWindow::signOut);
    header->addWidget(logout);
    layout->addWidget(bar);
    m_sections = new QStackedWidget;
    layout->addWidget(m_sections, 1);
    if (administrator) {
        buildOverviewPage();
        buildDevicesPage();
        buildUsersPage();
    } else {
        buildReservationPage();
        buildHoursPage();
    }
    buildIssuesPage();
    m_profileIndex = m_sections->count();
    buildProfilePage();
    auto *footer = new QHBoxLayout;
    m_statusLabel = textLabel(QStringLiteral("数据保存在本机。"), QStringLiteral("muted"));
    m_statusLabel->setAccessibleName(QStringLiteral("操作结果"));
    footer->addWidget(m_statusLabel, 1);
    footer->addWidget(textLabel(QStringLiteral("本机数据 · 北京时间 UTC+8"), QStringLiteral("muted")));
    auto *refresh = actionButton(QStringLiteral("刷新"));
    connect(refresh, &QPushButton::clicked, this, [this] {
        refreshDashboard();
        showMessage(QStringLiteral("已刷新当前本机数据。"));
    });
    footer->addWidget(refresh);
    layout->addLayout(footer);
    m_pages->addWidget(page);
    m_pages->setCurrentWidget(page);
    navigateTo(0);
    refreshDashboard();
    m_refreshTimer = new QTimer(page);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDashboard);
    m_refreshTimer->start(30000);
}

QVBoxLayout *MainWindow::addSection(const QString &title, const QString &description)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(12, 18, 12, 24);
    layout->setSpacing(18);
    layout->addWidget(textLabel(title, QStringLiteral("title")));
    layout->addWidget(textLabel(description, QStringLiteral("muted")));
    scroll->setWidget(content);
    m_sections->addWidget(scroll);
    return layout;
}

void MainWindow::navigateTo(const int index)
{
    if (!m_sections || index < 0 || index >= m_sections->count()) return;
    m_sections->setCurrentIndex(index);
    for (int i = 0; i < m_navigation.size(); ++i) m_navigation.at(i)->setChecked(i == index);
    // Each section keeps its own scroll area; returning to a page preserves reading position.
    refreshDashboard();
}

void MainWindow::buildReservationPage()
{
    auto *layout = addSection(QStringLiteral("实验室预约"), QStringLiteral("把时间留给探索 · 选择设备 → 设置时段 → 确认预约"));
    auto *controls = card(QStringLiteral("预约时间"), QStringLiteral("每天 08:00–17:00，每次预约一个完整小时，可同时选择多台设备。"));
    auto *row = new QHBoxLayout;
    m_dateInput = new QDateEdit(labNow().date());
    m_dateInput->setMinimumDate(labNow().date());
    m_dateInput->setCalendarPopup(true);
    m_dateInput->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_hourInput = new QComboBox;
    for (int hour = 8; hour <= 16; ++hour) m_hourInput->addItem(hourlyLabel(hour), hour);
    row->addWidget(textLabel(QStringLiteral("预约日期")));
    row->addWidget(m_dateInput);
    row->addSpacing(16);
    row->addWidget(textLabel(QStringLiteral("一小时时段")));
    row->addWidget(m_hourInput);
    row->addStretch();
    auto *mine = actionButton(QStringLiteral("查看我的预约 →"));
    connect(mine, &QPushButton::clicked, this, [this] { navigateTo(m_profileIndex); });
    row->addWidget(mine);
    cardLayout(controls)->addLayout(row);
    layout->addWidget(controls);
    layout->addWidget(createDevicePlan(false));
    auto *selection = card(QStringLiteral("确认预约"));
    m_selectionSummary = textLabel(QStringLiteral("尚未选择设备"));
    m_labSummary = textLabel(QString(), QStringLiteral("muted"));
    cardLayout(selection)->addWidget(m_selectionSummary);
    cardLayout(selection)->addWidget(m_labSummary);
    auto *actions = new QHBoxLayout;
    auto *report = actionButton(QStringLiteral("报告设备故障"));
    m_reserveButton = actionButton(QStringLiteral("确认预约 →"), true);
    connect(report, &QPushButton::clicked, this, &MainWindow::reportSelectedIssue);
    connect(m_reserveButton, &QPushButton::clicked, this, &MainWindow::reserveSelectedSeats);
    actions->addWidget(report);
    actions->addStretch();
    actions->addWidget(m_reserveButton);
    cardLayout(selection)->addLayout(actions);
    layout->addWidget(selection);
    layout->addStretch();
    connect(m_dateInput, &QDateEdit::dateChanged, this, [this] { m_selectedDevices.clear(); refreshStudentSeats(); });
    connect(m_hourInput, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { m_selectedDevices.clear(); refreshStudentSeats(); });
}

void MainWindow::buildHoursPage()
{
    auto *layout = addSection(QStringLiteral("使用时长"), QStringLiteral("预约与实际使用分别统计，让每一次使用都有记录。"));
    auto *summary = card(QStringLiteral("我的时长"));
    m_hoursSummary = textLabel(QString(), QStringLiteral("metric"));
    cardLayout(summary)->addWidget(m_hoursSummary);
    cardLayout(summary)->addWidget(textLabel(QStringLiteral("预约时长按未取消记录计算，每条 1 小时。实际使用时长只累计已签到且已签退的记录；使用中的记录在签退后计入。"), QStringLiteral("muted")));
    auto *details = actionButton(QStringLiteral("在“我的”查看实际签到与签退时间 →"));
    connect(details, &QPushButton::clicked, this, [this] { navigateTo(m_profileIndex); });
    cardLayout(summary)->addWidget(details, 0, Qt::AlignLeft);
    layout->addWidget(summary);
    layout->addStretch();
}

void MainWindow::buildIssuesPage()
{
    const bool administrator = m_repository.isAdministrator();
    auto *layout = addSection(administrator ? QStringLiteral("故障维护") : QStringLiteral("我的故障记录"),
        administrator ? QStringLiteral("查看上报记录，处理设备故障并恢复使用。") : QStringLiteral("上报设备异常，在这里查看处理进度。"));
    auto *issues = card(administrator ? QStringLiteral("维修记录") : QStringLiteral("上报记录"));
    auto *actions = new QHBoxLayout;
    auto *report = actionButton(QStringLiteral("上报设备故障"), true);
    connect(report, &QPushButton::clicked, this, &MainWindow::reportSelectedIssue);
    actions->addWidget(report);
    actions->addStretch();
    if (administrator) {
        m_resolveButton = actionButton(QStringLiteral("解决选中故障"));
        connect(m_resolveButton, &QPushButton::clicked, this, &MainWindow::resolveSelectedIssue);
        actions->addWidget(m_resolveButton);
        auto *exportButton = actionButton(QStringLiteral("导出 error.txt"));
        connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportErrorFile);
        actions->addWidget(exportButton);
    }
    cardLayout(issues)->addLayout(actions);
    m_issueEmpty = textLabel(administrator ? QStringLiteral("暂无故障报告，设备维护记录会显示在这里。") : QStringLiteral("你还没有上报故障。遇到问题时，点击“上报设备故障”。"), QStringLiteral("muted"));
    cardLayout(issues)->addWidget(m_issueEmpty);
    m_issueTable = recordTable({QStringLiteral("设备"), QStringLiteral("故障描述"), QStringLiteral("上报人"), QStringLiteral("状态")});
    m_issueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    connect(m_issueTable, &QTableWidget::itemSelectionChanged, this, [this] {
        if (!m_resolveButton) return;
        const QString id = selectedRecord(m_issueTable);
        bool open = false;
        for (const IssueReport &issue : m_repository.issues()) if (issue.id == id) open = !issue.resolved;
        m_resolveButton->setEnabled(open);
    });
    cardLayout(issues)->addWidget(m_issueTable);
    layout->addWidget(issues);
    layout->addStretch();
}

void MainWindow::buildProfilePage()
{
    auto *layout = addSection(QStringLiteral("我的"), QStringLiteral("个人资料、预约记录与实际使用，都在这里。"));
    const User &user = *m_repository.currentUser();
    auto *profile = card(QString());
    profile->setObjectName(QStringLiteral("hero"));
    auto *identity = new QHBoxLayout;
    auto *avatar = textLabel(user.name.left(1));
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(60, 60);
    avatar->setStyleSheet(QStringLiteral("background:#dceaff; color:#3568d4; border-radius:30px; font-size:25px; font-weight:700;"));
    identity->addWidget(avatar);
    auto *copy = new QVBoxLayout;
    copy->addWidget(textLabel(user.name, QStringLiteral("cardTitle")));
    copy->addWidget(textLabel(QStringLiteral("%1 · %2").arg(m_repository.isAdministrator() ? QStringLiteral("管理员") : QStringLiteral("学生"), user.loginId), QStringLiteral("muted")));
    copy->addWidget(textLabel(QStringLiteral("本机账号 · 记录仅保存在本机"), QStringLiteral("muted")));
    identity->addLayout(copy, 1);
    cardLayout(profile)->addLayout(identity);
    layout->addWidget(profile);
    auto *today = card(QStringLiteral("今日预约"));
    m_todaySummary = textLabel(QString(), QStringLiteral("metric"));
    cardLayout(today)->addWidget(m_todaySummary);
    if (!m_repository.isAdministrator()) {
        auto *reserve = actionButton(QStringLiteral("去预约 →"));
        connect(reserve, &QPushButton::clicked, this, [this] { navigateTo(0); });
        cardLayout(today)->addWidget(reserve, 0, Qt::AlignLeft);
    }
    layout->addWidget(today);
    auto *preferences = card(QStringLiteral("预约偏好"), QStringLiteral("根据未取消的本机预约自动汇总；次数相同时显示编号或时段靠前的项目。"));
    m_preferenceSummary = textLabel(QString());
    cardLayout(preferences)->addWidget(m_preferenceSummary);
    layout->addWidget(preferences);
    auto *history = card(QStringLiteral("预约历史 · 签到与签退"), QStringLiteral("包含已取消记录。仅在预约时段内签到；未开始且未签到的预约可以取消。所有时间为北京时间。"));
    m_bookingEmpty = textLabel(m_repository.isAdministrator()
        ? QStringLiteral("暂无个人预约记录。管理员可在“总览”查看实验室预约，在“设备”“用户”“故障”中进行管理。")
        : QStringLiteral("还没有预约记录。前往“预约”选择设备和时段，你的记录会显示在这里。"), QStringLiteral("muted"));
    cardLayout(history)->addWidget(m_bookingEmpty);
    m_bookingTable = recordTable({QStringLiteral("日期"), QStringLiteral("预约时段"), QStringLiteral("设备"), QStringLiteral("状态"), QStringLiteral("实际签到"), QStringLiteral("实际签退"), QStringLiteral("实际时长")});
    m_bookingTable->setMinimumHeight(280);
    cardLayout(history)->addWidget(m_bookingTable);
    auto *actions = new QHBoxLayout;
    m_checkInButton = actionButton(QStringLiteral("签到"), true);
    m_checkOutButton = actionButton(QStringLiteral("签退"));
    m_cancelButton = actionButton(QStringLiteral("取消预约"));
    connect(m_checkInButton, &QPushButton::clicked, this, &MainWindow::checkInSelectedBooking);
    connect(m_checkOutButton, &QPushButton::clicked, this, &MainWindow::checkOutSelectedBooking);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelSelectedBooking);
    connect(m_bookingTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateAttendanceButtons);
    actions->addWidget(m_checkInButton);
    actions->addWidget(m_checkOutButton);
    actions->addStretch();
    actions->addWidget(m_cancelButton);
    cardLayout(history)->addLayout(actions);
    m_attendanceHint = textLabel(QStringLiteral("选中一条预约，查看可执行的操作。"), QStringLiteral("muted"));
    cardLayout(history)->addWidget(m_attendanceHint);
    layout->addWidget(history);
    layout->addStretch();
}

void MainWindow::buildOverviewPage()
{
    auto *layout = addSection(QStringLiteral("实验室总览"), QStringLiteral("本机运行概况、预约记录与管理文件导出。"));
    auto *summary = card(QStringLiteral("运行概况"));
    m_overviewSummary = textLabel(QString(), QStringLiteral("metric"));
    cardLayout(summary)->addWidget(m_overviewSummary);
    layout->addWidget(summary);
    auto *exports = card(QStringLiteral("管理文件导出"), QStringLiteral("导出今日各小时的预约数量，以及全部故障记录。"));
    auto *actions = new QHBoxLayout;
    auto *count = actionButton(QStringLiteral("导出 count.txt"), true);
    auto *errors = actionButton(QStringLiteral("导出 error.txt"));
    connect(count, &QPushButton::clicked, this, &MainWindow::exportCountFile);
    connect(errors, &QPushButton::clicked, this, &MainWindow::exportErrorFile);
    actions->addWidget(count);
    actions->addWidget(errors);
    actions->addStretch();
    cardLayout(exports)->addLayout(actions);
    layout->addWidget(exports);
    auto *records = card(QStringLiteral("全部预约"), QStringLiteral("包含取消记录；实际签到和签退与预约时段分别展示。无记录时表格为空。"));
    m_overviewTable = recordTable({QStringLiteral("日期"), QStringLiteral("时段"), QStringLiteral("设备"), QStringLiteral("预约人"), QStringLiteral("状态"), QStringLiteral("实际签到"), QStringLiteral("实际签退")});
    cardLayout(records)->addWidget(m_overviewTable);
    layout->addWidget(records);
    layout->addStretch();
}

void MainWindow::buildDevicesPage()
{
    auto *layout = addSection(QStringLiteral("设备管理"), QStringLiteral("点击设备可标记故障或恢复正常。此处展示设备维护状态，预约占用见“总览”。"));
    layout->addWidget(createDevicePlan(true));
    layout->addStretch();
}

void MainWindow::buildUsersPage()
{
    auto *layout = addSection(QStringLiteral("用户档案"), QStringLiteral("查看注册用户、身份与累计时长，管理学生账号。"));
    auto *users = card(QStringLiteral("已注册用户"), QStringLiteral("已结束预约时长按时段计算；实际使用时长按已完成的签到签退记录计算。"));
    m_userTable = recordTable({QStringLiteral("姓名"), QStringLiteral("身份"), QStringLiteral("学号 / 工号"), QStringLiteral("已结束预约时长"), QStringLiteral("实际使用时长")});
    cardLayout(users)->addWidget(m_userTable);
    m_deactivateButton = actionButton(QStringLiteral("注销选中学生账号"));
    connect(m_deactivateButton, &QPushButton::clicked, this, &MainWindow::deactivateSelectedStudent);
    connect(m_userTable, &QTableWidget::itemSelectionChanged, this, [this] {
        const QString id = selectedRecord(m_userTable);
        bool student = false;
        for (const User &user : m_repository.users()) if (user.id == id) student = user.role == UserRole::Student;
        m_deactivateButton->setEnabled(student);
    });
    cardLayout(users)->addWidget(m_deactivateButton, 0, Qt::AlignRight);
    layout->addWidget(users);
    layout->addStretch();
}

QWidget *MainWindow::createDevicePlan(const bool administratorMode)
{
    auto *plan = card(QStringLiteral("实验室平面图 · 40 台设备"), administratorMode
        ? QStringLiteral("正常 / 故障 · 点击设备切换维护状态")
        : QStringLiteral("空闲 / 已预约 / 故障 / 已选择 · 点击空闲设备，可同时选择多个位置"));
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);
    for (int number = 1; number <= 40; ++number) {
        auto *button = new QToolButton;
        button->setProperty("deviceNumber", number);
        button->setMinimumSize(78, 62);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        button->setCursor(Qt::PointingHandCursor);
        styleDeviceButton(button, number, administratorMode);
        if (administratorMode) connect(button, &QToolButton::clicked, this, &MainWindow::toggleAdministratorDevice);
        else connect(button, &QToolButton::clicked, this, [this, button, number] {
            if (!m_dateInput || !m_hourInput || m_repository.isUnavailable(number, m_dateInput->date(), m_hourInput->currentData().toInt())) return;
            if (m_selectedDevices.contains(number)) m_selectedDevices.remove(number);
            else m_selectedDevices.insert(number);
            styleDeviceButton(button, number, false);
            updateSelectionSummary();
        });
        grid->addWidget(button, (number - 1) / 8, (number - 1) % 8);
        m_deviceButtons.append(button);
    }
    cardLayout(plan)->addLayout(grid);
    return plan;
}

void MainWindow::styleDeviceButton(QToolButton *button, const int deviceNumber, const bool administratorMode)
{
    bool faulty = false;
    for (const Device &device : m_repository.devices()) if (device.number == deviceNumber) faulty = device.faulty;
    const bool unavailable = !administratorMode && m_dateInput && m_hourInput
        && m_repository.isUnavailable(deviceNumber, m_dateInput->date(), m_hourInput->currentData().toInt());
    const bool selected = !administratorMode && m_selectedDevices.contains(deviceNumber);
    const QString state = faulty ? QStringLiteral("故障") : unavailable ? QStringLiteral("已预约")
        : selected ? QStringLiteral("已选择") : administratorMode ? QStringLiteral("正常") : QStringLiteral("空闲");
    const QString color = faulty ? QStringLiteral("#c54b52") : unavailable ? QStringLiteral("#8490a2")
        : selected ? QStringLiteral("#3568d4") : QStringLiteral("#337b83");
    const QString background = faulty ? QStringLiteral("#fff1f2") : unavailable ? QStringLiteral("#edf1f6")
        : selected ? QStringLiteral("#e4edff") : QStringLiteral("#f2fafb");
    button->setText(QStringLiteral("%1\n%2").arg(seatLabel(deviceNumber), state));
    button->setAccessibleName(QStringLiteral("设备 %1，%2").arg(deviceNumber).arg(state));
    button->setToolTip(QStringLiteral("设备 %1 · %2").arg(deviceNumber).arg(state));
    button->setEnabled(administratorMode || (!faulty && !unavailable));
    button->setStyleSheet(QStringLiteral("QToolButton { color:%1; background:%2; border:1px solid %3; border-radius:12px; font-weight:600; padding:7px; } QToolButton:hover:enabled { border:1px solid #5686ff; background:#edf4ff; }")
        .arg(color, background, selected ? QStringLiteral("#5686ff") : QStringLiteral("#dfe7f1")));
}

void MainWindow::refreshStudentSeats()
{
    if (!m_dateInput || !m_hourInput) return;
    if (m_dateInput->minimumDate() != labNow().date()) {
        const QSignalBlocker blocker(m_dateInput);
        const QDate previousDate = m_dateInput->date();
        m_dateInput->setMinimumDate(labNow().date());
        if (previousDate != m_dateInput->date()) m_selectedDevices.clear();
    }
    // Passive refresh must not discard a user's still-available multi-seat selection.
    for (auto it = m_selectedDevices.begin(); it != m_selectedDevices.end();) {
        if (m_repository.isUnavailable(*it, m_dateInput->date(), m_hourInput->currentData().toInt())) it = m_selectedDevices.erase(it);
        else ++it;
    }
    for (QToolButton *button : m_deviceButtons) styleDeviceButton(button, button->property("deviceNumber").toInt(), false);
    updateSelectionSummary();
}

void MainWindow::updateSelectionSummary()
{
    if (!m_selectionSummary || !m_dateInput || !m_hourInput) return;
    QList<int> seats = m_selectedDevices.values();
    std::sort(seats.begin(), seats.end());
    QStringList labels;
    for (int seat : seats) labels.append(seatLabel(seat));
    m_selectionSummary->setText(seats.isEmpty() ? QStringLiteral("尚未选择设备。点击上方空闲设备开始预约。")
        : QStringLiteral("已选择 %1 台：%2\n%3 · %4").arg(seats.size()).arg(labels.join(QStringLiteral("、")), m_dateInput->date().toString(Qt::ISODate), m_hourInput->currentText()));
    int available = 0;
    int faulty = 0;
    for (const Device &device : m_repository.devices()) {
        if (device.faulty) ++faulty;
        if (!m_repository.isUnavailable(device.number, m_dateInput->date(), m_hourInput->currentData().toInt())) ++available;
    }
    m_labSummary->setText(QStringLiteral("当前所选时段：%1 / 40 台设备可预约 · %2 台故障").arg(available).arg(faulty));
    if (m_reserveButton) m_reserveButton->setEnabled(!seats.isEmpty());
}

void MainWindow::reserveSelectedSeats()
{
    if (!m_dateInput || !m_hourInput) return;
    QString message;
    QList<int> devices = m_selectedDevices.values();
    std::sort(devices.begin(), devices.end());
    const bool ok = m_repository.reserve(devices, m_dateInput->date(), m_hourInput->currentData().toInt(), &message);
    if (ok) m_selectedDevices.clear();
    refreshDashboard();
    showMessage(message, !ok);
}

QList<Booking> MainWindow::personalBookings() const
{
    QList<Booking> result;
    const User *user = m_repository.currentUser();
    if (!user) return result;
    // bookingsForUser intentionally hides cancelled records; history must retain them.
    for (const Booking &booking : m_repository.bookings()) if (booking.userId == user->id) result.append(booking);
    std::stable_sort(result.begin(), result.end(), [](const Booking &a, const Booking &b) {
        return a.date == b.date ? a.startHour > b.startHour : a.date > b.date;
    });
    return result;
}

void MainWindow::refreshDashboard()
{
    if (!m_sections || !m_repository.currentUser()) return;
    if (m_repository.isAdministrator()) {
        for (QToolButton *button : m_deviceButtons) styleDeviceButton(button, button->property("deviceNumber").toInt(), true);
        refreshOverview();
        refreshUserTable();
    } else refreshStudentSeats();
    refreshBookingList();
    refreshProfileSummary();
    refreshIssueTable();
}

void MainWindow::refreshBookingList()
{
    if (!m_bookingTable || !m_repository.currentUser()) return;
    const TablePosition position(m_bookingTable);
    {
        const QSignalBlocker blocker(m_bookingTable);
        const QList<Booking> bookings = personalBookings();
        m_bookingTable->setRowCount(static_cast<int>(bookings.size()));
        for (int row = 0; row < bookings.size(); ++row) {
            const Booking &booking = bookings.at(row);
            setRecord(m_bookingTable, row, booking.id, {booking.date.toString(Qt::ISODate), hourlyLabel(booking.startHour),
                seatLabel(booking.deviceNumber), bookingState(booking), attendanceTime(booking.checkedInAt),
                attendanceTime(booking.checkedOutAt), actualDuration(booking)});
        }
        m_bookingEmpty->setVisible(bookings.isEmpty());
        m_bookingTable->setVisible(!bookings.isEmpty());
        position.restore(m_bookingTable);
    }
    updateAttendanceButtons();
}

void MainWindow::refreshProfileSummary()
{
    if (!m_todaySummary || !m_preferenceSummary) return;
    const QList<Booking> bookings = personalBookings();
    QList<Booking> todayBookings;
    int active = 0;
    int future = 0;
    int cancelled = 0;
    int completed = 0;
    int inUse = 0;
    qint64 actualSeconds = 0;
    QMap<int, int> devices;
    QMap<int, int> hours;
    for (const Booking &booking : bookings) {
        if (booking.cancelled) { ++cancelled; continue; }
        ++active;
        ++devices[booking.deviceNumber];
        ++hours[booking.startHour];
        if (booking.date == labNow().date()) todayBookings.append(booking);
        if (bookingStart(booking) > labNow()) ++future;
        if (booking.checkedInAt.isValid() && booking.checkedOutAt.isValid()) {
            ++completed;
            actualSeconds += completedSeconds(booking);
        } else if (booking.checkedInAt.isValid()) ++inUse;
    }
    std::stable_sort(todayBookings.begin(), todayBookings.end(), [](const Booking &a, const Booking &b) { return a.startHour < b.startHour; });
    QStringList todayLines;
    for (const Booking &booking : todayBookings) todayLines.append(QStringLiteral("%1 · %2 · %3").arg(seatLabel(booking.deviceNumber), hourlyLabel(booking.startHour), bookingState(booking)));
    m_todaySummary->setText(todayBookings.isEmpty()
        ? QStringLiteral("%1 · 今天还没有预约\n%2").arg(labNow().date().toString(QStringLiteral("yyyy 年 MM 月 dd 日")),
            m_repository.isAdministrator() ? QStringLiteral("个人预约为空，实验室预约可在总览查看。") : QStringLiteral("选好设备和时段后，会显示在这里。"))
        : QStringLiteral("%1 · %2 个时段（不含已取消）\n%3").arg(labNow().date().toString(QStringLiteral("yyyy 年 MM 月 dd 日"))).arg(todayBookings.size()).arg(todayLines.join(QLatin1Char('\n'))));
    m_preferenceSummary->setText(QStringLiteral("常用设备    %1\n常用时段    %2\n未来预约    %3 个时段\n全部记录    %4 条，其中已取消 %5 条")
        .arg(favoriteText(devices, true), favoriteText(hours, false)).arg(future).arg(bookings.size()).arg(cancelled));
    if (m_hoursSummary) m_hoursSummary->setText(QStringLiteral("累计预约    %1 小时\n实际使用    %2\n已完成签到签退    %3 条\n正在使用    %4 条\n已取消    %5 条")
        .arg(active).arg(durationText(actualSeconds)).arg(completed).arg(inUse).arg(cancelled));
}

QString MainWindow::selectedBookingId() const
{
    return selectedRecord(m_bookingTable);
}

void MainWindow::updateAttendanceButtons()
{
    if (!m_checkInButton || !m_checkOutButton || !m_cancelButton) return;
    m_checkInButton->setEnabled(false);
    m_checkOutButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    m_attendanceHint->setText(QStringLiteral("选中一条预约，查看可执行的操作。"));
    const QString id = selectedBookingId();
    for (const Booking &booking : personalBookings()) {
        if (booking.id != id) continue;
        m_checkInButton->setEnabled(canCheckIn(booking));
        m_checkOutButton->setEnabled(canCheckOut(booking));
        m_cancelButton->setEnabled(canCancel(booking));
        QString hint;
        if (booking.cancelled) hint = QStringLiteral("此预约已取消，设备时段已释放。记录保留在历史中。");
        else if (booking.checkedOutAt.isValid()) hint = QStringLiteral("已签退 · 实际使用 %1").arg(actualDuration(booking));
        else if (canCheckOut(booking)) hint = QStringLiteral("已签到。使用结束后请签退，保存实际使用时长。");
        else if (canCheckIn(booking)) hint = QStringLiteral("当前处于预约时段，可以签到。");
        else if (canCancel(booking)) hint = QStringLiteral("预约尚未开始。可取消预约，或在预约时段内签到。");
        else hint = QStringLiteral("预约时段已结束，无法补签到或取消。");
        m_attendanceHint->setText(QStringLiteral("%1 · %2 · %3\n%4").arg(booking.date.toString(Qt::ISODate), hourlyLabel(booking.startHour), seatLabel(booking.deviceNumber), hint));
        break;
    }
}

void MainWindow::checkInSelectedBooking()
{
    const QString id = selectedBookingId();
    if (id.isEmpty()) return;
    QString message;
    const bool ok = m_repository.checkIn(id, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::checkOutSelectedBooking()
{
    const QString id = selectedBookingId();
    if (id.isEmpty()) return;
    QString message;
    const bool ok = m_repository.checkOut(id, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::cancelSelectedBooking()
{
    const QString id = selectedBookingId();
    if (id.isEmpty()) { showMessage(QStringLiteral("请先选择一条预约。"), true); return; }
    QString message;
    const bool ok = m_repository.cancel(id, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::reportSelectedIssue()
{
    if (!m_repository.currentUser()) return;
    const bool reservationPage = !m_repository.isAdministrator() && m_sections && m_sections->currentIndex() == 0;
    if (reservationPage && m_selectedDevices.size() > 1) {
        showMessage(QStringLiteral("上报故障时请只选择一台设备，或前往“故障”页选择设备编号。"), true);
        return;
    }
    // The issues page also permits reporting occupied or faulty seats.
    const int suggested = m_selectedDevices.size() == 1 ? *m_selectedDevices.cbegin() : 1;
    QInputDialog deviceDialog(this);
    deviceDialog.setWindowTitle(QStringLiteral("上报设备故障"));
    deviceDialog.setLabelText(QStringLiteral("设备编号（1–40）"));
    deviceDialog.setInputMode(QInputDialog::IntInput);
    deviceDialog.setIntRange(1, 40);
    deviceDialog.setIntValue(suggested);
    deviceDialog.setOkButtonText(QStringLiteral("下一步"));
    deviceDialog.setCancelButtonText(QStringLiteral("取消"));
    if (deviceDialog.exec() != QDialog::Accepted) return;
    const int number = deviceDialog.intValue();
    QInputDialog descriptionDialog(this);
    descriptionDialog.setWindowTitle(QStringLiteral("上报设备故障"));
    descriptionDialog.setLabelText(QStringLiteral("%1 · 请描述故障现象").arg(seatLabel(number)));
    descriptionDialog.setInputMode(QInputDialog::TextInput);
    descriptionDialog.setOption(QInputDialog::UsePlainTextEditForTextInput);
    descriptionDialog.setOkButtonText(QStringLiteral("提交报告"));
    descriptionDialog.setCancelButtonText(QStringLiteral("取消"));
    if (descriptionDialog.exec() != QDialog::Accepted) return;
    const QString description = descriptionDialog.textValue();
    QString message;
    const bool ok = m_repository.reportIssue(number, description, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::refreshIssueTable()
{
    if (!m_issueTable || !m_repository.currentUser()) return;
    const TablePosition position(m_issueTable);
    const QSignalBlocker blocker(m_issueTable);
    QList<IssueReport> visible;
    for (const IssueReport &issue : m_repository.issues()) {
        if (m_repository.isAdministrator() || issue.reporterId == m_repository.currentUser()->id) visible.append(issue);
    }
    m_issueTable->setRowCount(static_cast<int>(visible.size()));
    for (int row = 0; row < visible.size(); ++row) {
        const IssueReport &issue = visible.at(visible.size() - 1 - row);
        QString reporter = QStringLiteral("已注销账号");
        for (const User &user : m_repository.users()) if (user.id == issue.reporterId) reporter = user.name;
        setRecord(m_issueTable, row, issue.id, {seatLabel(issue.deviceNumber), issue.description, reporter,
            issue.resolved ? QStringLiteral("已解决") : QStringLiteral("待处理")});
    }
    m_issueEmpty->setVisible(visible.isEmpty());
    m_issueTable->setVisible(!visible.isEmpty());
    position.restore(m_issueTable);
    if (m_resolveButton) {
        bool open = false;
        for (const IssueReport &issue : visible) if (issue.id == selectedRecord(m_issueTable)) open = !issue.resolved;
        m_resolveButton->setEnabled(open);
    }
}

void MainWindow::refreshUserTable()
{
    if (!m_userTable) return;
    const TablePosition position(m_userTable);
    const QSignalBlocker blocker(m_userTable);
    m_userTable->setRowCount(static_cast<int>(m_repository.users().size()));
    for (int row = 0; row < m_repository.users().size(); ++row) {
        const User &user = m_repository.users().at(row);
        qint64 seconds = 0;
        for (const Booking &booking : m_repository.bookings()) if (booking.userId == user.id && !booking.cancelled) seconds += completedSeconds(booking);
        setRecord(m_userTable, row, user.id, {user.name, user.role == UserRole::Administrator ? QStringLiteral("管理员") : QStringLiteral("学生"), user.loginId,
            QStringLiteral("%1 小时").arg(m_repository.bookedHoursForUser(user.id)), durationText(seconds)});
    }
    position.restore(m_userTable);
    bool student = false;
    for (const User &user : m_repository.users()) if (user.id == selectedRecord(m_userTable)) student = user.role == UserRole::Student;
    m_deactivateButton->setEnabled(student);
}

void MainWindow::refreshOverview()
{
    if (!m_overviewSummary || !m_overviewTable) return;
    int faulty = 0;
    int open = 0;
    for (const Device &device : m_repository.devices()) if (device.faulty) ++faulty;
    for (const IssueReport &issue : m_repository.issues()) if (!issue.resolved) ++open;
    m_overviewSummary->setText(QStringLiteral("注册用户  %1    当前时段已签到  %2    本周预约  %3\n正常设备  %4 / 40    故障设备  %5    待处理报告  %6")
        .arg(m_repository.users().size()).arg(m_repository.currentLabUsers()).arg(m_repository.currentWeekReservations())
        .arg(40 - faulty).arg(faulty).arg(open));
    const TablePosition position(m_overviewTable);
    const QSignalBlocker blocker(m_overviewTable);
    QList<Booking> bookings = m_repository.bookings();
    std::stable_sort(bookings.begin(), bookings.end(), [](const Booking &a, const Booking &b) {
        return a.date == b.date ? a.startHour > b.startHour : a.date > b.date;
    });
    m_overviewTable->setRowCount(static_cast<int>(bookings.size()));
    for (int row = 0; row < bookings.size(); ++row) {
        const Booking &booking = bookings.at(row);
        QString owner = QStringLiteral("已注销账号");
        for (const User &user : m_repository.users()) if (user.id == booking.userId) owner = QStringLiteral("%1 · %2").arg(user.name, user.loginId);
        setRecord(m_overviewTable, row, booking.id, {booking.date.toString(Qt::ISODate), hourlyLabel(booking.startHour), seatLabel(booking.deviceNumber), owner,
            bookingState(booking), attendanceTime(booking.checkedInAt), attendanceTime(booking.checkedOutAt)});
    }
    position.restore(m_overviewTable);
}

void MainWindow::toggleAdministratorDevice()
{
    auto *button = qobject_cast<QToolButton *>(sender());
    if (!button || !m_repository.isAdministrator()) return;
    const int deviceNumber = button->property("deviceNumber").toInt();
    bool faulty = false;
    for (const Device &device : m_repository.devices()) if (device.number == deviceNumber) faulty = device.faulty;
    QString message;
    const bool ok = m_repository.setFault(deviceNumber, !faulty, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::exportCountFile()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出今日预约统计"), QStringLiteral("count.txt"), QStringLiteral("文本文件 (*.txt)"));
    if (path.isEmpty()) return;
    QString message;
    const bool ok = m_repository.exportCountFile(path, labNow().date(), &message);
    showMessage(message, !ok);
}

void MainWindow::exportErrorFile()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出故障报告"), QStringLiteral("error.txt"), QStringLiteral("文本文件 (*.txt)"));
    if (path.isEmpty()) return;
    QString message;
    const bool ok = m_repository.exportErrorFile(path, &message);
    showMessage(message, !ok);
}

void MainWindow::deactivateSelectedStudent()
{
    const QString id = selectedRecord(m_userTable);
    if (id.isEmpty()) { showMessage(QStringLiteral("请先选择一个学生账号。"), true); return; }
    const QString name = m_userTable->item(m_userTable->currentRow(), 0)->text();
    QMessageBox confirmation(QMessageBox::Warning, QStringLiteral("注销学生账号"),
        QStringLiteral("确定注销“%1”的账号？该学生的全部预约记录也将被移除。").arg(name), QMessageBox::NoButton, this);
    confirmation.setTextFormat(Qt::PlainText);
    auto *confirm = confirmation.addButton(QStringLiteral("确认注销"), QMessageBox::DestructiveRole);
    auto *cancel = confirmation.addButton(QStringLiteral("返回"), QMessageBox::RejectRole);
    confirmation.setDefaultButton(cancel);
    confirmation.exec();
    if (confirmation.clickedButton() != confirm) return;
    QString message;
    const bool ok = m_repository.deactivateStudent(id, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::resolveSelectedIssue()
{
    const QString id = selectedRecord(m_issueTable);
    if (id.isEmpty()) { showMessage(QStringLiteral("请先选择一条待处理故障。"), true); return; }
    QString message;
    const bool ok = m_repository.resolveIssue(id, &message);
    refreshDashboard();
    showMessage(message, !ok);
}

void MainWindow::showMessage(const QString &message, const bool error)
{
    if (!m_statusLabel) return;
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(error ? QStringLiteral("color:#c54b52; font-weight:600;") : QStringLiteral("color:#337b83; font-weight:600;"));
}
