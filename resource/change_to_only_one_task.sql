UPDATE worktime
SET finishing_time = (DATETIME('now', 'unixepoch'))
WHERE finishing_time IS NULL
  AND id NOT IN (SELECT id
                 FROM worktime
                 WHERE finishing_time IS NULL
                 ORDER BY starting_time DESC
                 LIMIT 1 OFFSET 0);
