DROP TABLE IF EXISTS migrate;
DROP TABLE IF EXISTS settings;

CREATE TABLE migrate
(
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    applied    INTEGER NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now')))
);

CREATE TRIGGER trigger_migrate_updated_at
    AFTER UPDATE
    ON migrate
BEGIN
    UPDATE migrate SET updated_at = (strftime('%s', DATETIME('now'))) WHERE rowid == NEW.rowid;
END;

CREATE TABLE settings
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    setting_key TEXT    NOT NULL UNIQUE,
    value       TEXT,
    created_at  INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    updated_at  INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now')))
);

CREATE TRIGGER trigger_settings_updated_at
    AFTER UPDATE
    ON settings
BEGIN
    UPDATE settings SET updated_at = (strftime('%s', DATETIME('now'))) WHERE rowid == NEW.rowid;
END;

INSERT INTO settings(setting_key, value)
VALUES ('Timezone', '+0000'),
       ('Language', 'en'),
       ('LogLevel', 'info');

INSERT INTO migrate (applied)
VALUES (1);