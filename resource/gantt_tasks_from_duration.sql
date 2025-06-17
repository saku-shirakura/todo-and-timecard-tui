SELECT id,
       task_id,
       min(starting_time)  AS starting_time,
       max(finishing_time) AS finishing_time,
       created_at,
       updated_at
FROM null_set_worktime
WHERE (starting_time < ?1 AND finishing_time > ?2)
   OR starting_time BETWEEN ?1 AND ?2
   OR finishing_time BETWEEN ?1 AND ?2
GROUP BY task_id
ORDER BY starting_time, starting_time - finishing_time;