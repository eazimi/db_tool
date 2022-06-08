#ifndef TABLE_BUILDER_HPP
#define TABLE_BUILDER_HPP

#include "dump_builder.h"
#include <sqlite3.h>
#include <sstream>

namespace Kaco
{
    class DumpTableBuilder : public DumpBuilderBase
    {
        using Self = DumpTableBuilder;
    

    public:
        DumpTableBuilder(Dump &dump) : DumpBuilderBase{dump} {}

        Self &dump_tbls(sqlite3 *db)
        {

            bool rc = dump.prepare(db, 
                    "SELECT sql,tbl_name FROM sqlite_master WHERE type = 'table';", dump.stmt_table);
            if(!rc)
                return *this;
            
            dump.oss << "PRAGMA foreign_keys=OFF;\nBEGIN TRANSACTION;\n";

            rc = sqlite3_step(dump.stmt_table);
            while (rc == SQLITE_ROW)
            {
                string tbl_data = "", tbl_name = "";
                rc = dump.table_info(tbl_data, tbl_name);
                if(!rc)
                    return *this;

                /* CREATE TABLE statements */
                dump.oss << tbl_data << endl;

                /* fetch table data */
                ostringstream ss;
                ss << "SELECT * from " << tbl_name << ";";

                rc = dump.prepare(db, ss.str().c_str(), dump.stmt_data);
                if(!rc)
                    return *this;

                rc = sqlite3_step(dump.stmt_data);
                while (rc == SQLITE_ROW)
                {
                    // sprintf(cmd, "INSERT INTO %s VALUES(", table_name);
                    ss.str("");
                    ss << "INSERT INTO " << tbl_name << " VALUES(";
                    auto col_cnt = sqlite3_column_count(dump.stmt_data);
                    for (auto index = 0; index < col_cnt; index++)
                    {
                        if (index)
                            ss << ",";
                        tbl_data = (const char *)sqlite3_column_text(dump.stmt_data, index);

                        if (!tbl_data.empty())
                        {
                            if (sqlite3_column_type(dump.stmt_data, index) == SQLITE_TEXT)
                                ss << "'" << tbl_data << "'";  
                            else
                                ss << tbl_data;
                        }
                        else
                            ss << "NULL";
                    }
                    dump.oss << ss.str() << ");\n";
                    rc = sqlite3_step(dump.stmt_data);
                }
                rc = sqlite3_step(dump.stmt_table);
            }
            return *this;
        }
    };

} // namespace Kaco

#endif