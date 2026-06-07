#include "models/UserModel.h"
#include <QDebug>
#include <QVariantMap>

UserModel::UserModel(int id,
                     const QString& username,
                     const QString& email,
                     const QString& token)
    : m_id(id)
    , m_username(username)
    , m_email(email)
    , m_token(token)
    , m_createdAt(QDateTime::currentDateTime())
{
}

QVariantMap UserModel::toMap() const
{
    return {
        { "id",         m_id         },
        { "username",   m_username   },
        { "email",      m_email      },
        { "token",      m_token      },
        { "created_at", m_createdAt.toString(Qt::ISODate) }
    };
}

UserModel UserModel::fromMap(const QVariantMap& map)
{
    UserModel user;
    user.setId(map.value("id").toInt());
    user.setUsername(map.value("username").toString());
    user.setEmail(map.value("email").toString());
    user.setToken(map.value("token").toString());
    user.setCreatedAt(QDateTime::fromString(map.value("created_at").toString(),
                                           Qt::ISODate));
    return user;
}

// ── Operadores ─────────────────────────────────────────────────────────────

bool UserModel::operator==(const UserModel& other) const
{
    return m_id == other.m_id;
}

QDebug operator<<(QDebug dbg, const UserModel& u)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace()
        << "UserModel("
        << "id=" << u.m_id
        << ", username=" << u.m_username
        << ", email=" << u.m_email
        << ")";
    return dbg;
}
