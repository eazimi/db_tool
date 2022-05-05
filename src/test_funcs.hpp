#ifndef TEST_FUNCS_HPP
#define TEST_FUNCS_HPP

#include <string>
#include <memory>
#include "dbcompare.h"
#include "global_funcs.hpp"
#include "IDbReader.hpp"

using namespace std;
using namespace Kaco;

namespace Test
{
    static void test_readDbTriggers(const shared_ptr<DbCompare> &db)
    {
        auto triggers_db = db->readDbTriggers();
        print(triggers_db.first, "-> all the triggers in the main db", "main");
        print(triggers_db.second, "-> all the triggers in the ref db", "ref");        
    }

    static void test_readSingleTblTriggers(const shared_ptr<DbCompare> &db, string table_name)
    {
        db->readSingleTblTriggers(table_name);
    }

    static void test_diffTriggerDb(const shared_ptr<DbCompare> &db)
    {
        db->diffTriggerDb();
    }

    static void test_diffTriggerSingleTbl(const shared_ptr<DbCompare> &db, string table_name)
    {
        db->diffTriggerSingleTbl(table_name);
    }

} // namespace Test

#endif