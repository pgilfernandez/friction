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
                bool valid = false;
                bool enabled = false;
                double version = 0.0; // preset version
                QString id; // preset unique ID
                QString path; // preset absolute path
                QString title; // preset title
                QString author; // preset author (optional)
                QString description; // preset description (optional)
                QString url; // preset url (optional)
                QString license; // preset license (optional)
                QStringList categories; // preset categories (optional)
                QStringList highlighters; // editor highlighters (optional)
                QString definitions; // editor definitions
                QString bindings; // editor bindings
                QString script; // editor script
            };

            explicit ExpressionPresets(QObject *parent = nullptr);

            const QList<Expr> getAll();

            const QList<Expr> getCore(const QString &category = QString());
            const QList<Expr> getCoreBindings();

            const QList<Expr> getUser(const QString &category = QString());
            const QList<Expr> getUserBindings();

            void loadExpr(const QString &path);

            bool saveExpr(const int &index,
                          const QString &path);
            bool saveExpr(const QString &id,
                          const QString &path);
            bool saveExpr(const Expr &expr,
                          const QString &path);

            bool hasExpr(const int &index);
            bool hasExpr(const QString &id);

            const Expr getExpr(const int &index);
            const Expr getExpr(const QString &id);

            int getExprIndex(const QString &id);

            void addExpr(const Expr &expr);

            void remExpr(const int &index);
            void remExpr(const QString &id);

            bool isValidExprFile(const QString &path);

        private:
            QList<Expr> mExpr;

            void scanAll(const bool &clear = false);
        };
    }
}

#endif // EXPRESSION_PRESETS_H
