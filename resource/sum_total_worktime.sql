SELECT COALESCE(SUM(worktime.finishing_time - worktime.starting_time),
                0) total_worktime
FROM worktime
WHERE worktime.task_id = ?
  AND worktime.finishing_time IS NOT NULL;