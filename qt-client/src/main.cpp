#include "ui/MainWindow.h"

#include <QApplication>
#include "ui/BrandLogo.h"

/**
 * @brief Starts the native Windows Qt application.
 * @param argc Standard process argument count supplied by the operating system.
 * @param argv Standard process argument values supplied by the operating system.
 * @return The Qt event-loop exit code.
 */
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("IntelligentLabReservationQt"));
    QApplication::setOrganizationName(QStringLiteral("NJUPT"));

    application.setWindowIcon(labBrandIcon());

    MainWindow window;
    window.show();
    return application.exec();
}
