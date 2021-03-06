#ifndef IDBREADER_HPP
#define IDBREADER_HPP

#include <sqlite3.h>
#include <string>
#include <vector>
#include "global_defines.hpp"

using ExecCallback = int(void*, int, char**, char**);
using namespace std;

namespace Kaco
{
    class IDbReader
    {
    public:
        virtual bool connect(string dbPath) = 0;
        virtual int attach_db(string dbPath, DB_IDX alias) = 0;
        virtual void dbDump(char *fileName) = 0;
        virtual vector<string> getTables() = 0;
        virtual vector<string> getTableSchema(string tableName) = 0;
        virtual vector<string> getIndices(string tableName) = 0;
        virtual vector<pair<string, string>> getTriggers(string tableName) = 0;
        virtual string getCreateTblCmd(string tblName) = 0;
        virtual int sql_exec(string cmd, ExecCallback cb, vector<string> *results) = 0;
    };

} // namespace Kaco

#endif