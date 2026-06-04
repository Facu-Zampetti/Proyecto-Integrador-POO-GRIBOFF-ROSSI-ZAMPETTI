#ifndef CRYPTOHELPER_H
#define CRYPTOHELPER_H

#include <QString>
#include <QByteArray>

/**
 * @file CryptoHelper.h
 * @brief Utilidades criptográficas. Clase estática (no instanciable).
 *
 * Provee hashing seguro de contraseñas mediante QCryptographicHash.
 * Los métodos son estáticos para uso directo sin instancia.
 *
 * Principios aplicados: static, encapsulamiento, clase de utilidad.
 */
class CryptoHelper {
public:
    CryptoHelper() = delete;
    ~CryptoHelper() = delete;

    /// Hashea un texto con SHA-256. Retorna el hex digest.
    static QString sha256(const QString& plainText);

    /// Hashea un texto con SHA-256 con un salt fijo de la aplicación.
    static QString sha256WithSalt(const QString& plainText);

    /// Genera una cadena hexadecimal aleatoria de `bytes` bytes (para tokens).
    static QString generateRandomHex(int bytes = 16);

    /// Valida si un string parece un hash SHA-256 (64 chars hex).
    static bool isHexHash(const QString& str);

private:
    static constexpr const char* APP_SALT = "Carlens_Salt_2025_v1";
};

#endif // CRYPTOHELPER_H
