#include "ui/MainWindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QEvent>
#include <QTimer>
#include <QUuid>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) qFatal("%s", message);
}
QPushButton *button(MainWindow &window, const QString &text)
{
    for (auto *candidate : window.findChildren<QPushButton *>()) {
        if (candidate->text() == text && candidate->isVisible()) return candidate;
    }
    qFatal("Visible button not found: %s", qPrintable(text));
    return nullptr;
}
void click(MainWindow &window, const QString &text)
{
    auto *target = button(window, text);
    require(target->isEnabled(), "Expected enabled button");
    target->click();
    QApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
void login(MainWindow &window)
{
    for (auto *input : window.findChildren<QLineEdit *>()) {
        if (input->placeholderText() == QStringLiteral("请输入学号或管理员账号")) input->setText(QStringLiteral("student"));
        if (input->echoMode() == QLineEdit::Password) input->setText(QStringLiteral("pass1234"));
    }
    click(window, QStringLiteral("登录并继续"));
}
QTableWidget *history(MainWindow &window)
{
    for (auto *table : window.findChildren<QTableWidget *>()) {
        if (table->columnCount() == 7 && table->horizontalHeaderItem(4)->text() == QStringLiteral("实际签到") && table->isVisible()) return table;
    }
    qFatal("Profile history is not visible");
    return nullptr;
}
void select(QTableWidget *table, const QString &id)
{
    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 0)->data(Qt::UserRole).toString() == id) {
            table->setCurrentCell(row, 0);
            return;
        }
    }
    qFatal("Missing booking: %s", qPrintable(id));
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    // A unique application identity isolates the fixture from real user records.
    app.setApplicationName(QStringLiteral("LabWindowCheck-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString store = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    require(QDir().mkpath(store), "Cannot create isolated test directory");
    const QDate tomorrow = labNow().date().addDays(1);
    const QDate yesterday = labNow().date().addDays(-1);
    QJsonArray bookings;
    auto add = [&](const QString &id, const QDate &date, bool cancelled, const QString &in, const QString &out) {
        bookings.append(QJsonObject{{"id", id}, {"deviceNumber", 1}, {"date", date.toString(Qt::ISODate)},
            {"startHour", 8}, {"userId", "student-id"}, {"cancelled", cancelled}, {"checkedInAt", in}, {"checkedOutAt", out}});
    };
    add(QStringLiteral("future"), tomorrow, false, {}, {});
    add(QStringLiteral("cancelled"), tomorrow, true, {}, {});
    const QDateTime start(yesterday, QTime(8, 0), QTimeZone("Asia/Shanghai"));
    add(QStringLiteral("completed"), yesterday, false, start.toUTC().toString(Qt::ISODateWithMs), start.addSecs(600).toUTC().toString(Qt::ISODateWithMs));
    QFile fixture(store + QStringLiteral("/lab-demo.json"));
    require(fixture.open(QIODevice::WriteOnly), "Cannot write fixture");
    fixture.write(QJsonDocument(QJsonObject{
        {"users", QJsonArray{QJsonObject{{"id", "student-id"}, {"name", "测试学生"}, {"loginId", "student"}, {"password", "pass1234"}, {"role", "student"}}}},
        {"bookings", bookings}, {"devices", QJsonArray{}}, {"issues", QJsonArray{}}
    }).toJson());
    fixture.close();
    {
        MainWindow window;
        window.show();
        QApplication::processEvents();
        login(window);
        click(window, QStringLiteral("我的"));
        auto *table = history(window);
        require(table->rowCount() == 3, "History must include cancelled reservations");
        select(table, QStringLiteral("completed"));
        require(table->item(table->currentRow(), 6)->text() == QStringLiteral("0 小时 10 分 0 秒"), "Actual duration must use attendance timestamps");
        require(!button(window, QStringLiteral("签到"))->isEnabled(), "Completed booking cannot check in");
        require(!button(window, QStringLiteral("签退"))->isEnabled(), "Completed booking cannot check out");
        select(table, QStringLiteral("cancelled"));
        require(!button(window, QStringLiteral("取消预约"))->isEnabled(), "Cancelled booking cannot be cancelled again");
        select(table, QStringLiteral("future"));
        require(!button(window, QStringLiteral("签到"))->isEnabled(), "Future booking cannot check in early");
        click(window, QStringLiteral("刷新"));
        require(table->item(table->currentRow(), 0)->data(Qt::UserRole).toString() == QStringLiteral("future"), "Refresh must retain selected reservation");
        click(window, QStringLiteral("取消预约"));
        require(table->rowCount() == 3, "Cancellation must retain history");
        require(table->item(table->currentRow(), 3)->text() == QStringLiteral("已取消"), "Cancellation must update status");
        click(window, QStringLiteral("时长"));
        click(window, QStringLiteral("故障"));
        click(window, QStringLiteral("预约"));
        click(window, QStringLiteral("退出"));
        login(window);
        click(window, QStringLiteral("我的"));
        require(history(window)->rowCount() == 3, "Relogin must rebuild complete history");
        int activeTimers = 0;
        for (auto *timer : window.findChildren<QTimer *>()) if (timer->isActive() && timer->interval() == 30000) ++activeTimers;
        require(activeTimers == 1, "Relogin must leave one refresh timer");
    }
    // Only the uniquely named test application directory created above is removed.
    require(QDir(store).removeRecursively(), "Cannot clean isolated test directory");
    return 0;
}
