SELECT ceil(row_id / ? /* per page*/) + 1 AS page_pos
FROM (SELECT id, row_number() over (ORDER BY status_id, name) AS row_id
      FROM task
      WHERE parent_id IS NULL
        AND status_id = ?)
WHERE id = ?;
