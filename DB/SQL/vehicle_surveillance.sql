-- phpMyAdmin SQL Dump
-- version 5.2.3
-- https://www.phpmyadmin.net/
--
-- Servidor: db
-- Tiempo de generación: 18-05-2026 a las 21:14:18
-- Versión del servidor: 8.0.46
-- Versión de PHP: 8.3.31

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Base de datos: `vehicle_surveillance`
--

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `processing_sessions`
--

CREATE TABLE `processing_sessions` (
  `id` int NOT NULL,
  `video_source_id` int NOT NULL,
  `started_at` datetime NOT NULL,
  `ended_at` datetime DEFAULT NULL,
  `status` varchar(30) NOT NULL,
  `yolo_model` varchar(50) NOT NULL,
  `tracking_model` varchar(50) NOT NULL,
  `frame_skip` int NOT NULL DEFAULT '1',
  `confidence_threshold` decimal(5,4) NOT NULL DEFAULT '0.5000',
  `iou_threshold` decimal(5,4) DEFAULT NULL,
  `notes` text,
  `created_by_user_id` int DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `users`
--

CREATE TABLE `users` (
  `id` int NOT NULL,
  `username` varchar(50) NOT NULL,
  `email` varchar(120) NOT NULL,
  `password_hash` varchar(255) NOT NULL,
  `full_name` varchar(120) DEFAULT NULL,
  `role` varchar(30) NOT NULL DEFAULT 'operator',
  `is_active` tinyint(1) NOT NULL DEFAULT '1',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `last_login_at` datetime DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `user_sessions`
--

CREATE TABLE `user_sessions` (
  `id` int NOT NULL,
  `user_id` int NOT NULL,
  `session_token` varchar(255) NOT NULL,
  `expires_at` datetime NOT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `revoked_at` datetime DEFAULT NULL,
  `ip_address` varchar(45) DEFAULT NULL,
  `user_agent` varchar(255) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `vehicle_ai_analysis`
--

CREATE TABLE `vehicle_ai_analysis` (
  `id` int NOT NULL,
  `track_id` int NOT NULL,
  `snapshot_id` int DEFAULT NULL,
  `requested_by_user_id` int NOT NULL,
  `model_name` varchar(100) NOT NULL,
  `prompt_version` varchar(50) NOT NULL,
  `summary_text` text NOT NULL,
  `attributes_json` json DEFAULT NULL,
  `raw_response` json DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `vehicle_detections`
--

CREATE TABLE `vehicle_detections` (
  `id` int NOT NULL,
  `track_id` int NOT NULL,
  `detected_at` datetime NOT NULL,
  `frame_index` int NOT NULL,
  `bbox_x1` int NOT NULL,
  `bbox_y1` int NOT NULL,
  `bbox_x2` int NOT NULL,
  `bbox_y2` int NOT NULL,
  `confidence` decimal(5,4) NOT NULL,
  `vehicle_class` varchar(30) NOT NULL,
  `center_x` int DEFAULT NULL,
  `center_y` int DEFAULT NULL,
  `width` int DEFAULT NULL,
  `height` int DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `vehicle_snapshots`
--

CREATE TABLE `vehicle_snapshots` (
  `id` int NOT NULL,
  `track_id` int NOT NULL,
  `detection_id` int DEFAULT NULL,
  `file_path` varchar(500) NOT NULL,
  `snapshot_type` varchar(30) NOT NULL,
  `captured_at` datetime NOT NULL,
  `frame_index` int NOT NULL,
  `width` int NOT NULL,
  `height` int NOT NULL,
  `file_size_bytes` bigint DEFAULT NULL,
  `is_best` tinyint(1) NOT NULL DEFAULT '0',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `vehicle_tracks`
--

CREATE TABLE `vehicle_tracks` (
  `id` int NOT NULL,
  `processing_session_id` int NOT NULL,
  `external_track_id` int NOT NULL,
  `vehicle_class` varchar(30) NOT NULL,
  `started_at` datetime NOT NULL,
  `ended_at` datetime DEFAULT NULL,
  `start_frame_index` int DEFAULT NULL,
  `end_frame_index` int DEFAULT NULL,
  `frame_count` int NOT NULL DEFAULT '0',
  `avg_confidence` decimal(5,4) NOT NULL DEFAULT '0.0000',
  `max_confidence` decimal(5,4) DEFAULT NULL,
  `best_snapshot_id` int DEFAULT NULL,
  `status` varchar(30) NOT NULL DEFAULT 'active',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `video_sources`
--

CREATE TABLE `video_sources` (
  `id` int NOT NULL,
  `name` varchar(100) NOT NULL,
  `source_type` varchar(30) NOT NULL,
  `source_path` varchar(500) NOT NULL,
  `description` text,
  `location` varchar(150) DEFAULT NULL,
  `is_active` tinyint(1) NOT NULL DEFAULT '1',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

--
-- Índices para tablas volcadas
--

--
-- Indices de la tabla `processing_sessions`
--
ALTER TABLE `processing_sessions`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_processing_sessions_video_source` (`video_source_id`),
  ADD KEY `fk_processing_sessions_user` (`created_by_user_id`);

--
-- Indices de la tabla `users`
--
ALTER TABLE `users`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `username` (`username`),
  ADD UNIQUE KEY `email` (`email`);

--
-- Indices de la tabla `user_sessions`
--
ALTER TABLE `user_sessions`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `session_token` (`session_token`),
  ADD KEY `fk_user_sessions_user` (`user_id`);

--
-- Indices de la tabla `vehicle_ai_analysis`
--
ALTER TABLE `vehicle_ai_analysis`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_vehicle_ai_analysis_track` (`track_id`),
  ADD KEY `fk_vehicle_ai_analysis_snapshot` (`snapshot_id`),
  ADD KEY `fk_vehicle_ai_analysis_user` (`requested_by_user_id`);

--
-- Indices de la tabla `vehicle_detections`
--
ALTER TABLE `vehicle_detections`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_vehicle_detections_track` (`track_id`);

--
-- Indices de la tabla `vehicle_snapshots`
--
ALTER TABLE `vehicle_snapshots`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_vehicle_snapshots_track` (`track_id`),
  ADD KEY `fk_vehicle_snapshots_detection` (`detection_id`);

--
-- Indices de la tabla `vehicle_tracks`
--
ALTER TABLE `vehicle_tracks`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_vehicle_tracks_processing_session` (`processing_session_id`),
  ADD KEY `fk_vehicle_tracks_best_snapshot` (`best_snapshot_id`);

--
-- Indices de la tabla `video_sources`
--
ALTER TABLE `video_sources`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT de las tablas volcadas
--

--
-- AUTO_INCREMENT de la tabla `processing_sessions`
--
ALTER TABLE `processing_sessions`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `users`
--
ALTER TABLE `users`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `user_sessions`
--
ALTER TABLE `user_sessions`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `vehicle_ai_analysis`
--
ALTER TABLE `vehicle_ai_analysis`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `vehicle_detections`
--
ALTER TABLE `vehicle_detections`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `vehicle_snapshots`
--
ALTER TABLE `vehicle_snapshots`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `vehicle_tracks`
--
ALTER TABLE `vehicle_tracks`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT de la tabla `video_sources`
--
ALTER TABLE `video_sources`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;

--
-- Restricciones para tablas volcadas
--

--
-- Filtros para la tabla `processing_sessions`
--
ALTER TABLE `processing_sessions`
  ADD CONSTRAINT `fk_processing_sessions_user` FOREIGN KEY (`created_by_user_id`) REFERENCES `users` (`id`) ON DELETE SET NULL,
  ADD CONSTRAINT `fk_processing_sessions_video_source` FOREIGN KEY (`video_source_id`) REFERENCES `video_sources` (`id`) ON DELETE CASCADE;

--
-- Filtros para la tabla `user_sessions`
--
ALTER TABLE `user_sessions`
  ADD CONSTRAINT `fk_user_sessions_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`) ON DELETE CASCADE;

--
-- Filtros para la tabla `vehicle_ai_analysis`
--
ALTER TABLE `vehicle_ai_analysis`
  ADD CONSTRAINT `fk_vehicle_ai_analysis_snapshot` FOREIGN KEY (`snapshot_id`) REFERENCES `vehicle_snapshots` (`id`) ON DELETE SET NULL,
  ADD CONSTRAINT `fk_vehicle_ai_analysis_track` FOREIGN KEY (`track_id`) REFERENCES `vehicle_tracks` (`id`) ON DELETE CASCADE,
  ADD CONSTRAINT `fk_vehicle_ai_analysis_user` FOREIGN KEY (`requested_by_user_id`) REFERENCES `users` (`id`) ON DELETE RESTRICT;

--
-- Filtros para la tabla `vehicle_detections`
--
ALTER TABLE `vehicle_detections`
  ADD CONSTRAINT `fk_vehicle_detections_track` FOREIGN KEY (`track_id`) REFERENCES `vehicle_tracks` (`id`) ON DELETE CASCADE;

--
-- Filtros para la tabla `vehicle_snapshots`
--
ALTER TABLE `vehicle_snapshots`
  ADD CONSTRAINT `fk_vehicle_snapshots_detection` FOREIGN KEY (`detection_id`) REFERENCES `vehicle_detections` (`id`) ON DELETE SET NULL,
  ADD CONSTRAINT `fk_vehicle_snapshots_track` FOREIGN KEY (`track_id`) REFERENCES `vehicle_tracks` (`id`) ON DELETE CASCADE;

--
-- Filtros para la tabla `vehicle_tracks`
--
ALTER TABLE `vehicle_tracks`
  ADD CONSTRAINT `fk_vehicle_tracks_best_snapshot` FOREIGN KEY (`best_snapshot_id`) REFERENCES `vehicle_snapshots` (`id`) ON DELETE SET NULL,
  ADD CONSTRAINT `fk_vehicle_tracks_processing_session` FOREIGN KEY (`processing_session_id`) REFERENCES `processing_sessions` (`id`) ON DELETE CASCADE;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
