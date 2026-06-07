#include "utils/CryptoHelper.h"
#include <QCryptographicHash>
#include <QFile>
#include <QRandomGenerator>
#include <QRegularExpression>

QString CryptoHelper::sha256(const QString& plainText)
{
    QByteArray hash = QCryptographicHash::hash(
        plainText.toUtf8(),
        QCryptographicHash::Sha256
    );
    return QString::fromLatin1(hash.toHex());
}

QString CryptoHelper::sha256WithSalt(const QString& plainText)
{
    QString salted = plainText + APP_SALT;
    return sha256(salted);
}

QString CryptoHelper::generateRandomHex(int bytes)
{
    QByteArray result;
    result.resize(bytes);
    for (int i = 0; i < bytes; ++i) {
        result[i] = static_cast<char>(
            QRandomGenerator::global()->bounded(256));
    }
    return QString::fromLatin1(result.toHex());
}

bool CryptoHelper::isHexHash(const QString& str)
{
    if (str.length() != 64) return false;
    static const QRegularExpression hexRx("^[0-9a-fA-F]{64}$");
    return hexRx.match(str).hasMatch();
}

QString CryptoHelper::md5(const QString& text)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString CryptoHelper::md5File(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QCryptographicHash hash(QCryptographicHash::Md5);
    while (!file.atEnd())
        hash.addData(file.read(65536));  // 64 KB chunks

    return QString::fromLatin1(hash.result().toHex());
}
