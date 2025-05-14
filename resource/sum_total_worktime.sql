SELECT COALESCE(SUM(total_worktime), 0) AS total_worktime
FROM total_worktime_group_by_task
WHERE task_id = ?1
   OR parent_task = ?1;