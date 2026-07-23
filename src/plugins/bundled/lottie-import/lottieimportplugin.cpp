#include "frictionplugin.h"

#include "Animators/transformanimator.h"
#include "Boxes/containerbox.h"
#include "Private/document.h"
#include "appsupport.h"
#include "canvas.h"
#include "lottie/lottieimporter.h"
#include "lottieimportdialog.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QWidget>

class LottieImportPlugin final : public QObject, public Friction::Plugins::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FRICTION_PLUGIN_IID FILE "metadata.json")
    Q_INTERFACES(Friction::Plugins::Plugin)

public:
    int apiVersion() const override { return Friction::Plugins::ApiVersion; }
    QString id() const override { return QStringLiteral("graphics.friction.lottie-import"); }
    QString name() const override { return tr("Lottie Import"); }
    QString version() const override { return QStringLiteral("1.0.0"); }

    QList<Friction::Plugins::Action> actions() const override
    {
        Friction::Plugins::Action action;
        action.id = QStringLiteral("import");
        action.text = tr("Import Lottie Animation");
        action.toolTip = tr("Import a Lottie or dotLottie animation");
        action.iconName = QStringLiteral("file_import");
        action.menu = Friction::Plugins::MenuLocation::Import;
        return {action};
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
        if (actionId != QStringLiteral("import") || !mHost->activeScene()) { return; }

        const QString recentDir = AppSupport::getSettings(
                    "files", "recentImportDir", QDir::homePath()).toString();
        const QString path = AppSupport::getOpenFile(
                    mHost->mainWindow(), tr("Import Lottie Animation"), recentDir,
                    tr("Lottie Files (*.json *.lottie)"));
        if (path.isEmpty()) { return; }

        Canvas *scene = mHost->activeScene();
        ImportLottie::Analysis analysis;
        LottieImportDialog::ScaleMode scaleMode;
        LottieImportDialog::StructureMode structureMode;
        LottieImportDialog::SceneDurationMode durationMode;
        const LottieImportDialog::SceneInfo sceneInfo{
            QSize(scene->getCanvasWidth(), scene->getCanvasHeight()),
            scene->getMinFrame(), scene->getMaxFrame(), scene->getFps()
        };
        if (!LottieImportDialog::sExec(path, sceneInfo, analysis, scaleMode,
                                       structureMode, durationMode,
                                       mHost->mainWindow())) { return; }

        qreal scaleX = 1;
        qreal scaleY = 1;
        if (analysis.size.width() > 0 && analysis.size.height() > 0) {
            if (scaleMode == LottieImportDialog::ScaleMode::scaleScene) {
                scene->setCanvasSize(analysis.size.width(), analysis.size.height());
            } else if (scaleMode == LottieImportDialog::ScaleMode::fitWidth) {
                scaleX = scaleY = scene->getCanvasWidth()/qreal(analysis.size.width());
            } else if (scaleMode == LottieImportDialog::ScaleMode::fitHeight) {
                scaleX = scaleY = scene->getCanvasHeight()/qreal(analysis.size.height());
            } else if (scaleMode == LottieImportDialog::ScaleMode::stretch) {
                scaleX = scene->getCanvasWidth()/qreal(analysis.size.width());
                scaleY = scene->getCanvasHeight()/qreal(analysis.size.height());
            }
        }

        ContainerBox *target = scene->getCurrentGroup();
        auto block = scene->blockUndoRedo();
        const auto imported = ImportLottie::loadFile(path, scene, durationMode);
        if (!imported) { return; }
        block.reset();
        target->prp_pushUndoRedoName(tr("Import Lottie Animation"));

        const auto wrapper = qSharedPointerCast<ContainerBox>(imported);
        if (wrapper) {
            wrapper->getBoxTransformAnimator()->setScale(scaleX, scaleY);
            target->insertContained(0, wrapper);
            if (structureMode == LottieImportDialog::StructureMode::directObjects) {
                wrapper->ungroupKeepTransform_k();
            } else {
                wrapper->planCenterPivotPosition();
                wrapper->updateAllBoxes(UpdateReason::userChange);
            }
        } else {
            target->insertContained(0, imported);
        }
        scene->requestUpdate();
        mHost->actionFinished();
        AppSupport::setSettings("files", "recentImportDir",
                                QFileInfo(path).absoluteDir().absolutePath());
    }

    void shutdown() override { mHost = nullptr; }

private:
    Friction::Plugins::Host *mHost = nullptr;
};

#include "lottieimportplugin.moc"
