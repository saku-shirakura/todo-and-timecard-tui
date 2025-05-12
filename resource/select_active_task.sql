SELECT *
FROM worktime
WHERE finishing_time IS NULL
ORDER BY starting_time DESC
LIMIT 1 OFFSET 0;
