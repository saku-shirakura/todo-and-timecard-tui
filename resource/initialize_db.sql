DROP TRIGGER IF EXISTS trigger_task_updated_at;
DROP TRIGGER IF EXISTS trigger_worktime_updated_at;
DROP TRIGGER IF EXISTS trigger_schedule_updated_at;

DROP INDEX IF EXISTS idx_task_status_id_per_parent;
DROP INDEX IF EXISTS idx_parent_id;
DROP INDEX IF EXISTS idx_worktime_time_per_task;
DROP INDEX IF EXISTS idx_schedule_time_per_task;

DROP TABLE IF EXISTS task;
DROP TABLE IF EXISTS status;
DROP TABLE IF EXISTS worktime;
DROP TABLE IF EXISTS schedule;

DROP VIEW IF EXISTS total_worktime_group_by_task;

CREATE TABLE status
(
    id    INTEGER PRIMARY KEY,
    label TEXT NOT NULL UNIQUE
);

CREATE TABLE task
(
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id  INTEGER REFERENCES task (id) ON DELETE CASCADE             DEFAULT NULL,
    name       TEXT                                              NOT NULL,
    detail     TEXT,
    status_id  INTEGER REFERENCES status (id) ON DELETE RESTRICT NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', DATETIME('now')))
);

CREATE TRIGGER trigger_task_updated_at
    AFTER UPDATE
    ON task
BEGIN
    UPDATE task SET updated_at = strftime('%s', DATETIME('now')) WHERE rowid == NEW.rowid;
END;

CREATE TABLE worktime
(
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id        INTEGER REFERENCES task (id) ON DELETE CASCADE NOT NULL,
    starting_time  INTEGER                                        NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    finishing_time INTEGER                                                 DEFAULT NULL,
    created_at     INTEGER                                        NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    updated_at     INTEGER                                        NOT NULL DEFAULT (strftime('%s', DATETIME('now')))
);

CREATE TRIGGER trigger_worktime_updated_at
    AFTER UPDATE
    ON worktime
BEGIN
    UPDATE worktime SET updated_at = (strftime('%s', DATETIME('now'))) WHERE rowid == NEW.rowid;
END;

CREATE TABLE schedule
(
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id        INTEGER REFERENCES task (id) ON DELETE CASCADE NOT NULL,
    starting_time  INTEGER                                        NOT NULL,
    finishing_time INTEGER                                        NOT NULL,
    created_at     INTEGER                                        NOT NULL DEFAULT (strftime('%s', DATETIME('now'))),
    updated_at     INTEGER                                        NOT NULL DEFAULT (strftime('%s', DATETIME('now')))
);

CREATE TRIGGER trigger_schedule_updated_at
    AFTER UPDATE
    ON schedule
BEGIN
    UPDATE schedule SET updated_at = strftime('%s', DATETIME('now')) WHERE rowid == NEW.rowid;
end;

INSERT INTO status(id, label)
VALUES (1, 'Progress'),
       (2, 'Incomplete'),
       (3, 'Complete'),
       (4, 'Not planned');

CREATE VIEW total_worktime_group_by_task AS
SELECT row_number() AS id,
       task.parent_id parent_task,
       task.id AS     task_id,
       COALESCE(
               SUM(worktime.finishing_time - worktime.starting_time), 0
       )              total_worktime
FROM worktime
         LEFT OUTER JOIN task ON task.id = task_id
GROUP BY task_id;

CREATE INDEX idx_task_status_id_per_parent ON task (parent_id, status_id);
CREATE INDEX idx_parent_id ON task (parent_id);
CREATE INDEX idx_worktime_time_per_task ON worktime (task_id, starting_time, finishing_time);
CREATE INDEX idx_schedule_time_per_task ON schedule (task_id, starting_time, finishing_time);
