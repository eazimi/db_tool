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
            bool rc_bool = dump.prepare(db, 
                    "SELECT sql,tbl_name FROM sqlite_master WHERE type = 'table';", &dump.stmt_table);
            if(!rc_bool)
                return *this;
            dump.oss << "PRAGMA foreign_keys=OFF;\nBEGIN TRANSACTION;\n";
            auto rc = sqlite3_step(dump.stmt_table);
            while (rc == SQLITE_ROW)
            {
                string tbl_data = "", tbl_name = "";
                rc_bool = dump.table_info(tbl_data, tbl_name);
                if(!rc_bool)
                    return *this;

                /* CREATE TABLE statements */
                dump.oss << tbl_data << ";\n";

                /* fetch table data */
                ostringstream ss;
                ss << "SELECT * from " << tbl_name << ";";
                rc_bool = dump.prepare(db, ss.str().c_str(), &dump.stmt_data);
                if(!rc_bool)
                    return *this;
                auto tbl_records = dump.table_data(tbl_name);
                dump.oss << tbl_records;
                rc = sqlite3_step(dump.stmt_table);
            }
            return *this;
        }
    };

} // namespace Kaco

#endif