WITH RECURSIVE hierarchy_table AS (SELECT id, parent_id
                                   FROM task
                                   WHERE id = ?2
                                   UNION ALL
                                   SELECT B.id, B.parent_id
                                   FROM hierarchy_table AS A
                                            INNER JOIN task AS B
                                   WHERE B.parent_id = A.id)
SELECT EXISTS (SELECT 1
               FROM hierarchy_table
               WHERE id = ?1) AS result;