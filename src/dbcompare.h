#ifndef DBCOMPARE_H
#define DBCOMPARE_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "trigger.h"
#include "IDbReader.hpp"
#include "data_types.hpp"

using namespace std;

namespace Kaco
{
    class DbCompare
    {
        public:
            DbCompare();
            DbCompare(std::shared_ptr<IDbReader> db1, std::shared_ptr<IDbReader> db2);
            ~DbCompare();

            bool initialize();
            string compareAndMerge();
            void testDbDump();
            void testGetTables();
            void testTableSchema();
            void testTableIndices();
            void testTableTriggers();
            void testTableTriggers(string tableName);
            void testCreateNewTbl();
            void testDiffTableNames();
            void testDiffTableSchemas();
            void test_diffTriggerDb();
            void test_diffTriggerSingleTbl(string table_name);

        private:
            shared_ptr<IDbReader> m_db1, m_db2;
            shared_ptr<Trigger> m_trigger;
            vector<string> m_mainTbls, m_refTbls;
            map<string, string> m_mainTblSchema, m_refTblSchema;
            map<string, string> m_db1TblIndices, m_db2TblIndices;
            MAP_STR_VPS2 m_mainTblTriggers, m_refTblTriggers;
            bool m_initialized;

            void initDbTables();
            void initDbTableSchema();
            void initDbTableIndices();
            void initDbTableTriggers();
            string createNewTbl(string tblName);
            pair<vector<string>, vector<string>> diffTblNames();
            vector<tuple<string, string, string>> diffTblSchemas(); // 0: table name, 1: table schema in main db, 2: table schema in ref db
    };

} // namespace Kaco

#endif


