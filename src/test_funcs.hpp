#ifndef TEST_FUNCS_HPP
#define TEST_FUNCS_HPP

#include <string>
#include <tuple>
#include <cstring>
#include <sstream>
#include "dbcompare.h"
#include "global_funcs.hpp"
#include "global_defines.hpp"
#include "log.h"
#include "log_builder.hpp"

using namespace std;
using namespace Kaco;

namespace Test
{
    static string format_schema_string(string schema)
    {
        ostringstream oss{""};
        auto token = strtok(const_cast<char *>(schema.c_str()), STR_SEPERATOR);
        while(token != nullptr)
        {
            oss << string(1, '"') << token << string(1, '"') << ", ";
            token = strtok(nullptr, STR_SEPERATOR);
        }
        auto oss_str = oss.str();
        int virgool = oss_str.find_last_of(", ");
        oss_str.replace(virgool - 1, 2, "");
        return oss_str;
    }

    static void test_records_status(DbCompare *db)
    {        
        auto common_tbls = db->common_tbls_db(DB_IDX::local, DB_IDX::remote);
        for (auto p : common_tbls)
        {
            auto tbl_name = p.first;
            auto tbl_cols = db->table_cols(tbl_name, DB_IDX::remote);
            auto tbl_pk_local = db->table_pk(tbl_name, DB_IDX::local);
            auto tbl_pk_remote = db->table_pk(tbl_name, DB_IDX::remote);
            auto records = db->records_status(tbl_name);
            if (tbl_pk_local.empty() || tbl_pk_remote.empty())
            {
                cout << Log::create()->add_schema(DB_ALIAS[DB_IDX::remote])
                            .add_table(tbl_name)
                            .add_col_names(tbl_cols)
                            .add_data("", (get<0>(records)).first)
                            .str_no_pk();
                cout << Log::create()->add_schema(DB_ALIAS[DB_IDX::local])
                            .add_table(tbl_name)
                            .add_col_names(tbl_cols)
                            .add_data("", (get<0>(records)).second)
                            .str_no_pk();
                continue;
            }
            cout << Log::create()->add_msg_text("-> new records in ")
                    .add_schema(DB_ALIAS[DB_IDX::remote])
                    .add_table(tbl_name)
                    .add_col_names(tbl_cols)
                    .add_data("", (get<0>(records)).first)
                    .str_records();
            cout << Log::create()->add_msg_text("-> new records in ")
                    .add_schema(DB_ALIAS[DB_IDX::local])
                    .add_table(tbl_name)
                    .add_col_names(tbl_cols)
                    .add_data("", (get<0>(records)).second)
                    .str_records();
            cout << Log::create()->add_msg_text("-> modified records in ")
                    .add_table(tbl_name)
                    .add_col_names(tbl_cols)
                    .add_data("-> old values", (get<1>(records)).second)
                    .add_data("-> new values", (get<1>(records)).first)
                    .str_records();
        }
    }

    static void diff_tables(DbCompare *db)
    {
        auto diff = db->diff_tbls_db(DB_IDX::local, DB_IDX::remote);
        cout << Log::create()->add_msg_multi("-> new/modified tables in ")
                .add_schema(DB_ALIAS[DB_IDX::local])
                .add_msg_multi(" db")
                .add_data("", diff.first)
                .str_diff_tbls();
        cout << Log::create()->add_msg_multi("-> new/modified tables in ")
                .add_schema(DB_ALIAS[DB_IDX::remote])
                .add_msg_multi(" db")
                .add_data("", diff.second)
                .str_diff_tbls();
    }

    static void diff_schema(DbCompare *db)
    {
        auto diff = db->diff_schema_db(DB_IDX::local, DB_IDX::remote);
        auto schema_1 = diff.first;
        auto schema_2 = diff.second;
        vector<tuple<string, string, string>> mdfyd_schema{};
        vector<pair<string, string>> new_local{}, new_remote{};
        string col_names("cid|name|type|notnull|dflt_value|pk");
        for (auto p : schema_1)
        {
            auto tbl_name = p.first;
            auto it = schema_2.find(tbl_name);
            if (it != schema_2.end())
            {
                mdfyd_schema.emplace_back(make_tuple(tbl_name, p.second, it->second));
                schema_2.erase(it->first);
            }
            else
                new_local.emplace_back(make_pair(tbl_name, p.second));
        }
        for (auto p : schema_2)
            new_remote.emplace_back(p);

        cout << Log::create()->add_msg_multi({"-> all the schema differences between", "'s tables", "and",
                                "-> schema of new tables in", "db"})
                                .add_schema(DB_ALIAS[DB_IDX::local])
                                .add_schema_aux(DB_ALIAS[DB_IDX::remote])
                                .add_captions({"-> schema in", " db:"})
                                .add_data({"-> schema in", " db:"}, mdfyd_schema)
                                .add_col_names(col_names)
                                .add_data({new_local, new_remote})
                                .str_schema();

        // cout << string(1, '"') << "-> table schema diff between " << LOCAL
        //      << " and " << REMOTE << string(1, '"') << endl;
        // for (auto tu : mdfyd_schema)
        // {
        //     cout << "[" << get<0>(tu) << "]" << endl
        //          << col_names << endl
        //          << "-> schema in " << LOCAL << " db: " << format_schema_string(get<1>(tu)) << endl
        //          << "-> schema in " << REMOTE << " db: " << format_schema_string(get<2>(tu)) << string(2, '\n');
        // }
        // cout << string(1, '"') << "-> schema of new tables in " << LOCAL << " db" << string(1, '"') << endl;
        // for (auto p : new_local)
        // {
        //     cout << "[" << p.first << "]" << endl
        //          << col_names << endl
        //          << format_schema_string(p.second) << string(2, '\n');
        // }
        // cout << string(1, '"') << "-> schema of new tables in " << REMOTE << " db" << string(1, '"') << endl;
        // for (auto p : new_remote)
        // {
        //     cout << "[" << p.first << "]" << endl
        //          << col_names << endl
        //          << format_schema_string(p.second) << string(2, '\n');
        // }
                                
    }
} // namespace Test

#endif