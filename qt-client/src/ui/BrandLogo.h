#pragma once
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QXmlStreamReader>

// Decode the supplied PNG embedded in the shared web favicon without requiring Qt SVG plugins.
inline QIcon labBrandIcon()
{
    QFile file(QStringLiteral(":/brand/lab-logo.svg"));
    if (!file.open(QIODevice::ReadOnly)) return {};
    QXmlStreamReader xml(file.readAll());
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QStringLiteral("image")) continue;
        const QString href = xml.attributes().value(QStringLiteral("href")).toString();
        const QString prefix = QStringLiteral("data:image/png;base64,");
        if (!href.startsWith(prefix)) continue;
        QPixmap pixmap;
        if (pixmap.loadFromData(QByteArray::fromBase64(href.mid(prefix.size()).toLatin1()), "PNG")) return QIcon(pixmap);
    }
    return {};
}
