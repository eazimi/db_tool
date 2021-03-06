#include "dbcompare.h"
#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "global_funcs.hpp"
#include "data_types.hpp"
#include "global_defines.hpp"

using namespace std;

// #define DEBUG
namespace Kaco
{
    static map<string, string> initTableIndices(const shared_ptr<IDbReader> &db, const vector<string> &tables)
    {
        map<string, string> result{};
        for (auto table : tables)
        {
            auto tableIndex = db->getIndices(table);
            stringstream ss;
            for (auto i = 0; i < tableIndex.size(); i++)
            {
                if (i)
                    ss << STR_SEPERATOR;
                ss << tableIndex[i];
            }
            result.insert({table, ss.str()});
        }
        return result;
    }

    DbCompare::DbCompare(std::shared_ptr<IDbReader> db1, std::shared_ptr<IDbReader> db2) : m_db1(db1), m_db2(db2), m_initialized(false)
    {
        m_trigger = make_shared<Trigger>(db1.get(), db2.get());
        m_table = make_shared<Table>(db1.get(), db2.get());
        INIT_MAPS;
    }

    DbCompare::DbCompare(std::shared_ptr<IDbReader> local_db, 
                         std::shared_ptr<IDbReader> remote_db, 
                         std::shared_ptr<IDbReader> base_db) : m_db1(local_db), m_db2(remote_db), m_base_db(base_db), m_initialized(false)
    {
        m_trigger = make_shared<Trigger>(local_db.get(), remote_db.get(), base_db.get());
        m_table = make_shared<Table>(local_db.get(), remote_db.get(), base_db.get());
        m_commands = make_shared<Commands>();
        m_master_db = local_db;
        INIT_MAPS;
    }

    DbCompare::~DbCompare()
    {
        CLEAR_MAPS;
    }

    bool DbCompare::initialize()
    {
        CHECK_INITIALIZED(m_initialized, MSG_ALREADY_INIT, FALSE);
        m_table->init_tbls();
        auto db_tbls = m_table->read_tbl_db();
        m_trigger->init_triggers(db_tbls.first, db_tbls.second);
        m_trigger->init_triggers(db_tbls.first, db_tbls.second, {});
        m_table->init_tbl_schema();
        initDbTableIndices();
        m_initialized = true;
        return m_initialized;
    }

    string DbCompare::compareAndMerge()
    {
        CHECK_INITIALIZED(!m_initialized, MSG_NOT_INIT, STR_NULL);

        string result = {};
        auto main_schema = m_table->read_tschema_db().first;
        auto ref_schema = m_table->read_tschema_db().second;
        // db1: source -> db2: target
        for (auto tblName_schema : main_schema)
        {
            auto srcTblName = tblName_schema.first;
            // bool targetTblFound = (m_refTblSchema.find(srcTblName) != m_refTblSchema.end());
            bool targetTblFound = false;
            auto targetTblSchema = ref_schema[srcTblName];
            if (targetTblSchema != "")
                targetTblFound = true;
            if (targetTblFound)
            {
                // check for schemas
                auto srcTblSchema = tblName_schema.second;
                auto targetTblSchema = ref_schema[srcTblName];
                if (srcTblSchema == targetTblSchema)
                {
                    // check for the indices
                    auto srcTblIndices = m_db1TblIndices[srcTblName];
                    auto targetTblIndices = m_db2TblIndices[srcTblName];
                    if (srcTblIndices == targetTblIndices)
                    {
                        // check for triggers
                        // auto srcTblTriggers = m_mainTblTriggers[srcTblName];
                        // auto targetTblTriggers = m_refTblTriggers[srcTblName];
                        // if (srcTblTriggers == targetTblTriggers)
                        // {
                        //     // TODO: complete it
                        //     // check for data
                        // }
                        // else
                        // {
                        //     // TODO: complete it
                        //     // different triggers
                        //     cout << srcTblName << " is different in triggers" << endl;
                        // }
                    }
                    else
                    {
                        // TODO: complete it
                        // different indices
                        cout << srcTblName << " is different in indices" << endl;
                    }
                }
                else
                {
                    // TODO: complete it
                    // different schemas
                    cout << srcTblName << " is different in schema" << endl;
                }
            }
            else
            {
                // TODO: complete it
                // new table
                cout << srcTblName << " is new to db2" << endl;
            }
        }

        return result;
    }

    void DbCompare::dump_db(DB_IDX db_idx, string path)
    {
        switch (db_idx)
        {
        case DB_IDX::local:
            m_db1->dbDump(const_cast<char *>(path.c_str()));
            break;

        case DB_IDX::remote:
            m_db2->dbDump(const_cast<char *>(path.c_str()));
            break;

        default:
            m_base_db->dbDump(const_cast<char *>(path.c_str()));
            break;
        }
    }

    void DbCompare::testTableIndices()
    {
        print(m_db1TblIndices, "-> db1 all the table indices");
        print(m_db2TblIndices, "-> db2 all the table indices");
    }

    PA_VS2 DbCompare::readDbTables() const
    {
        return m_table->read_tbl_db();
    }

    PA_MAP_S2 DbCompare::readDbTblSchema() const
    {
        return m_table->read_tschema_db();
    }

    PA_VEC_PS2 DbCompare::readSingleTblTriggers(string table_name) const
    {
        return m_trigger->read_trigger_tbl(table_name);
    }

    PA_MAP_SVPS2 DbCompare::diffTriggerDb() const
    {
        auto db_tbls = m_table->read_tbl_db();
        return m_trigger->diff_trigger_db(db_tbls.first, db_tbls.second);
    }

    PA_VEC_PS2 DbCompare::diffTriggerSingleTbl(string table_name) const
    {
        return m_trigger->diff_trigger_tbl(table_name);
    }

    PA_VEC_PS2 DbCompare::diffTriggerSingleTbl(string table_name, DB_IDX db_idx1, DB_IDX db_idx2) const
    {
        return m_trigger->diff_trigger_tbl(table_name, db_idx1, db_idx2);
    }

    VEC_PS2 DbCompare::updateTriggerSingleTbl(string table_name) const
    {
        return m_trigger->update_trigger_tbl(table_name);
    }

    int DbCompare::attach_db(string remote_path, string base_path)
    {
        auto rc = m_master_db->attach_db(remote_path, DB_IDX::remote);
        if(rc == SQLITE_OK)
            rc = m_master_db->attach_db(base_path, DB_IDX::base);
        return rc;
    }

    vector<string> DbCompare::diff_records(string tbl_name, DB_IDX db_idx1, DB_IDX db_idx2) const
    {
        return m_commands->diff_records(m_master_db.get(), tbl_name, db_idx1, db_idx2);
    }

    PA_PA_VS2 DbCompare::records_status(string tbl_name) const
    {   
        auto pk = m_table->table_pk(tbl_name, DB_IDX::remote);
        auto tbl_cols = m_table->table_cols(tbl_name, DB_IDX::remote);
        return m_commands->records_status(m_master_db.get(), tbl_name, tbl_cols, pk);
    }

    vector<string> DbCompare::table_cols(string tbl_name, DB_IDX db_idx) const
    {
        return m_table->table_cols(tbl_name, db_idx);
    }

    void DbCompare::initDbTableIndices()
    {
        auto db_tbls = m_table->read_tbl_db();
        auto tableIndices1 = initTableIndices(m_db1, db_tbls.first);
        m_db1TblIndices = std::move(tableIndices1);

        auto tableIndices2 = initTableIndices(m_db2, db_tbls.second);
        m_db2TblIndices = std::move(tableIndices2);
    }

} // namespace Kaco
