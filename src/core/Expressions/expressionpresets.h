#ifndef EXPRESSION_PRESETS_H
#define EXPRESSION_PRESETS_H

#include <QObject>
#include <QList>
#include <QStringList>

namespace Friction
{
    namespace Core
    {
        class ExpressionPresets : public QObject
        {
            Q_OBJECT
        public:
            struct Expr
            {
                bool core = false;
                bool library = false;
                bool valid = false;
                bool enabled = false;
                double version = 0.0; // preset version
                QString path; // preset absolute path
                QString title; // preset title
                QString author; // preset author (optional)
                QString description; // preset description (optional)
                QStringList categories; // preset categories (optional)
                QStringList highlighters; // editor highlighters (optional)
                QString definitions; // editor definitions
                QString bindings; // editor bindings
                QString script; // editor script
            };

            explicit ExpressionPresets(QObject *parent = nullptr);

            void scanAll(const bool &clear = false);
            const QList<Expr> getAll();
            const QList<Expr> getCore();
            const QList<Expr> getUser();

            void loadExpr(const QString &path);
            bool saveExpr(const int &index,
                          const QString &path);
            bool saveExpr(const Expr &expr,
                          const QString &path);
            bool hasExpr(const int &index);
            const Expr getExpr(const int &index);
            void addExpr(const Expr &expr);
            void remExpr(const int &index);

        private:
            QList<Expr> mExpr;
        };
    }
}

#endif // EXPRESSION_PRESETS_H
