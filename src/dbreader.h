#ifndef DBREADER_H
#define DBREADER_H

#include <string>
#include <functional>
#include <vector>
#include <utility>
#include "IDbReader.hpp"
#include "global_defines.hpp"

using namespace std;

namespace Kaco
{    
    class DbReader : public IDbReader
    {
        // TODO: set flags by a function
        // TODO: read path from a config file
        // TODO: refactor dbDump()
        public:
            explicit DbReader() = default;
            ~DbReader();

            bool connect(string dbPath);
            int attach_db(string dbPath, DB_IDX alias);
            int sql_exec(string cmd, ExecCallback cb, vector<string> *results);
            void dbDump(char *fileName);
            void command_exec(string dbPath, string output, string command);
            void dbDiff(string dbPath1, string dbPath2, string output);
            vector<string> getTables();
            vector<string> getTableSchema(string tableName);
            string getCreateTblCmd(string tblName);
            vector<pair<string, string>> getTriggers(string tableName);
            vector<string> getIndices(string tableName);

        private:
            sqlite3 *db;
    };

} // namespace Kaco

#endif