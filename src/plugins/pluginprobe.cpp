// SPDX-License-Identifier: GPL-3.0-only

#include "coreplugininterface.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QPluginLoader>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        qCritical() << "usage:" << argv[0] << "PLUGIN";
        return 2;
    }

    QPluginLoader loader(QFileInfo(QString::fromLocal8Bit(argv[1]))
                         .absoluteFilePath());
    QObject *instance = loader.instance();
    if (!instance) {
        qCritical() << loader.errorString();
        return 1;
    }

    auto *plugin = qobject_cast<FrictionCorePluginInterface*>(instance);
    if (!plugin) {
        qCritical() << "Plugin does not implement"
                    << FrictionCorePluginInterface_iid;
        return 1;
    }

    const QJsonObject metadata = loader.metaData().value("MetaData").toObject();
    qInfo().noquote() << metadata.value("id").toString();
    return 0;
}
