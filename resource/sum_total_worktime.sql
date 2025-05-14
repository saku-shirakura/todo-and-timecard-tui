WITH RECURSIVE task_children AS (SELECT total_worktime_group_by_task.task_id,
                                        total_worktime_group_by_task.parent_task,
                                        total_worktime_group_by_task.total_worktime
                                 FROM total_worktime_group_by_task
                                 WHERE task_id = ?1
                                 UNION ALL
                                 SELECT total_worktime_group_by_task.task_id,
                                        total_worktime_group_by_task.parent_task,
                                        total_worktime_group_by_task.total_worktime
                                 FROM total_worktime_group_by_task
                                 WHERE parent_task = ?1
                                 UNION ALL
                                 SELECT total_worktime_group_by_task.task_id,
                                        total_worktime_group_by_task.parent_task,
                                        total_worktime_group_by_task.total_worktime
                                 FROM total_worktime_group_by_task
                                          INNER JOIN task_children
                                 WHERE total_worktime_group_by_task.task_id = task_children.parent_task)
SELECT SUM(total_worktime) AS total_worktime
FROM task_children;