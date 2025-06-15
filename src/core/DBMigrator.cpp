// MIT License
//
// Copyright (c) 2025 Saku Shirakura <saku@sakushira.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "DBMigrator.h"

void core::db::DBMigrator::migrate()
{
    NoMappingTable tbl{};
    tbl.usePlaceholderUniSql("SELECT 1 FROM pragma_table_info('migrate') LIMIT 1;");
    if (tbl.getRawTable().empty()) { DBManager::execute(std::string(F_MIG_V1_SQL, SIZE_MIG_V1_SQL)); }

    tbl.usePlaceholderUniSql("SELECT 1 AS id, MAX(applied) AS applied FROM migrate;");
    long long latest_applied = 0;
    if (!tbl.getRawTable().empty())
        latest_applied = getLongLong(tbl.getRawTable().front().at("applied"));
    while (latest_applied < stoll(std::string(F_MIGRATE_LATEST_, SIZE_MIGRATE_LATEST_))) {
        DBManager::execute(_migration_sql.at(latest_applied));
        latest_applied++;
    }
}

std::vector<std::string> core::db::DBMigrator::_migration_sql{
    std::string(F_MIG_V1_SQL, SIZE_MIG_V1_SQL)
};
