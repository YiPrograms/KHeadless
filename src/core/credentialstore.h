#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace KHeadless
{

class CredentialStore
{
public:
    explicit CredentialStore(QString path = {});

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;
    bool setPassword(const QString &username, const QString &password, QString *error = nullptr);
    bool removeUser(const QString &username);
    bool verify(const QString &username, const QString &password) const;
    QStringList users() const;
    QString path() const;

private:
    QString m_path;
    QHash<QString, QString> m_hashes;
};

}
