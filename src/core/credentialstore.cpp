#include "credentialstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <sodium.h>

using namespace KHeadless;

CredentialStore::CredentialStore(QString path)
    : m_path(path.isEmpty()
                 ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/credentials.json")
                 : std::move(path))
{
    if (sodium_init() < 0) {
        qFatal("Unable to initialize libsodium");
    }
}

bool CredentialStore::load(QString *error)
{
    QFile file(m_path);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QJsonParseError parseError;
    const auto object = QJsonDocument::fromJson(file.readAll(), &parseError).object();
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return false;
    }
    m_hashes.clear();
    for (auto i = object.begin(); i != object.end(); ++i) {
        m_hashes.insert(i.key(), i.value().toString());
    }
    return true;
}

bool CredentialStore::save(QString *error) const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QJsonObject object;
    for (auto i = m_hashes.cbegin(); i != m_hashes.cend(); ++i) {
        object.insert(i.key(), i.value());
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QFile::setPermissions(m_path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

bool CredentialStore::setPassword(const QString &username, const QString &password, QString *error)
{
    const auto normalized = username.trimmed();
    if (normalized.isEmpty() || password.size() < 8) {
        if (error) {
            *error = QStringLiteral("Username is required and passwords must be at least 8 characters");
        }
        return false;
    }
    QByteArray hash(crypto_pwhash_STRBYTES, '\0');
    const auto passwordBytes = password.toUtf8();
    if (crypto_pwhash_str(hash.data(),
                          passwordBytes.constData(),
                          static_cast<unsigned long long>(passwordBytes.size()),
                          crypto_pwhash_OPSLIMIT_MODERATE,
                          crypto_pwhash_MEMLIMIT_MODERATE)
        != 0) {
        if (error) {
            *error = QStringLiteral("Unable to allocate memory for Argon2id");
        }
        return false;
    }
    m_hashes.insert(normalized, QString::fromUtf8(hash.constData()));
    return true;
}

bool CredentialStore::removeUser(const QString &username)
{
    return m_hashes.remove(username) > 0;
}

bool CredentialStore::verify(const QString &username, const QString &password) const
{
    const auto hash = m_hashes.value(username).toUtf8();
    const auto passwordBytes = password.toUtf8();
    return !hash.isEmpty()
        && crypto_pwhash_str_verify(hash.constData(), passwordBytes.constData(), static_cast<unsigned long long>(passwordBytes.size())) == 0;
}

QStringList CredentialStore::users() const
{
    auto result = m_hashes.keys();
    result.sort();
    return result;
}

QString CredentialStore::path() const
{
    return m_path;
}
