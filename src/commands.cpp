#include "commands.h"
#include <sstream>
#include <iostream>
#include "global_funcs.hpp"
#include "commands_funcs.hpp"

namespace Kaco
{
    // get the differences by finding records that are in the db_idx1.tbl_name but not in db_idx2.tbl_name
    vector<string> Commands::diff_records(IDbReader *db, string tbl_name, DB_IDX db_idx1, DB_IDX db_idx2)
    {
        stringstream ss;
        ss << "SELECT * FROM " << DB_ALIAS[db_idx1]
           << "." << tbl_name
           << " EXCEPT SELECT * FROM " << DB_ALIAS[db_idx2]   
           << "." << tbl_name
           << ";";
        vector<string> records;
        db->sql_exec(ss.str(), cb_sql_exec, &records);
        return records;
    }
    
    vector<string> Commands::new_records(IDbReader *db, string tbl_name, vector<string> tbl_pk, DB_IDX db_idx1, DB_IDX db_idx2)
    {
        auto alias1 = DB_ALIAS[db_idx1];
        auto alias2 = DB_ALIAS[db_idx2];
        stringstream ss_all_pk, ss_pk2;
        int pk_size = tbl_pk.size();
        for (auto i = 0; i < pk_size; i++)
        {
            if (i > 0)
            {
                ss_all_pk << " AND ";
                ss_pk2 << " AND ";
            }            
            ss_all_pk << alias1 << "." << tbl_name << "." << tbl_pk[i]
                      << "="
                      << alias2 << "." << tbl_name << "." << tbl_pk[i];
            ss_pk2 << alias2 << "." << tbl_name << "." << tbl_pk[i]
                   << " IS NULL";
        }
        stringstream ss;
        ss << "SELECT * FROM " << alias1 << "." << tbl_name
           << " LEFT JOIN " << alias2 << "." << tbl_name 
           << " ON " << ss_all_pk.str()            
           << " WHERE " << ss_pk2.str() << ";";
        vector<string> records;
        db->sql_exec(ss.str(), cb_sql_exec, &records);
        return records;
    }

    vector<string> Commands::modified_records(const vector<string> &diff_recs, const vector<string> &new_recs)
    {
        vector<string> diff_fmt = {}, new_fmt = {};
        auto map_diff = format_recs(diff_recs, diff_fmt);
        auto map_new = format_recs(new_recs, new_fmt);
        auto update_fmt = getDiff(diff_fmt, new_fmt);
        auto update = retrieve_records(update_fmt.first, map_diff);
        return update;
    }

    // arguments: (pair<db, db_idx>, pair<tbl_name, tbl_cols>, record_value)
    int Commands::delete_record(pair<IDbReader *, DB_IDX> db_param, pair<string, vector<string>> tbl_param,
                                string rec_values)
    {
        auto col_val = match_col_val(rec_values, tbl_param.second);
        string str_cv = col_equal_val(col_val, true);
        stringstream ss_del;
        ss_del << "DELETE FROM " << DB_ALIAS[db_param.second]
               << "." << tbl_param.first << " WHERE "
               << str_cv;
        // cout << "delete_record" << endl << ss_del.str() << endl;
        auto rc = db_param.first->sql_exec(ss_del.str(), nullptr, nullptr);
        return rc;
    }

    // arguments: (pair<db, db_idx>, pair<tbl_name, tbl_cols>, record_value)
    int Commands::insert_record(pair<IDbReader *, DB_IDX> db_param, pair<string, vector<string>> tbl_param,
                                string rec_values)
    {
        auto col_val = match_col_val(rec_values, tbl_param.second);
        auto str_cv = col_val_par(col_val, true);
        stringstream ss_ins;
        ss_ins << "INSERT INTO " << DB_ALIAS[DB_IDX::local]
               << "." << tbl_param.first << " "
               << str_cv.first << " VALUES "
               << str_cv.second;
        auto rc = db_param.first->sql_exec(ss_ins.str(), nullptr, nullptr);
        return rc;
    }

    // returns pair<pair<new records in remote, new records in local>,
    //    pair<modified records in remote, modified records in local>>
    PA_PA_VS2 Commands::records_status(IDbReader *db, string tbl_name, vector<string> tbl_cols, vector<string> primary_key)
    {
        auto diff_remote_base = diff_records(db, tbl_name, DB_IDX::remote, DB_IDX::base);
        auto diff_local_base = diff_records(db, tbl_name, DB_IDX::local, DB_IDX::base);
        auto diff_remote_local = diff_records(db, tbl_name, DB_IDX::remote, DB_IDX::local);
        auto diff_local_remote = diff_records(db, tbl_name, DB_IDX::local, DB_IDX::remote);
        
        // bool remote_local_change = !diff_remote_local.empty();
        // bool remote_base_change = !diff_remote_base.empty();
        // bool local_base_change = !diff_local_base.empty();

        if(primary_key.empty())
            return make_pair(make_pair(diff_remote_local, diff_local_remote), make_pair(vector<string>{}, vector<string>{}));

        auto new_remote_local = new_records(db, tbl_name, primary_key, DB_IDX::remote, DB_IDX::local);
        auto new_local_remote = new_records(db, tbl_name, primary_key, DB_IDX::local, DB_IDX::remote);
        auto modified_remote = modified_records(diff_remote_local, new_remote_local);
        auto modified_local = modified_records(diff_local_remote, new_local_remote);

        // insert into local
        /*
            get new records to remote, extract values from the record, create insert command the run it
        */

        // delete from local
        /*
            get the new records to the local, check if they exitst in the base table then delete them
        */

        // update in local
        /*
            find the record in the local that corresponds to a sample record picked from modified records in the remote
            according to the pk

            create the update command for the local table, use the extracted values from the record that comes from
            modified records in the remote in the set clause, use the extracted values from the record that comes
            from modified records in the local in the where clause

            run update command
        */

        map<string, string> map_modified_local = map_col_record(modified_local);

        // for (auto record : modified_remote)
        // {
        //     auto pk_val_remote = pk_value(record);
        //     string rec_local = (map_modified_local.find(pk_val_remote))->second;

        //     delete_record({db, DB_IDX::local}, {tbl_name, tbl_cols}, rec_local);
        //     insert_record({db, DB_IDX::local}, {tbl_name, tbl_cols}, record);
        // }

        return make_pair(make_pair(new_remote_local, new_local_remote), make_pair(modified_remote, modified_local));
    }

} // namespace Kaco