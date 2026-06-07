#ifndef IMANAGER_H
#define IMANAGER_H

/**
 * @file IManager.h
 * @brief Interfaz base para todos los Managers del sistema.
 *
 * Define el contrato mínimo que todo Manager debe cumplir:
 * - initialize(): configurar recursos del manager.
 * - shutdown():   liberar recursos.
 * - isInitialized(): consultar estado.
 *
 * Principio aplicado: Abstracción + Polimorfismo (clases abstractas).
 */
class IManager {
public:
    virtual ~IManager() = default;

    /// Inicializa el manager (abre DB, carga config, etc.)
    virtual void initialize() = 0;

    /// Libera todos los recursos del manager.
    virtual void shutdown() = 0;

    /// Retorna true si el manager fue inicializado correctamente.
    virtual bool isInitialized() const = 0;
};

#endif // IMANAGER_H
