SELECT COALESCE(SUM(total_worktime), 0) AS total_worktime
FROM total_worktime_group_by_task
WHERE task_id IN (WITH RECURSIVE hierarchy_table AS (SELECT id, parent_id
                                                     FROM task
                                                     WHERE id = ?1
                                                     UNION ALL
                                                     SELECT B.id, B.parent_id
                                                     FROM hierarchy_table AS A
                                                              INNER JOIN task AS B
                                                     WHERE B.parent_id = A.id)
                  SELECT id
                  FROM hierarchy_table);