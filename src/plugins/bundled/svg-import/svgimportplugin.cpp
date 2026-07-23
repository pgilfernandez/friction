#include "frictionplugin.h"

#include "Animators/transformanimator.h"
#include "Boxes/containerbox.h"
#include "appsupport.h"
#include "canvas.h"
#include "svganimationimportdialog.h"
#include "svganimationimporter.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QWidget>

class SvgImportPlugin final : public QObject, public Friction::Plugins::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FRICTION_PLUGIN_IID FILE "metadata.json")
    Q_INTERFACES(Friction::Plugins::Plugin)

public:
    int apiVersion() const override { return Friction::Plugins::ApiVersion; }
    QString id() const override { return QStringLiteral("graphics.friction.svg-import"); }
    QString name() const override { return tr("SVG Animation Import"); }
    QString version() const override { return QStringLiteral("1.0.0"); }

    QList<Friction::Plugins::Action> actions() const override
    {
        Friction::Plugins::Action action;
        action.id = QStringLiteral("import");
        action.text = tr("Import SVG Animation");
        action.toolTip = tr("Import an animated SVG as editable Friction objects");
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
                    mHost->mainWindow(), tr("Import SVG Animation"), recentDir,
                    tr("SVG Files (*.svg)"));
        if (path.isEmpty()) { return; }

        Canvas *scene = mHost->activeScene();
        QSizeF svgSize;
        SVGAnimationImportDialog::ScaleMode scaleMode;
        SVGAnimationImportDialog::StructureMode structureMode;
        SVGAnimationImportDialog::SceneDurationMode durationMode;
        const SVGAnimationImportDialog::SceneInfo sceneInfo{
            QSize(scene->getCanvasWidth(), scene->getCanvasHeight()),
            scene->getMinFrame(), scene->getMaxFrame(), scene->getFps()
        };
        if (!SVGAnimationImportDialog::sExec(
                    path, sceneInfo, svgSize, scaleMode, structureMode,
                    durationMode, mHost->mainWindow())) { return; }

        qreal scaleX = 1;
        qreal scaleY = 1;
        if (scaleMode == SVGAnimationImportDialog::ScaleMode::fitWidth) {
            scaleX = scaleY = scene->getCanvasWidth()/svgSize.width();
        } else if (scaleMode == SVGAnimationImportDialog::ScaleMode::fitHeight) {
            scaleX = scaleY = scene->getCanvasHeight()/svgSize.height();
        } else if (scaleMode == SVGAnimationImportDialog::ScaleMode::stretch) {
            scaleX = scene->getCanvasWidth()/svgSize.width();
            scaleY = scene->getCanvasHeight()/svgSize.height();
        }

        ContainerBox *target = scene->getCurrentGroup();
        auto block = scene->blockUndoRedo();
        bool technicalRoot = false;
        const auto imported = ImportSVGAnimation::loadSVGFile(
                    path, scene, durationMode, &technicalRoot);
        if (!imported) { return; }
        const QString importName = QFileInfo(path).completeBaseName();
        block.reset();
        target->prp_pushUndoRedoName(tr("Import SVG Animation"));

        const bool directObjects = structureMode ==
                SVGAnimationImportDialog::StructureMode::directObjects;
        qsptr<ContainerBox> wrapper;
        if (technicalRoot) {
            wrapper = qSharedPointerCast<ContainerBox>(imported);
            wrapper->prp_setName(importName);
        } else {
            wrapper = enve::make_shared<ContainerBox>(importName, eBoxType::group);
            wrapper->addContained(imported);
        }
        wrapper->getBoxTransformAnimator()->setScale(scaleX, scaleY);
        target->insertContained(0, wrapper);
        if (directObjects) {
            wrapper->ungroupKeepTransform_k();
        } else {
            wrapper->planCenterPivotPosition();
            wrapper->updateAllBoxes(UpdateReason::userChange);
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

#include "svgimportplugin.moc"
