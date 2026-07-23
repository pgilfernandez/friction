#include "frictionplugin.h"

#include <QMessageBox>
#include <QObject>

class MinimalPlugin final : public QObject, public Friction::Plugins::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FRICTION_PLUGIN_IID FILE "metadata.json")
    Q_INTERFACES(Friction::Plugins::Plugin)

public:
    int apiVersion() const override { return Friction::Plugins::ApiVersion; }
    QString id() const override { return QStringLiteral("example.friction.minimal"); }
    QString name() const override { return tr("Minimal Friction Plugin"); }
    QString version() const override { return QStringLiteral("1.0.0"); }

    QList<Friction::Plugins::Action> actions() const override
    {
        Friction::Plugins::Action action;
        action.id = QStringLiteral("hello");
        action.text = tr("Hello from Plugin");
        action.toolTip = tr("Run the minimal plugin example");
        action.menu = Friction::Plugins::MenuLocation::Help;
        action.requiresActiveScene = false;
        return {action};
    }

    bool initialize(Friction::Plugins::Host *host, QString *error) override
    {
        Q_UNUSED(error)
        mHost = host;
        return mHost;
    }

    void triggerAction(const QString &actionId) override
    {
        if (actionId == QStringLiteral("hello")) {
            QMessageBox::information(mHost->mainWindow(), name(),
                                     tr("The plugin was loaded without rebuilding Friction."));
        }
    }

    void shutdown() override { mHost = nullptr; }

private:
    Friction::Plugins::Host *mHost = nullptr;
};

#include "minimalplugin.moc"
