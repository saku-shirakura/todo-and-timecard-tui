SELECT SUM(COALESCE(worktime.finishing_time, STRFTIME('%s', DATETIME('now'))) - worktime.starting_time) AS sum_worktime
FROM worktime
WHERE worktime.task_id = ?;