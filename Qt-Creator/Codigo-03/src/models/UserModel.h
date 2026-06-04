#ifndef USERMODEL_H
#define USERMODEL_H

#include <QString>
#include <QDateTime>

/**
 * @file UserModel.h
 * @brief Modelo de datos del usuario autenticado.
 *
 * Encapsula la información del usuario retornada por el backend.
 * Principio aplicado: Encapsulamiento, separación de datos.
 */
class UserModel {
public:
    UserModel() = default;

    explicit UserModel(int id,
                       const QString& username,
                       const QString& email,
                       const QString& token = QString());

    // ── Getters ───────────────────────────────────────────────────────
    int            id()        const { return m_id; }
    const QString& username()  const { return m_username; }
    const QString& email()     const { return m_email; }
    const QString& token()     const { return m_token; }
    const QDateTime& createdAt() const { return m_createdAt; }
    bool isValid()             const { return m_id > 0 && !m_username.isEmpty(); }

    // ── Setters ───────────────────────────────────────────────────────
    void setId(int id)                       { m_id = id; }
    void setUsername(const QString& username) { m_username = username; }
    void setEmail(const QString& email)       { m_email = email; }
    void setToken(const QString& token)       { m_token = token; }
    void setCreatedAt(const QDateTime& dt)    { m_createdAt = dt; }

    /// Serializa el modelo como mapa clave-valor para SQLite.
    QVariantMap toMap() const;

    /// Crea un UserModel desde un mapa clave-valor (desde SQLite o JSON).
    static UserModel fromMap(const QVariantMap& map);

private:
    int       m_id       = 0;
    QString   m_username;
    QString   m_email;
    QString   m_token;
    QDateTime m_createdAt;
};

#endif // USERMODEL_H
