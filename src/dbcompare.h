#ifndef DBCOMPARE_H
#define DBCOMPARE_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "trigger.h"
#include "table.h"
#include "IDbReader.hpp"
#include "data_types.hpp"

using namespace std;

namespace Kaco
{
    class DbCompare
    {
        public:
            explicit DbCompare(std::shared_ptr<IDbReader> db1, std::shared_ptr<IDbReader> db2);
            ~DbCompare();

            bool initialize();
            string compareAndMerge();
            void testDbDump();

            void testGetTables();
            void testTableSchema();            

            void testTableIndices();
            void testCreateNewTbl();
            void testDiffTableNames();
            void testDiffTableSchemas();
            PA_VS2 readDbTables() const;
            PA_MAP_S2 readDbTblSchema() const;
            inline PA_MAP_SVPS2 readDbTriggers() const { return m_trigger->readDbTriggers(); };
            PA_VEC_PS2 readSingleTblTriggers(string table_name) const;
            inline PA_MAP_SVPS2 diffTriggerDb() const { return m_trigger->diffTriggerDb(m_mainTbls, m_refTbls); }
            PA_VEC_PS2 diffTriggerSingleTbl(string table_name) const;
            VEC_PS2 updateTriggerSingleTbl(string table_name) const;

        private:
            shared_ptr<IDbReader> m_db1, m_db2;
            shared_ptr<Trigger> m_trigger;
            shared_ptr<Table> m_table;
            vector<string> m_mainTbls, m_refTbls;
            map<string, string> m_mainTblSchema, m_refTblSchema;
            map<string, string> m_db1TblIndices, m_db2TblIndices;
            bool m_initialized;

            void initDbTables();
            void initDbTableSchema();
            void initDbTableIndices();
            string createNewTbl(string tblName);
            pair<vector<string>, vector<string>> diffTblNames();
            vector<tuple<string, string, string>> diffTblSchemas(); // 0: table name, 1: table schema in main db, 2: table schema in ref db
    };

} // namespace Kaco

#endif


