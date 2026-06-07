#ifndef SINGLETON_H
#define SINGLETON_H

#include <QString>

/**
 * @file Singleton.h
 * @brief Template CRTP que encapsula el patrón Singleton.
 *
 * Uso:
 *   class MyManager : public QObject, public Singleton<MyManager> {
 *       friend class Singleton<MyManager>; // acceso al ctor privado
 *       ...
 *   };
 *
 * Beneficio: elimina el boilerplate de s_instance + instance() repetido
 * en cada manager. El compilador genera la instancia estática por tipo.
 *
 * Principios aplicados: templates (CRTP), herencia, encapsulamiento.
 */
template <typename T>
class Singleton {
public:
    /// Retorna la única instancia global de T, creándola si no existe.
    static T* instance()
    {
        if (!s_instance)
            s_instance = new T();
        return s_instance;
    }

    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton()          = default;
    virtual ~Singleton() = default;

private:
    static T* s_instance;
};

template <typename T>
T* Singleton<T>::s_instance = nullptr;

// ── Segunda demo: Result<T> genérico ─────────────────────────────────────
/**
 * @brief Wrapper genérico para el resultado de una operación que puede fallar.
 *
 * Ejemplo de uso:
 *   Result<VehicleModel> r = loadVehicle(id);
 *   if (r.ok) display(r.value);
 *   else      showError(r.error);
 *
 * Principios: template de estructura, programación genérica.
 */
template <typename T>
struct Result {
    bool    ok    = false;
    T       value = {};
    QString error;

    static Result success(const T& v)          { return {true,  v,  {}}; }
    static Result failure(const QString& msg)  { return {false, {}, msg}; }
};

#endif // SINGLETON_H
