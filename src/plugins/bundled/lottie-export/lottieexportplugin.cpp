#include "frictionplugin.h"

#include "appsupport.h"
#include "exportlottiedialog.h"

#include <QObject>
#include <QWidget>

class LottieExportPlugin final : public QObject, public Friction::Plugins::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FRICTION_PLUGIN_IID FILE "metadata.json")
    Q_INTERFACES(Friction::Plugins::Plugin)

public:
    int apiVersion() const override { return Friction::Plugins::ApiVersion; }
    QString id() const override { return QStringLiteral("graphics.friction.lottie-export"); }
    QString name() const override { return tr("Lottie Export"); }
    QString version() const override { return QStringLiteral("1.0.0"); }

    QList<Friction::Plugins::Action> actions() const override
    {
        Friction::Plugins::Action preview;
        preview.id = QStringLiteral("preview");
        preview.text = tr("Preview Lottie");
        preview.toolTip = tr("Preview Lottie animation in a web browser");
        preview.iconName = QStringLiteral("seq_preview");
        preview.shortcut = QKeySequence(AppSupport::getSettings(
                    "shortcuts", "previewLottie", "Alt+Ctrl+F12").toString());
        preview.menu = Friction::Plugins::MenuLocation::Export;
        preview.showInToolBar = true;

        Friction::Plugins::Action exportAction;
        exportAction.id = QStringLiteral("export");
        exportAction.text = tr("Export Lottie");
        exportAction.toolTip = tr("Export a Lottie or dotLottie animation");
        exportAction.iconName = QStringLiteral("output");
        exportAction.shortcut = QKeySequence(AppSupport::getSettings(
                    "shortcuts", "exportLottie", "Alt+Shift+F12").toString());
        exportAction.menu = Friction::Plugins::MenuLocation::Export;
        exportAction.showInToolBar = true;
        return {preview, exportAction};
    }

    bool initialize(Friction::Plugins::Host *host,
                    QString *errorMessage) override
    {
        Q_UNUSED(errorMessage)
        mHost = host;
        return mHost;
    }

    void triggerAction(const QString &actionId) override
    {
        const bool preview = actionId == QStringLiteral("preview");
        const QString warning = preview ? QString() : tr(
            "Lottie export is a native exporter. Some Friction features "
            "cannot be represented by the Lottie format.");
        auto dialog = new ExportLottieDialog(mHost->mainWindow(), warning);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        if (preview) { dialog->showPreview(true); }
        else { dialog->show(); }
    }

    void shutdown() override { mHost = nullptr; }

private:
    Friction::Plugins::Host *mHost = nullptr;
};

#include "lottieexportplugin.moc"
