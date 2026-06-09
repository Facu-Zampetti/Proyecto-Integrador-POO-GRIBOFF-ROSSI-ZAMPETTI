# Plan Técnico del Proyecto: Carlens
## Descripción General

Este proyecto implementa un sistema integral de vigilancia vehicular que permite la detección, seguimiento, registro y análisis inteligente de vehículos observados en múltiples fuentes de video. El despliegue se realiza de manera containerizada sobre un VPS, lo cual facilita la portabilidad, el mantenimiento y la escalabilidad.

---

## 1. Stack Tecnológico

### Frontend: Aplicación de Escritorio en Qt/C++
- **Framework:** Qt Creator
- **Lenguaje:** C++
- **Descripción:** Aplicación desktop que funciona como la interfaz cliente principal.  
  Permite autenticación de usuarios, interacción con videos, visualización de resultados de análisis, y el envío de capturas e información al backend.
- **Comunicación:** Utiliza API REST (protocolo HTTP/HTTPS) para enviar y obtener datos del backend.
- **Subida de imágenes:** Maneja la transferencia de archivos de imagen y metadatos de manera autenticada hacia el VPS a través de API o SFTP.

### Backend
- **Lenguaje:** Python 3.10+
- **Responsabilidad:** Exposición de API REST, lógica de negocio, manejo de usuarios, sesiones, y persistencia de todos los datos.
- **Autenticación:** Validación de credenciales (Basic Auth / JWT).
- **Integración con base de datos:** Usando drivers SQLAlchemy/MySQL.

### Infraestructura / DevOps / Base de Datos / Servidor
- **Plataforma:** VPS Contabo (Ubuntu)
- **Orquestador:** Docker Compose
- **Servicios:**
    - `db` - **MySQL 8**: Base de datos principal del sistema.
    - `phpmyadmin` - **phpMyAdmin**: Administración web de la base de datos.
    - `backend` - Python
    - `nginx` - Reverse proxy, sirve API y paneles, expone phpMyAdmin.
    - `certbot` - Para certificados SSL/TLS (opcional/automatizado).
- **Volúmenes y persistencia:**
    - Volumen para la base de datos.
    - Volumen/directorio para snapshots de imágenes.
- **Seguridad:**
    - Acceso a VPS mediante SSH llaveada.
    - Firewall bloqueando puertos innecesarios.
    - phpMyAdmin protegido con Basic Auth y expuesto sólo por HTTPS/TLS.

---

## 2. Modelo de Base de Datos

Se modelan los siguientes componentes principales:

1. **users:** Usuarios del sistema.
2. **user_sessions:** Sesiones activas de login.
3. **video_sources:** Fuentes de video (archivos, cámaras, streams).
4. **processing_sessions:** Corridas de procesamiento sobre un video.
5. **vehicle_tracks:** Seguimiento de cada vehículo a lo largo del tiempo.
6. **vehicle_detections:** Detecciones frame a frame de cada track.
7. **vehicle_snapshots:** Imágenes almacenadas por cada vehículo/track.
8. **vehicle_ai_analysis:** Resultados de análisis avanzados de IA sobre vehículos/snapshots.

Las imágenes no se guardan como binario en la base, sino como archivos en el VPS, siendo la DB responsable de almacenar la ruta y los metadatos estructurados.

---

## 3. Autenticación

- **Backend/API:**
    - Soporte para Basic Auth (usuario/contraseña).
    - Soporte para JWT (emisión y validación).
    - Los endpoints sensibles requieren autenticación.
- **Frontend (Qt/C++):**
    - Al inicio de sesión se solicitan credenciales.
    - Se maneja el token JWT o autenticación HTTP básica para acceder a la API.
- **phpMyAdmin:**
    - Acceso protegido por Basic Auth configurado en Nginx.
    - Usuarios gestionados desde la propia base MySQL y `.env`.

- **SSH/Servidor:**
    - Acceso al VPS bloqueado por clave SSH.
    - Sólo usuarios autorizados pueden subir imágenes al servidor vía SFTP/SCP o a través de la API autenticada.
- **Gestión de permisos:**
    - El modelo de usuarios y roles permite distinguir operadores, admin, empresa, etc. (customizable según necesidades).

---

## 4. Procesamiento de imágenes y persistencia

- **Aplicación Qt/C++:**  
  Genera las imágenes capturadas, las guarda inicialmente en local, y luego las sube al VPS/servidor vía SFTP/SSH o utilizando un endpoint API HTTP POST autenticado, según configuración.
- El backend recibe y documenta imágenes; la ruta física se registra junto a los metadatos en la base de datos (`vehicle_snapshots`).
- Relaciona las capturas con la sesión, track y metadatos correspondientes.

---

## 5. Seguridad y buenas prácticas

- Todas las keys/secretos se gestionan en `.env` y no se suben al repositorio.
- Acceso a servicios internos sólo desde redes/puertos necesarios.
- phpMyAdmin y Nginx configurados para proteger rutas administrativas.
- Los volúmenes de datos y certificados están persistidos fuera de los contenedores para facilitar backup y restauración.
- El backend valida la autenticidad y permisos del usuario en los endpoints críticos.

---

## 6. Flujo Principal del Sistema (resumido)

1. El usuario inicia sesión desde la aplicación Qt.
2. Selecciona o sube una fuente de video.
3. Activa una sesión de procesamiento.
4. La app detecta vehículos, crea tracks y almacena detecciones.
5. Se generan snapshots de imágenes, que se suben al VPS.
6. Los metadatos se almacenan en la base de datos vía API.
7. El usuario puede consultar, descargar o solicitar análisis avanzados sobre cada vehículo.

---

## 7. Requerimientos para desarrollo y despliegue

- **Docker** y **Docker Compose** instalados en el VPS.
- **Qt Creator** y librerías de C++ según dependencias del frontend.
- Acceso a claves SSH válidas y permisos sobre el VPS para levantar y mantener los servicios.
- Scripts de inicialización (`db/init.sql`) alineados al modelo actual.
- `.env` personalizado para el entorno.

---

## 8. Contacto, soporte y contribuciones

- Cualquier bug, mejora o contribución debe hacerse a través del repositorio oficial [https://github.com/cosimani/vps-poo-2026](https://github.com/cosimani/vps-poo-2026) y sólo *después de analizar el impacto sobre la infraestructura compartida*.
- Los pull requests serán revisados y validados según la política del curso o la organización del equipo.

---

## 9. Roles

- **Ignacio Griboff**: MySQL - phpmyadmin - SQLite - adminDB - Dockercompose - nginx - VPS
- **Gonzalo Rossi**: Python(Backend) - Datamanager - YoloV11 - Gemini - RTSP
- **Facundo Zampetti**: Frontend - QT - Documentación - Informes

---

**Este documento técnico debe mantenerse actualizado ante cualquier cambio en los servicios, modelo de datos o prácticas de seguridad para garantizar la integridad y confiabilidad del sistema.**
