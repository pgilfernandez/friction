#include "frictionplugin.h"

#include <QCoreApplication>
#include <QPluginLoader>
#include <QSet>
#include <QTextStream>

class ProbeHost final : public Friction::Plugins::Host
{
public:
    QWidget *mainWindow() const override { return nullptr; }
    Document *document() const override { return nullptr; }
    Canvas *activeScene() const override { return nullptr; }
    void actionFinished() override {}
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream error(stderr);
    QSet<QString> ids;
    ProbeHost host;

    for (int i = 1; i < argc; ++i) {
        QPluginLoader loader(QString::fromLocal8Bit(argv[i]));
        QObject *instance = loader.instance();
        if (!instance) {
            error << argv[i] << ": " << loader.errorString() << Qt::endl;
            return 1;
        }
        auto plugin = qobject_cast<Friction::Plugins::Plugin *>(instance);
        if (!plugin) {
            error << argv[i] << ": not a Friction plugin" << Qt::endl;
            return 2;
        }
        if (plugin->apiVersion() != Friction::Plugins::ApiVersion ||
                plugin->id().isEmpty() || ids.contains(plugin->id())) {
            error << argv[i] << ": invalid API version or id" << Qt::endl;
            return 3;
        }
        QString initializeError;
        if (!plugin->initialize(&host, &initializeError)) {
            error << argv[i] << ": " << initializeError << Qt::endl;
            return 4;
        }
        QSet<QString> actionIds;
        for (const auto &action : plugin->actions()) {
            if (action.id.isEmpty() || action.text.isEmpty() ||
                    actionIds.contains(action.id)) {
                error << argv[i] << ": invalid or duplicate action" << Qt::endl;
                return 5;
            }
            actionIds.insert(action.id);
        }
        if (actionIds.isEmpty()) {
            error << argv[i] << ": plugin has no actions" << Qt::endl;
            return 6;
        }
        ids.insert(plugin->id());
        plugin->shutdown();
        loader.unload();
    }

    return ids.size() == argc - 1 ? 0 : 7;
}
