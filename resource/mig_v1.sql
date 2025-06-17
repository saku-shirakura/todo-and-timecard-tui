DROP TABLE IF EXISTS migrate;
DROP TABLE IF EXISTS settings;
DROP VIEW IF EXISTS null_set_worktime;

CREATE TABLE migrate
(
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    applied    INTEGER NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (unixepoch(DATETIME('now'))),
    updated_at INTEGER NOT NULL DEFAULT (unixepoch(DATETIME('now')))
);

CREATE TRIGGER trigger_migrate_updated_at
    AFTER UPDATE
    ON migrate
BEGIN
    UPDATE migrate SET updated_at = (unixepoch(DATETIME('now'))) WHERE rowid == NEW.rowid;
END;

CREATE TABLE settings
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    setting_key TEXT    NOT NULL UNIQUE,
    value       TEXT,
    created_at INTEGER NOT NULL DEFAULT (unixepoch(DATETIME('now'))),
    updated_at INTEGER NOT NULL DEFAULT (unixepoch(DATETIME('now')))
);

CREATE TRIGGER trigger_settings_updated_at
    AFTER UPDATE
    ON settings
BEGIN
    UPDATE settings SET updated_at = (unixepoch(DATETIME('now'))) WHERE rowid == NEW.rowid;
END;

CREATE VIEW null_set_worktime AS
SELECT id,
       task_id,
       starting_time,
       coalesce(finishing_time, unixepoch(DATETIME('now'))) AS finishing_time,
       created_at,
       updated_at
FROM worktime;

INSERT INTO settings(setting_key, value)
VALUES ('timezone', '+0000'),
       ('log level', 'info');

INSERT INTO migrate (applied)
VALUES (1);