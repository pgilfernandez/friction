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

void ExpressionPresets::scanAll(const bool &clear)
{
    if (clear) { mExpr.clear(); }
    // TODO
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getAll()
{
    return mExpr;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getCore()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid || !expr.core) { continue; }
        list.append(expr);
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getUser()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid ||
            expr.core ||
            expr.path.startsWith(":")) { continue; }
        list.append(expr);
    }
    return list;
}

void ExpressionPresets::loadExpr(const QString &path)
{
    if (!QFile::exists(path)) { return; }
    // TODO
}

bool ExpressionPresets::saveExpr(const int &index,
                                 const QString &path)
{
    if (!hasExpr(index) || path.isEmpty()) { return false; }
    return saveExpr(mExpr.at(index), path);
}

bool ExpressionPresets::saveExpr(const Expr &expr,
                                 const QString &path)
{
    if (!expr.valid || path.isEmpty()) { return false; }
    // TODO
    return false;
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

const ExpressionPresets::Expr
ExpressionPresets::getExpr(const int &index)
{
    if (hasExpr(index)) { return mExpr.at(index); }
    Expr expr;
    expr.valid = false;
    return expr;
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
