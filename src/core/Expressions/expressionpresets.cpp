#include "expressionpresets.h"
#include "appsupport.h"

#include <QFile>
#include <QSettings>
#include <QDebug>

using namespace Friction::Core;

ExpressionPresets::ExpressionPresets(QObject *parent)
    : QObject{parent}
{
    scanAll();
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getAll()
{
    return mExpr;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getCoreDefinitions()) { list << expr; }
    for (const auto &expr : getUserDefinitions()) { list << expr; }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getCore(const QString &category)
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid || !expr.core) { continue; }
        if (!category.isEmpty()) {
            if (!expr.categories.contains(category)) { continue; }
        }
        list.append(expr);
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getCoreDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getCore()) {
        if (expr.bindings.isEmpty() &&
            !expr.definitions.isEmpty() &&
            expr.script.isEmpty()) { list.append(expr); }
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getUser(const QString &category)
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid ||
            expr.core ||
            expr.path.startsWith(":")) { continue; }
        if (!category.isEmpty()) {
            if (!expr.categories.contains(category)) { continue; }
        }
        list.append(expr);
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getUserDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getUser()) {
        if (expr.bindings.isEmpty() &&
            !expr.definitions.isEmpty() &&
            expr.script.isEmpty()) { list.append(expr); }
    }
    return list;
}

void ExpressionPresets::loadExpr(const QString &path)
{
    if (!QFile::exists(path)) { return; }
    qDebug() << "Load expression" << path;
    if (!isValidExprFile(path)) {
        qDebug() << "Bad expression" << path;
        return;
    }

    QSettings fexpr(path, QSettings::IniFormat);
    ExpressionPresets::Expr expr;
    expr.core = path.startsWith(":");
    expr.valid = true;
    expr.enabled = true; // TODO (settings)
    expr.version = fexpr.value("version").toDouble();
    expr.id = fexpr.value("id").toString();
    expr.path = path;
    expr.title = fexpr.value("title").toString();
    expr.author = fexpr.value("author").toString();
    expr.description = fexpr.value("description").toString();
    expr.url = fexpr.value("url").toString();
    expr.license = fexpr.value("license").toString();
    expr.categories = fexpr.value("categories").toStringList();
    expr.highlighters = fexpr.value("highlighters").toStringList();
    expr.definitions = fexpr.value("definitions").toString();
    expr.bindings = fexpr.value("bindings").toString();
    expr.script = fexpr.value("script").toString();

    if (!hasExpr(expr.id)) {
        qDebug() << "Added expression" << expr.title << expr.id;
        mExpr << expr;
    }
}

void ExpressionPresets::loadExpr(const QStringList &paths)
{
    for (const auto &path : paths) {
        loadExpr(path);
    }
}

bool ExpressionPresets::saveExpr(const int &index,
                                 const QString &path)
{
    if (!hasExpr(index) || path.isEmpty()) { return false; }
    return saveExpr(mExpr.at(index), path);
}

bool ExpressionPresets::saveExpr(const QString &id,
                                 const QString &path)
{
    return saveExpr(getExpr(id), path);
}

bool ExpressionPresets::saveExpr(const Expr &expr,
                                 const QString &path)
{
    if (!expr.valid || path.isEmpty()) { return false; }

    QSettings fexpr(path, QSettings::IniFormat);
    fexpr.setValue("version", expr.version);
    fexpr.setValue("id", expr.id);
    fexpr.setValue("title", expr.title);
    fexpr.setValue("author", expr.author);
    fexpr.setValue("description", expr.description);
    fexpr.setValue("url", expr.url);
    fexpr.setValue("license", expr.license);
    fexpr.setValue("categories", expr.categories);
    fexpr.setValue("highlighters", expr.highlighters);
    fexpr.setValue("definitions", expr.definitions);
    fexpr.setValue("bindings", expr.bindings);
    fexpr.setValue("script", expr.script);
    fexpr.sync();

    return isValidExprFile(path);
}

bool ExpressionPresets::hasExpr(const int &index)
{
    if (mExpr.count() > 0 &&
        index >= 0 &&
        index < mExpr.count()) {
        return mExpr.at(index).valid;
    }
    return false;
}

bool ExpressionPresets::hasExpr(const QString &id)
{
    if (id.isEmpty()) { return false; }
    for (const auto &expr : mExpr) {
        if (!expr.valid) { continue; }
        if (expr.id == id) { return true; }
    }
    return false;
}

const ExpressionPresets::Expr
ExpressionPresets::getExpr(const int &index)
{
    if (hasExpr(index)) { return mExpr.at(index); }
    Expr expr;
    expr.valid = false;
    return expr;
}

const ExpressionPresets::Expr ExpressionPresets::getExpr(const QString &id)
{
    Expr noExpr;
    noExpr.valid = false;
    if (id.isEmpty()) { return noExpr; }

    for (const auto &expr : mExpr) {
        if (!expr.valid) { continue; }
        if (expr.id == id) { return expr; }
    }

    return noExpr;
}

int ExpressionPresets::getExprIndex(const QString &id)
{
    if (id.isEmpty()) { return -1; }
    for (int i = 0; i < mExpr.count(); i++) {
        if (mExpr.at(i).id == id) { return i; }
    }
    return -1;
}

void ExpressionPresets::addExpr(const Expr &expr)
{
    if (!expr.valid) { return; }
    // TODO
}

void ExpressionPresets::remExpr(const int &index)
{
    if (!hasExpr(index)) { return; }
    const auto expr = mExpr.at(index);
    if (expr.core) { return; }

    const QString path = expr.path;
    mExpr.removeAt(index);
    // TODO
    qDebug() << "also remove" << path;
}

void ExpressionPresets::remExpr(const QString &id)
{
    const int index = getExprIndex(id);
    if (index < 0) { return; }
    remExpr(index);
}

bool ExpressionPresets::isValidExprFile(const QString &path)
{
    if (!QFile::exists(path)) { return false; }

    QSettings fexpr(path, QSettings::IniFormat);

    const double version = fexpr.value("version").toDouble();
    const bool hasId = !fexpr.value("id").toString().isEmpty();
    const bool hasBindings = !fexpr.value("bindings").toString().isEmpty();
    const bool hasDefinitions = !fexpr.value("definitions").toString().isEmpty();
    const bool hasScript = !fexpr.value("script").toString().isEmpty();

    if (version >= 0.1 &&
        hasId &&
        (hasBindings || hasDefinitions || hasScript)) { return true; }

    return false;
}

void ExpressionPresets::scanAll(const bool &clear)
{
    if (clear) { mExpr.clear(); }

    QStringList expressions;
    expressions << ":/expressions/clamp.fexpr";

    for (const auto &file : AppSupport::getFilesFromPath(AppSupport::getAppUserExPresetsPath(),
                                                         QStringList() << "*.fexpr")) {
        qDebug() << "Checking user expression" << file;
        if (isValidExprFile(file)) { expressions << file; }
    }

    loadExpr(expressions);
}
