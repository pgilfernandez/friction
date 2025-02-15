/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "expressiondialog.h"

#include <QLabel>
#include <QCheckBox>
#include <QScrollBar>
#include <QStatusBar>
#include <QApplication>
#include <QButtonGroup>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QUuid>
#include <QRegularExpression>

#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciapis.h>

#include "Expressions/expression.h"
#include "Boxes/boundingbox.h"
#include "Private/document.h"
#include "expressioneditor.h"
#include "appsupport.h"
#include "canvas.h"

using namespace Friction::Core;

class JSLexer : public QsciLexerJavaScript
{
public:
    using QsciLexerJavaScript::QsciLexerJavaScript;
    JSLexer(QsciScintilla* const editor)
        : QsciLexerJavaScript(editor)
    {
        setDefaultPaper(QColor(33, 33, 38)/*"#2E2F30"*/);
        setFont(QApplication::font());
        setColor("#D6CF9A");

        setColor("#666666", Comment);
        setColor("#666666", CommentLine);
        setColor("#666666", CommentDoc);
        setColor("#666666", CommentLineDoc);

        setColor("#bf803C", Number);
        setColor("#D69545", DoubleQuotedString);
        setColor("#D69545", SingleQuotedString);

        setColor("#45C6D6", Keyword); // javascript
        setColor("#88ffbb", KeywordSet2); // definitions
        setColor("#FF8080", GlobalClass); // bindings

        setAutoIndentStyle(QsciScintilla::AiMaintain);
    }

    const char *keywords(int set) const {
        if (set == 1) {
            return sKeywordClass1;
        } else if (set == 2) {
            return mDefKeywordsClass2.data();
        } else if (set == 4) {
            return mBindKeywords5.data();
        }
        return 0;
    }

    QStringList autoCompletionWordSeparators() const
    {
        QStringList wl;

        wl << "::" << "->";

        return wl;
    }

    void addDefinition(const QString& def)
    {
        mDefinitions << def;
    }

    void removeDefinition(const QString& def)
    {
        mDefinitions.removeOne(def);
    }

    void clearDefinitions()
    {
        mDefinitions.clear();
    }

    void prepareDefinitions()
    {
        mDefKeywordsClass2 = mDefinitions.join(' ').toUtf8();
    }

    void addBinding(const QString& bind)
    {
        mBindings << bind;
    }

    void removeBinding(const QString& bind)
    {
        mBindings.removeOne(bind);
    }

    void clearBindings()
    {
        mBindings.clear();
    }

    void prepareBindings()
    {
        mBindKeywords5 = mBindings.join(' ').toUtf8();
    }

private:
    QStringList mDefinitions;
    QStringList mBindings;

    QByteArray mDefKeywordsClass2;
    QByteArray mBindKeywords5;

    static const char *sKeywordClass1;
};

const char *JSLexer::sKeywordClass1 =
            "abstract boolean break byte case catch char class const continue "
            "debugger default delete do double else enum export extends final "
            "finally float for function goto if implements import in instanceof "
            "int interface long native new package private protected public "
            "return short static super switch synchronized this throw throws "
            "transient try typeof var void volatile while with "
            "true false "
            "Math";

#define KEYWORDSET_MAX  8

class JSEditor : public QsciScintilla
{
public:
    JSEditor(const QString& fillerText)
        : mFillerTextV(fillerText)
    {
        setMinimumWidth(20*eSizesUI::widget);

        setFont(QApplication::font());
        setMargins(2);
        setMarginType(0, NumberMargin);
        setMarginWidth(0, "9999");
        setMarginWidth(1, "9");
        setMarginsFont(font());
        setMarginsForegroundColor("#999999");
        setMarginsBackgroundColor(QColor(40, 40, 47)/*"#444444"*/);

        setTabWidth(4);
        setBraceMatching(SloppyBraceMatch);
        setMatchedBraceBackgroundColor(QColor(33, 33, 38)/*"#555555"*/);
        setUnmatchedBraceBackgroundColor(QColor(33, 33, 38)/*"#555555"*/);
        setMatchedBraceForegroundColor("#D6CF9A");
        setUnmatchedBraceForegroundColor(QColor(255, 115, 115));
        setCaretForegroundColor(Qt::white);
        setCaretWidth(2);

        setAutoCompletionThreshold(1);
        setAutoCompletionCaseSensitivity(false);

        setScrollWidth(1);
        setScrollWidthTracking(true);

        connect(this, &JSEditor::SCN_FOCUSIN,
                this, &JSEditor::clearFillerText);
        connect(this, &JSEditor::SCN_FOCUSOUT,
                this, [this]() {
            if (length() == 0) { setFillerText(); }
            else { mFillerText = false; }
        });
    }

    void updateLexer()
    {
        for (int k = 0; k <= KEYWORDSET_MAX; ++k) {
            const char *kw = lexer() -> keywords(k + 1);

            if (!kw) { kw = ""; }

            SendScintilla(SCI_SETKEYWORDS, k, kw);
        }
        recolor();
    }

    void setText(const QString &text) override
    {
        if (text.isEmpty()) {
            setFillerText();
        } else {
            QsciScintilla::setText(text);
        }
    }

    QString text() const
    {
        if (mFillerText) { return QString(); }
        return QsciScintilla::text();
    }

    void setFillerText()
    {
        if (mFillerText) { return; }
        mFillerText = true;
        setText(mFillerTextV);
    }

    void clearFillerText()
    {
        if (mFillerText) {
            mFillerText = false;
            QsciScintilla::setText("");
        }
    }

private:
    bool mFillerText = false;
    const QString mFillerTextV;
    using QsciScintilla::setText;
    using QsciScintilla::text;
};

void addBasicDefs(QsciAPIs* const target)
{
    target->add("function");
    target->add("var");
    target->add("return");
    target->add("true");
    target->add("false");
    target->add("new");
    target->add("this");
    target->add("delete");
    target->add("const");
    target->add("break");
    target->add("while");
    target->add("for");

    target->add("Math.E");
    target->add("Math.LN2");
    target->add("Math.LN10");
    target->add("Math.LOG2E");
    target->add("Math.LOG10E");
    target->add("Math.PI");
    target->add("Math.SQRT1_2");
    target->add("Math.SQRT2");

    target->add("Math.abs(x)");
    target->add("Math.acos(x)");
    target->add("Math.acosh(x)");
    target->add("Math.asin(x)");
    target->add("Math.asinh(x)");
    target->add("Math.atan(x)");
    target->add("Math.atanh(x)");
    target->add("Math.atan2(y, x)");
    target->add("Math.cbrt(x)");
    target->add("Math.ceil(x)");
    target->add("Math.clz32(x)");
    target->add("Math.cos(x)");
    target->add("Math.cosh(x)");
    target->add("Math.exp(x)");
    target->add("Math.expm1(x)");
    target->add("Math.floor(x)");
    target->add("Math.fround(x)");
    target->add("Math.hypot(x, y, ...)");
    target->add("Math.imul(x, y)");
    target->add("Math.log(x)");
    target->add("Math.log1p(x)");
    target->add("Math.log10(x)");
    target->add("Math.log2(x)");
    target->add("Math.max(x, y, ...)");
    target->add("Math.min(x, y, ...)");
    target->add("Math.pow(x, y)");
    target->add("Math.random()");
    target->add("Math.round(x)");
    target->add("Math.sign(x)");
    target->add("Math.sin(x)");
    target->add("Math.sinh(x)");
    target->add("Math.sqrt(x)");
    target->add("Math.tan(x)");
    target->add("Math.tanh(x)");
    target->add("Math.trunc(x)");

    // add expressions presets
    const auto expressions = eSettings::sInstance->fExpressions.getAll();
    for (const auto &expr : expressions) {
        if (!expr.enabled) { continue; }
        for (const auto &highlight : expr.highlighters) {
            target->add(highlight);
        }
    }
}

ExpressionDialog::ExpressionDialog(QrealAnimator* const target,
                                   QWidget * const parent)
    : Friction::Ui::Dialog(parent)
    , mTarget(target)
    , mTab(nullptr)
    , mTabEditor(0)
    , mPresetsCombo(nullptr)
    , mSettings(eSettings::sInstance)
{
    setWindowTitle(tr("Expression %1").arg(target->prp_getName()));

    const auto windowLayout = new QVBoxLayout(this);
    setLayout(windowLayout);

    mTab = new QTabWidget(this);
    mTab->setTabBarAutoHide(true);
    windowLayout->addWidget(mTab);

    const auto editorWidget = new QWidget(this);
    mTabEditor = mTab->addTab(editorWidget, tr("Editor"));
    const auto mainLayout = new QVBoxLayout(editorWidget);

    mainLayout->addWidget(setupPresetsUi());

    const auto tabLayout = new QHBoxLayout;
    tabLayout->setContentsMargins(0, 0, 0, 0);
    mBindingsButton = new QPushButton(tr("Bindings and Script"), this);
    mBindingsButton->setFocusPolicy(Qt::NoFocus);
    mBindingsButton->setObjectName("leftButton");
    mBindingsButton->setCheckable(true);
    mBindingsButton->setChecked(true);

    mDefinitionsButon = new QPushButton(tr("Definitions"), this);
    mDefinitionsButon->setFocusPolicy(Qt::NoFocus);
    mDefinitionsButon->setObjectName("rightButton");
    mDefinitionsButon->setCheckable(true);

    const auto tabGroup = new QButtonGroup(this);
    tabGroup->addButton(mBindingsButton, 0);
    tabGroup->addButton(mDefinitionsButon, 1);
    tabGroup->setExclusive(true);
    connect(tabGroup, qOverload<int, bool>(&QButtonGroup::idToggled),
            this, [this](const int id, const bool checked) {
        if (checked) { setCurrentTabId(id); }
    });

    tabLayout->addWidget(mBindingsButton);
    tabLayout->addWidget(mDefinitionsButon);
    mainLayout->addLayout(tabLayout);

    mBindings = new ExpressionEditor(target, this);
    connect(mBindings, &ExpressionEditor::textChanged,
            this, [this]() {
        mBindingsChanged = true;
        updateAllScript();
    });

    mBindingsLabel = new QLabel(tr("Bindings:"));
    mainLayout->addWidget(mBindingsLabel);
    mainLayout->addWidget(mBindings, 1);

    mBindingsError = new QLabel(this);
    mBindingsError->setObjectName("errorLabel");
    mainLayout->addWidget(mBindingsError);

    mDefsLabel = new QLabel(tr("Definitions:"));
    mainLayout->addWidget(mDefsLabel);
    mDefinitions = new JSEditor(tr("// Here you can define JavaScript functions,\n"
                                   "// you can later use in the 'Calculate'\n"
                                   "// portion of the script."));
    mDefsLexer = new JSLexer(mDefinitions);

    mDefinitionsApi = new QsciAPIs(mDefsLexer);
    addBasicDefs(mDefinitionsApi);
    mDefinitionsApi->prepare();

    mDefinitions->setLexer(mDefsLexer);
    mDefinitions->setAutoCompletionSource(QsciScintilla::AcsAll);
    mDefinitions->setText(target->getExpressionDefinitionsString());
    connect(mDefinitions, &QsciScintilla::textChanged, this, [this]() {
        mDefinitions->autoCompleteFromAll();
        mDefinitionsChanged = true;
        updateAllScript();
    });

    mainLayout->addWidget(mDefinitions);

    mDefinitionsError = new QLabel(this);
    mDefinitionsError->setObjectName("errorLabel");
    mainLayout->addWidget(mDefinitionsError);

    mScriptLabel = new QLabel(QString("%1 (  ) :").arg(tr("Calculate")));
    mainLayout->addWidget(mScriptLabel);
    mScript = new JSEditor(tr("// Here you can define a JavaScript script,\n"
                              "// that will be evaluated every time any of\n"
                              "// the bound property values changes.\n"
                              "// You should return the resulting value\n"
                              "// at the end of this script."));
    mScriptLexer = new JSLexer(mScript);

    mScriptApi = new QsciAPIs(mScriptLexer);
    addBasicDefs(mScriptApi);
    mScriptApi->prepare();

    mScript->setLexer(mScriptLexer);
    mScript->setAutoCompletionSource(QsciScintilla::AcsAll);
    mScript->setText(target->getExpressionScriptString());
    connect(mScript, &QsciScintilla::textChanged,
            mScript, &QsciScintilla::autoCompleteFromAll);
    mainLayout->addWidget(mScript, 2);

    mScriptError = new QLabel(this);
    mScriptError->setObjectName("errorLabel");
    mainLayout->addWidget(mScriptError);

    const auto buttonsLayout = new QHBoxLayout;
    const auto applyButton = new QPushButton(tr("Apply"), this);
    const auto okButton = new QPushButton(tr("Ok"), this);
    const auto cancelButton = new QPushButton(tr("Cancel"), this);
    const auto checkBox = new QCheckBox(tr("Auto Apply"), this);
    connect(checkBox, &QCheckBox::stateChanged,
            this, [this](const int state) {
        if (state) {
            mAutoApplyConn << connect(mBindings, &ExpressionEditor::textChanged,
                                      this, [this]() { apply(false); });
            mAutoApplyConn << connect(mDefinitions, &QsciScintilla::textChanged,
                                      this, [this]() { apply(false); });
            mAutoApplyConn << connect(mScript, &QsciScintilla::textChanged,
                                      this, [this]() { apply(false); });
        } else { mAutoApplyConn.clear(); }
    });

    buttonsLayout->addWidget(checkBox);
    buttonsLayout->addWidget(applyButton);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonsLayout);

    connect(applyButton, &QPushButton::released,
            this, [this]() { apply(true); });
    connect(okButton, &QPushButton::released,
            this, [this]() {
        const bool valid = apply(true);
        if(valid) accept();
    });
    connect(cancelButton, &QPushButton::released,
            this, &ExpressionDialog::reject);

    connect(mScript, &QsciScintilla::SCN_FOCUSIN,
            this, [this]() {
        if(mBindingsChanged || mDefinitionsChanged) {
            updateAllScript();
        }
    });
    setCurrentTabId(0);
    updateAllScript();

    const int pixSize = eSizesUI::widget/2;
    eSizesUI::widget.add(mBindingsButton,
                         [this](const int size) {
        Q_UNUSED(size)
        mBindingsButton->setFixedHeight(eSizesUI::button);
        mDefinitionsButon->setFixedHeight(eSizesUI::button);
    });

    QPixmap pix(pixSize, pixSize);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setBrush(Qt::red);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawEllipse(pix.rect().adjusted(1, 1, -1, -1));
    p.end();
    mRedDotIcon = QIcon(pix);
}

void ExpressionDialog::setCurrentTabId(const int id)
{
    const bool first = id == 0;
    if (!first) { mBindingsButton->setChecked(false); }
    mBindingsLabel->setVisible(first);
    mBindings->setVisible(first);
    mBindingsError->setVisible(first);

    mScriptLabel->setVisible(first);
    mScript->setVisible(first);
    mScriptError->setVisible(first);

    if (first) { mDefinitionsButon->setChecked(false); }
    mDefsLabel->setVisible(!first);
    mDefinitions->setVisible(!first);
    mDefinitionsError->setVisible(!first);
}

void ExpressionDialog::updateAllScript()
{
    mScriptApi->clear();
    addBasicDefs(mScriptApi);

    mScriptLexer->clearDefinitions();
    mScriptLexer->clearBindings();

    mDefsLexer->clearDefinitions();

    updateScriptBindings();
    updateScriptDefinitions();

    mScriptApi->prepare();

    mScriptLexer->prepareDefinitions();
    mScriptLexer->prepareBindings();
    mScript->updateLexer();

    mDefsLexer->prepareDefinitions();
    mDefinitions->updateLexer();

    mDefinitionsChanged = false;
    mBindingsChanged = false;
}

void ExpressionDialog::updateScriptDefinitions()
{
    const QString defsText = mDefinitions->text();
    QString scriptContext;

    int nOpenBrackets = 0;
    for (const auto& c : defsText) {
        if (c == '{') { nOpenBrackets++; }
        else if (c == '}') { nOpenBrackets--; }
        else if (c == QChar::LineFeed) { continue; }
        else if (!nOpenBrackets) { scriptContext.append(c); }
    }

    {
        QRegExp funcDefs("(class|function)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*(\\([a-zA-Z0-9_, ]*\\))");
        int pos = 0;
        while ((pos = funcDefs.indexIn(scriptContext, pos)) != -1) {
              QStringList funcs = funcDefs.capturedTexts();
              for (int i = 2; i < funcs.count() - 1; i += 3) {
                  const auto& func = funcs.at(i);
                  const auto& funcArgs = funcs.at(i + 1);
                  if (func.isEmpty()) { continue; }
                  mScriptApi->add(func + funcArgs);
                  mScriptLexer->addDefinition(func);
                  mDefsLexer->addDefinition(func);
              }
              pos += funcDefs.matchedLength();
        }
    }

    {
        QRegExp varDefs("([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(?!=)");
        int pos = 0;
        while ((pos = varDefs.indexIn(scriptContext, pos)) != -1) {
              QStringList vars = varDefs.capturedTexts();
              for (int i = 1; i < vars.count(); i++) {
                  const auto& var = vars.at(i);
                  if (var.isEmpty()) { continue; }
                  mScriptApi->add(var);
                  mScriptLexer->addDefinition(var);
                  mDefsLexer->addDefinition(var);
              }
              pos += varDefs.matchedLength();
        }
    }
}

bool ExpressionDialog::getBindings(PropertyBindingMap& bindings)
{
    mBindingsError->clear();
    const auto bindingsStr = mBindings->text();
    try {
        bindings = PropertyBindingParser::parseBindings(bindingsStr,
                                                        nullptr,
                                                        mTarget);
        mBindingsButton->setIcon(QIcon());
        return true;
    } catch (const std::exception& e) {
        mBindingsButton->setIcon(mRedDotIcon);
        mBindingsError->setText(e.what());
        return false;
    }
}

#define BFC_0 "<font color=\"#FF8080\">"
#define BFC_1 "</font>"

void ExpressionDialog::updateScriptBindings()
{
    QStringList bindingList;
    PropertyBindingMap bindings;
    if (getBindings(bindings)) {
        for (const auto& binding : bindings) {
            bindingList << binding.first;
            mScriptApi->add(binding.first);
            mScriptLexer->addBinding(binding.first);
        }
    }
    mScriptLabel->setText(tr("Calculate") + " ( " BFC_0 +
                            bindingList.join(BFC_1 ", " BFC_0) +
                          BFC_1 " ) :");
}

bool ExpressionDialog::apply(const bool action)
{
    mBindingsButton->setIcon(QIcon());
    mDefinitionsButon->setIcon(QIcon());
    mDefinitionsError->clear();
    mScriptError->clear();

    const auto definitionsStr = mDefinitions->text();
    const auto scriptStr = mScript->text();

    PropertyBindingMap bindings;
    if (!getBindings(bindings)) { return false; }

    auto engine = std::make_unique<QJSEngine>();
    try {
        Expression::sAddDefinitionsTo(definitionsStr, *engine);
    } catch (const std::exception& e) {
        mDefinitionsError->setText(e.what());
        mDefinitionsButon->setIcon(mRedDotIcon);
        return false;
    }

    QJSValue eEvaluate;
    try {
        Expression::sAddScriptTo(scriptStr, bindings, *engine, eEvaluate,
                                 Expression::sQrealAnimatorTester);
    } catch (const std::exception& e) {
        mScriptError->setText(e.what());
        mBindingsButton->setIcon(mRedDotIcon);
        return false;
    }

    try {
        auto expr = Expression::sCreate(definitionsStr,
                                        scriptStr, std::move(bindings),
                                        std::move(engine),
                                        std::move(eEvaluate));
        if (expr && !expr->isValid()) { expr = nullptr; }
        if (action) {
            mTarget->setExpressionAction(expr);
        } else {
            mTarget->setExpression(expr);
        }
    } catch (const std::exception& e) {
        return false;
    }

    Document::sInstance->actionFinished();
    return true;
}

QWidget *ExpressionDialog::setupPresetsUi()
{
    const auto presetWidget = new QWidget(this);
    presetWidget->setContentsMargins(0, 0, 0, 0);

    const auto presetLayout = new QHBoxLayout(presetWidget);
    presetLayout->setContentsMargins(0, 0, 0, 0);

    mPresetsCombo = new QComboBox(this);
    mPresetsCombo->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
    mPresetsCombo->setToolTip(tr("Select a Preset from the list to fill Bindings, Definitions\n"
                                 "and Calculate fields. In case there is no Preset available,\n"
                                 "you can create a new one by clicking on the '+' button."));

    const auto presetLabel = new QLabel(tr("Preset"), this);

    const auto addPresetBtn = new QPushButton(QIcon::fromTheme("plus"),
                                              QString(), this);
    addPresetBtn->setToolTip(tr("Save as New Preset"));

    const auto removePresetBtn = new QPushButton(QIcon::fromTheme("minus"),
                                                 QString(), this);
    removePresetBtn->setToolTip(tr("Remove Active Preset"));

    const auto editPresetBtn = new QPushButton(QIcon::fromTheme("edit"),
                                               QString(), this);
    editPresetBtn->setToolTip(tr("Edit Active Preset Name"));

    const auto importPresetBtn = new QPushButton(QIcon::fromTheme("file-import"),
                                                 QString(), this);
    importPresetBtn->setToolTip(tr("Import Preset from file"));

    const auto exportPresetBtn = new QPushButton(QIcon::fromTheme("file-export"),
                                                 QString(), this);
    exportPresetBtn->setToolTip(tr("Export Active Preset to file"));

    presetLayout->addWidget(presetLabel);
    presetLayout->addWidget(mPresetsCombo);
    presetLayout->addWidget(addPresetBtn);
    presetLayout->addWidget(removePresetBtn);
    presetLayout->addWidget(editPresetBtn);
    presetLayout->addWidget(importPresetBtn);
    presetLayout->addWidget(exportPresetBtn);

    populatePresets(true);

    connect(mPresetsCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        const QString id = mPresetsCombo->itemData(index).toString();
        if (!id.isEmpty()) { applyPreset(id) ;}
    });

    connect(exportPresetBtn,
            &QPushButton::released,
            this, [this]() {
        QString path = QFileDialog::getSaveFileName(this,
                                                    tr("Export Preset"),
                                                    AppSupport::getSettings("files",
                                                                            "lastExprExportDir",
                                                                            QDir::homePath()).toString(),
                                                    "Expressions (*.fexpr)");
        if (path.trimmed().isEmpty()) { return; }
        if (QFileInfo(path).suffix() != "fexpr") { path.append(".fexpr"); }
        exportPreset(path);
    });

    connect(importPresetBtn,
            &QPushButton::released,
            this, [this]() {
        const QString preset = QFileDialog::getOpenFileName(this,
                                                            tr("Import Preset"),
                                                            AppSupport::getSettings("files",
                                                                                    "lastExprImportDir",
                                                                                    QDir::homePath()).toString(),
                                                            "Expressions (*.fexpr)");
        if (mSettings->fExpressions.isValidExprFile(preset)) {
            importPreset(preset);
        } else {
            QMessageBox::warning(this,
                                 tr("Failed to read preset"),
                                 tr("This file is not a valid expression preset."));
        }
    });

    connect(addPresetBtn,
            &QPushButton::released,
            this, [this]() {
        Friction::Ui::Dialog dialog(this);
        dialog.setWindowTitle(tr("New Preset"));

        QVBoxLayout layout(&dialog);

        QLabel label(tr("Title"), &dialog);
        layout.addWidget(&label);

        QLineEdit lineEdit(&dialog);
        layout.addWidget(&lineEdit);
        lineEdit.setFocus();

        QHBoxLayout buttonLayout;
        QPushButton cancelButton(tr("Cancel"), &dialog);
        QPushButton okButton(tr("Ok"), &dialog);

        okButton.setDefault(true);

        buttonLayout.addWidget(&okButton);
        buttonLayout.addWidget(&cancelButton);
        layout.addLayout(&buttonLayout);

        connect(&cancelButton, &QPushButton::clicked,
                &dialog, &QDialog::reject);
        connect(&okButton, &QPushButton::clicked,
                &dialog, &QDialog::accept);

        if (dialog.exec() == QDialog::Accepted) {
            savePreset(lineEdit.text().trimmed());
        }
    });

    connect(removePresetBtn,
            &QPushButton::released,
            this, [this]() {
        /*int index = mPresetsCombo->currentIndex();
        if (index > 0) {
            QString presetName = mPresetsCombo->itemText(index);

            QDialog dialog(this);
            dialog.setWindowTitle(tr("Confirm Delete"));

            QVBoxLayout layout(&dialog);

            QLabel label(tr("Are you sure you want to <b>remove '%1'</b> Preset?").arg(presetName), &dialog);
            layout.addWidget(&label);

            QHBoxLayout buttonLayout;
            QPushButton cancelButton(tr("Cancel"), &dialog);
            QPushButton okButton(tr("OK"), &dialog);
            buttonLayout.addWidget(&okButton);
            buttonLayout.addWidget(&cancelButton);
            layout.addLayout(&buttonLayout);

            connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
            connect(&okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

            if (dialog.exec() == QDialog::Accepted) {
                QString filePath = mPresetsDir.filePath(QString("%1.json").arg(presetName));
                QString filePathUser = mPresetsDirUser.filePath(QString("%1.json").arg(presetName));
                QFile file(filePath);
                QFile fileUser(filePathUser);
                if (file.exists()) {
                    if (file.remove()) {
                        qWarning() << "Preset file removed:" << filePath;
                    } else {
                        qWarning() << "Failed to remove preset file:" << filePath;
                    }
                } else if (fileUser.exists()) {
                    if (fileUser.remove()) {
                        qWarning() << "Preset file removed:" << filePathUser;
                    } else {
                        qWarning() << "Failed to remove preset file:" << filePathUser;
                    }
                } else {
                    qWarning() << "Preset file does not exist in:" << filePath;
                    qWarning() << "Preset file does not exist in:" << filePathUser;
                }
                updatePresetCombo();
            }
        }*/
    });

    connect(editPresetBtn,
            &QPushButton::released,
            this, [this]() {
        /*int index = mPresetsCombo->currentIndex();
        if (index > 0) {
            QString presetName = mPresetsCombo->itemText(index);

            QDialog dialog(this);
            dialog.setWindowTitle(tr("Preset Name"));

            QVBoxLayout layout(&dialog);

            QLabel label(tr("<b>Edit</b> Preset Name:"), &dialog);
            layout.addWidget(&label);

            QLineEdit lineEdit(&dialog);
            lineEdit.setText(presetName);
            layout.addWidget(&lineEdit);
            lineEdit.setFocus();

            QHBoxLayout buttonLayout;
            QPushButton cancelButton(tr("Cancel"), &dialog);
            QPushButton okButton(tr("OK"), &dialog);
            buttonLayout.addWidget(&okButton);
            buttonLayout.addWidget(&cancelButton);
            layout.addLayout(&buttonLayout);

            connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
            connect(&okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

            if (dialog.exec() == QDialog::Accepted) {
                QString newPresetName = lineEdit.text().trimmed();
                if (!newPresetName.isEmpty() && newPresetName != presetName) {
                    QString oldFilePath = mPresetsDirUser.filePath(QString("%1.json").arg(presetName));
                    QString newFilePath = mPresetsDirUser.filePath(QString("%1.json").arg(newPresetName));
                    if (QFile::rename(oldFilePath, newFilePath)) {
                        updatePresetCombo();
                        int newIndex = mPresetsCombo->findText(newPresetName);
                        if (newIndex != -1) {
                            mPresetsCombo->setCurrentIndex(newIndex);
                        }
                    } else {
                        qWarning() << "Failed to rename preset file:" << oldFilePath << "to" << newFilePath;
                    }
                }
            }
        }*/
    });

    return presetWidget;
}

void ExpressionDialog::populatePresets(const bool &clear)
{
    if (clear) {
        mPresetsCombo->clear();
        mPresetsCombo->addItem(tr("Select ..."));
    }
    for (const auto &expr : mSettings->fExpressions.getUser()) {
        mPresetsCombo->addItem(expr.title, expr.id);
    }
}

void ExpressionDialog::exportPreset(const QString &path)
{
    if (path.isEmpty()) { return; }

    const QString bindings = mBindings->text();
    const QString definitions = mDefinitions->text();
    const QString script = mScript->text();

    if (bindings.isEmpty() &&
        definitions.isEmpty() &&
        script.isEmpty()) { return; }

    ExpressionPresets::Expr expr;
    expr.title = QFileInfo(path).baseName();
    expr.id = genPresetId(expr.title);
    expr.bindings = bindings;
    expr.definitions = definitions;
    expr.script = script;

    // TODO: open a new dialog to edit the meta data before save

    if (mSettings->fExpressions.saveExpr(expr, path)) {
        QMessageBox::information(this,
                                 tr("Export Success"),
                                 tr("Your new preset has been exported to %1.").arg(path));
        AppSupport::setSettings("files",
                                "lastExprExportDir",
                                QFileInfo(path).absoluteDir().absolutePath());
    } else {
        QMessageBox::warning(this,
                             tr("Export Failed"),
                             tr("Unable to export preset to %1.").arg(path));
    }
}

void ExpressionDialog::importPreset(const QString& path)
{
    return;

    AppSupport::setSettings("files",
                            "lastExprImportDir",
                            QFileInfo(path).absoluteDir().absolutePath());

    /*QString path = filePath;
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, tr("Import Preset"), mPresetsDirUser.absolutePath(), tr("JSON Files (*.json);;All Files (*)"));
        if (path.isEmpty()) {
            return;
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for reading:" << path;
        return;
    }

    QString jsonContent = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON content";
        return;
    }

    QJsonObject obj = doc.object();
    QString bindings = obj.value("bindings").toString();
    QString calculate = obj.value("calculate").toString();
    QString definitions = obj.value("definitions").toString();

    mBindings->setText(bindings);
    mScript->setText(calculate);
    mDefinitions->setText(definitions);

    updateAllScript();

    QString presetName = QFileInfo(path).baseName();
    int index = mPresetsCombo->findText(presetName);
    if (index != -1) {
        mPresetsCombo->setCurrentIndex(index);
    }*/
}

void ExpressionDialog::savePreset(const QString &title)
{
    if (title.trimmed().isEmpty()) { return; }

    const QString bindings = mBindings->text();
    const QString definitions = mDefinitions->text();
    const QString script = mScript->text();

    const bool hasDef = !definitions.trimmed().isEmpty();
    const bool hasBind = !bindings.isEmpty();
    const bool hasScript = !script.trimmed().isEmpty();
    const bool onlyDef = hasDef && !hasBind && !hasScript;

    if (bindings.isEmpty() &&
        definitions.isEmpty() &&
        script.isEmpty()) { return; }

    ExpressionPresets::Expr expr;
    expr.valid = true;
    expr.enabled = true;
    expr.version = 1.0;
    expr.title = title;
    expr.id = genPresetId(QString());
    expr.bindings = bindings;
    expr.definitions = definitions;
    expr.script = script;

    if (onlyDef) {
        const auto lines = definitions.split("\n");
        for (const auto &line : lines) {
            if (line.startsWith("function")) {
                QStringList split = line.split("function");
                if (split.count() >= 2) { expr.highlighters << split.at(1)
                                                                    .trimmed()
                                                                    .split("{")
                                                                    .takeFirst()
                                                                    .trimmed(); }
            }
        }
    }

    const QString path = QString("%1/%2.fexpr").arg(AppSupport::getAppUserExPresetsPath(),
                                                    expr.id);

    if (!mSettings->fExpressions.saveExpr(expr, path)) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Unable to save preset %1.").arg(path));
    } else {
        mSettings->fExpressions.addExpr(expr);
        if (!expr.highlighters.isEmpty()) {
            for (const auto &highlight : expr.highlighters) {
                mScriptApi->add(highlight);
            }
        }
        populatePresets(true);
        const int index = mPresetsCombo->findData(expr.id);
        if (index >= 0) {
            mPresetsCombo->blockSignals(true);
            mPresetsCombo->setCurrentIndex(index);
            mPresetsCombo->blockSignals(false);
        }
    }
}

void ExpressionDialog::applyPreset(const QString &id)
{
    if (!mSettings->fExpressions.hasExpr(id)) { return; }

    const auto expr = mSettings->fExpressions.getExpr(id);

    const bool hasBind = !expr.bindings.isEmpty();
    const bool hasScript = !expr.script.isEmpty();

    if (!hasBind && !hasScript) { return; }

    mDefinitions->clearFillerText();
    mDefinitions->setText(expr.definitions);

    mBindings->clearFillerText();
    mBindings->setText(expr.bindings);

    mScript->clearFillerText();
    mScript->setText(expr.script);

    updateAllScript();

    if (hasScript) { apply(true); }
}

const QString ExpressionDialog::genPresetId(const QString &title)
{
    static QRegularExpression rx("[^a-zA-Z0-9-_]");
    QString uid = QUuid::createUuid().toString();
    if (!title.isEmpty()) {
        uid.append(QString("_%1").arg(title));
    }
    return uid.replace(rx, "");
}
