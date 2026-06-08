USE vehicle_surveillance;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(120) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    full_name VARCHAR(120) NULL,
    role VARCHAR(30) NOT NULL DEFAULT 'operator',
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    last_login_at DATETIME NULL
);

CREATE TABLE user_sessions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    session_token VARCHAR(255) NOT NULL UNIQUE,
    expires_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    revoked_at DATETIME NULL,
    ip_address VARCHAR(45) NULL,
    user_agent VARCHAR(255) NULL,
    CONSTRAINT fk_user_sessions_user
        FOREIGN KEY (user_id) REFERENCES users(id)
        ON DELETE CASCADE
);

CREATE TABLE video_sources (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    source_type VARCHAR(30) NOT NULL,
    source_path VARCHAR(500) NOT NULL,
    description TEXT NULL,
    location VARCHAR(150) NULL,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE processing_sessions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    video_source_id INT NOT NULL,
    started_at DATETIME NOT NULL,
    ended_at DATETIME NULL,
    status VARCHAR(30) NOT NULL,
    yolo_model VARCHAR(50) NOT NULL,
    tracking_model VARCHAR(50) NOT NULL,
    frame_skip INT NOT NULL DEFAULT 1,
    confidence_threshold DECIMAL(5,4) NOT NULL DEFAULT 0.5000,
    iou_threshold DECIMAL(5,4) NULL,
    notes TEXT NULL,
    created_by_user_id INT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_processing_sessions_video_source
        FOREIGN KEY (video_source_id) REFERENCES video_sources(id)
        ON DELETE CASCADE,
    CONSTRAINT fk_processing_sessions_user
        FOREIGN KEY (created_by_user_id) REFERENCES users(id)
        ON DELETE SET NULL
);

CREATE TABLE vehicle_tracks (
    id INT AUTO_INCREMENT PRIMARY KEY,
    processing_session_id INT NOT NULL,
    external_track_id INT NOT NULL,
    vehicle_class VARCHAR(30) NOT NULL,
    started_at DATETIME NOT NULL,
    ended_at DATETIME NULL,
    start_frame_index INT NULL,
    end_frame_index INT NULL,
    frame_count INT NOT NULL DEFAULT 0,
    avg_confidence DECIMAL(5,4) NOT NULL DEFAULT 0.0000,
    max_confidence DECIMAL(5,4) NULL,
    best_snapshot_id INT NULL,
    status VARCHAR(30) NOT NULL DEFAULT 'active',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT fk_vehicle_tracks_processing_session
        FOREIGN KEY (processing_session_id) REFERENCES processing_sessions(id)
        ON DELETE CASCADE
);

CREATE TABLE vehicle_detections (
    id INT AUTO_INCREMENT PRIMARY KEY,
    track_id INT NOT NULL,
    detected_at DATETIME NOT NULL,
    frame_index INT NOT NULL,
    bbox_x1 INT NOT NULL,
    bbox_y1 INT NOT NULL,
    bbox_x2 INT NOT NULL,
    bbox_y2 INT NOT NULL,
    confidence DECIMAL(5,4) NOT NULL,
    vehicle_class VARCHAR(30) NOT NULL,
    center_x INT NULL,
    center_y INT NULL,
    width INT NULL,
    height INT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_vehicle_detections_track
        FOREIGN KEY (track_id) REFERENCES vehicle_tracks(id)
        ON DELETE CASCADE
);

CREATE TABLE vehicle_snapshots (
    id INT AUTO_INCREMENT PRIMARY KEY,
    track_id INT NOT NULL,
    detection_id INT NULL,
    file_path VARCHAR(500) NOT NULL,
    snapshot_type VARCHAR(30) NOT NULL,
    captured_at DATETIME NOT NULL,
    frame_index INT NOT NULL,
    width INT NOT NULL,
    height INT NOT NULL,
    file_size_bytes BIGINT NULL,
    is_best BOOLEAN NOT NULL DEFAULT FALSE,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_vehicle_snapshots_track
        FOREIGN KEY (track_id) REFERENCES vehicle_tracks(id)
        ON DELETE CASCADE,
    CONSTRAINT fk_vehicle_snapshots_detection
        FOREIGN KEY (detection_id) REFERENCES vehicle_detections(id)
        ON DELETE SET NULL
);

ALTER TABLE vehicle_tracks
ADD CONSTRAINT fk_vehicle_tracks_best_snapshot
FOREIGN KEY (best_snapshot_id) REFERENCES vehicle_snapshots(id)
ON DELETE SET NULL;

CREATE TABLE vehicle_ai_analysis (
    id INT AUTO_INCREMENT PRIMARY KEY,
    track_id INT NOT NULL,
    snapshot_id INT NULL,
    requested_by_user_id INT NOT NULL,
    model_name VARCHAR(100) NOT NULL,
    prompt_version VARCHAR(50) NOT NULL,
    summary_text TEXT NOT NULL,
    attributes_json JSON NULL,
    raw_response JSON NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_vehicle_ai_analysis_track
        FOREIGN KEY (track_id) REFERENCES vehicle_tracks(id)
        ON DELETE CASCADE,
    CONSTRAINT fk_vehicle_ai_analysis_snapshot
        FOREIGN KEY (snapshot_id) REFERENCES vehicle_snapshots(id)
        ON DELETE SET NULL,
    CONSTRAINT fk_vehicle_ai_analysis_user
        FOREIGN KEY (requested_by_user_id) REFERENCES users(id)
        ON DELETE RESTRICT
);
