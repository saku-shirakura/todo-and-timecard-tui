UPDATE worktime
SET finishing_time = (strftime('%s', DATETIME('now')))
WHERE finishing_time IS NULL
  AND id NOT IN (SELECT id
                 FROM worktime
                 WHERE finishing_time IS NULL
                 ORDER BY id DESC
                 LIMIT 1 OFFSET 0);
