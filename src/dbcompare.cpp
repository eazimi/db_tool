#include "dbcompare.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stack>
#include <unordered_set>
#include <unordered_map>

using namespace std;

// #define DEBUG

#define STR_SEPERATOR "##"
#define VAL_SEPERATOR "|"
#define STR_NULL std::string("")
#define FALSE false
#define SCHEMA_REF "ref"
#define SCHEMA_MAIN "main"
#define CT_REF "REFERENCES"
#define MSG_ALREADY_INIT "DbCompare is already initialized."
#define MSG_NOT_INIT "DbCompare is not initialized. Call initialize() first!"

#define CT_CONSTRAINT   "CONSTRAINT"
#define CT_PRIMARY      "PRIMARY"
#define CT_UNIQUE       "UNIQUE"
#define CT_CHECK        "CHECK"
#define CT_FOREIGN      "FOREIGN"

#define INIT_VECTORS  \
    m_db1Tables = {}; \
    m_db2Tables = {};

#define INIT_MAPS                               \
    m_db1TblSchema = {};                        \
    m_db2TblSchema = {};                        \
    m_db1TblIndices = {}, m_db2TblIndices = {}; \
    m_db1TblTriggers = {}, m_db2TblTriggers = {};

#define CLEAR_MAPS            \
    m_db1TblSchema.clear();   \
    m_db2TblSchema.clear();   \
    m_db1TblIndices.clear();  \
    m_db2TblIndices.clear();  \
    m_db1TblTriggers.clear(); \
    m_db2TblTriggers.clear();

#define CHECK_INITIALIZED(x, message, r) \
    if ((x))                             \
    {                                    \
        cout << message                  \
             << endl;                    \
        return r;                        \
    }

#define CHECK_IS_TBL_CONSTRAINT(x) \
    (((x) == CT_CONSTRAINT) ? true : (((x) == CT_PRIMARY) ? true : (((x) == CT_UNIQUE) ? true : (((x) == CT_CHECK) ? true : (((x) == CT_FOREIGN) ? true : false)))))

namespace Kaco
{
    template <typename T>
    static void print(string message, T data)
    {
        cout << endl
             << "\"" << message << "\"" << endl;
        for (const auto &str : data)
            cout << str << endl;
    }

    // true: print value, false: print key
    static auto mprint = [](string message, unordered_map<string, string> data, bool keyVal = true)
    {
        cout << endl
             << message << endl;
        for (const auto &str : data)
        {
            auto val = (keyVal) ? str.second : str.first;
            cout << val << endl;
        }
    };

    static auto updateRefTable = [](const vector<string> &cols, string schema)
    {
        vector<string> updated_cols = {};
        for (auto rec : cols)
        {
            size_t pos = rec.find(CT_REF);
        int ctref_size = ((string)CT_REF).length();
        while (pos != string::npos)
        {
            int index = pos + ctref_size + 1; // right on the beginning of the table name
                pos = rec.find_first_of(" ", index);
                if (pos != string::npos)
            {
                int tblname_len = pos - index;
                    auto tblName = rec.substr(index, tblname_len);
                stringstream ss;
                    ss << "\"" << schema << "\""
                       << "." << tblName;
                auto newtbl_name = ss.str();
                int newtblname_size = newtbl_name.size();
                    rec.replace(index, tblname_len, newtbl_name);
                    pos = rec.find(CT_REF, index + newtblname_size);
            }
            else
                break;
        }
            updated_cols.push_back(std::move(rec));
        }
        return updated_cols;
    };

    static auto updateColNames = [](unordered_map<string, string> cols, string schema, string tblName)
    {
        vector<string> updatedCols = {};
        for (auto col : cols)
        {
            auto colName = col.first;
            auto colDef = col.second;
            size_t pos = colDef.find(colName);
            if (pos != string::npos)
            {
                int col_length = colName.length();
                char ch = colDef[pos + col_length];
                if ((ch == ' ') || (ch == ',') || (ch == ')'))
                {
                    stringstream ss;
                    ss << "\"" << schema << "\""
                       << "." << tblName << "." << colName;
                    colDef.replace(pos, col_length, ss.str());
                    ss.str("");
                }
                updatedCols.push_back(std::move(colDef));
            }
        }
        return updatedCols;
    };

    static auto updateColNameInConstraints = [](vector<string> constraints, vector<string> cols, string schema, string tblName)
    {
        vector<string> updated_constraints = {};
        for (auto constraint : constraints)
        {
            for (auto col : cols)
            {
                int index = 0;
                do
                {
                    size_t pos = constraint.find(col, index);
                    if (pos != string::npos)
                    {
                        int col_length = col.length();
                        auto next_pos = pos + col_length;
                        char ch = constraint[next_pos];
                        if ((ch == ' ') || (ch == ',') || (ch == ')'))
                        {
                            stringstream ss;
                            ss << "\"" << schema << "\""
                               << "." << tblName << "." << col;
                            auto ss_str = ss.str();
                            constraint.replace(pos, col_length, ss_str);
                            ss.str("");
                            index += (pos + ss_str.length());
                        }
                        else
                            index = next_pos;
                    }
                    else
                        break;
                } while (true);
            }
            updated_constraints.push_back(std::move(constraint));
        }
        return updated_constraints;
    };    

    static auto getColNamesDetails = [](unordered_map<string, string> cols)
    {
        vector<string> col_name = {};
        vector<string> col_detail = {};
        for (const auto &str : cols)
        {
            col_name.push_back(str.first);
            col_detail.push_back(str.second);
        }
        return make_pair(col_name, col_detail);
    };

    static auto mergeColsAndConstraint = [](vector<string> cols, vector<string> constraints)
    {
        int col_size = cols.size();
        vector<string> column_constrain(col_size + constraints.size());
        column_constrain.assign(cols.begin(), cols.end());
        column_constrain.insert(column_constrain.begin() + col_size, constraints.begin(), constraints.end());
        return column_constrain;
    };

    static auto getColsAndConstraints = [](vector<string> colsCons)
    {
        unordered_map<string, string> cols = {};
        vector<string> constraints = {};
        for (auto str : colsCons)
        {
            int pos = str.find_first_of(" ");
            auto col = str.substr(0, pos);
            bool isTblConstraint = CHECK_IS_TBL_CONSTRAINT(col);
            if (!isTblConstraint)
                cols.insert({std::move(col), std::move(str)});
            else
                constraints.push_back(std::move(str));
        }
        return make_pair(cols, constraints);
    };

    static auto checkForMatch = [](char *stream)
    {
        stack<char> stack;
        for (auto i = 0; stream[i] != '\0'; i++)
        {
            if ((stream[i] == '(') || (stream[i] == '['))
                stack.push(stream[i]);
            else if ((stream[i] == ')') || (stream[i] == ']')) // I assume that [ and ( are followed by closings immediately
                stack.pop();
        }
        if (stack.empty())
            return true;
        return false;
    };

    // it assumes that there is no disparity between the number of '(' and ')'
    static auto handleWord = [](string word, stack<char> &stack)
    {
        bool stackIsEmpty = false;
        bool virgool = false;
        for (auto ch : word)
        {
            if (ch == '(')
                stack.push('(');
            else if (ch == ')')
                stack.pop();
            else if (ch == ',')
            {
                virgool = true;
                stackIsEmpty = stack.empty();
            }
        }
        return (stackIsEmpty && virgool);
    };

    static vector<string>
    splitCreateTblCmd(string cmd)
    {
        vector<string> cols = {};
        int firstPos = cmd.find_first_of("(");
        int lastPos = cmd.find_last_of(")");

        auto data = cmd.substr(firstPos + 1, lastPos - firstPos - 1);
        int spacePos = 0;
        stack<char> stack;
        stringstream ss;
        spacePos = data.find_first_of(" ");

#ifdef DEBUG
        cout << endl
             << "splitCreateTblCmd(): " << endl;
        cout << "input cmd is: " << endl
             << cmd << endl;
        cout << endl
             << "data is: " << endl
             << data << endl;
#endif

        while (spacePos != string::npos)
        {
            auto word = data.substr(0, spacePos);
            bool flush = handleWord(word, stack);
            if (flush)
            {
                ss << word.substr(0, word.length() - 1);
                cols.push_back(std::move(ss.str()));
                ss.str("");
            }
            else
                ss << word << " ";
            data = data.substr(spacePos + 1);
            spacePos = data.find_first_of(" ");
        }
        ss << data;
        cols.push_back(ss.str());
        return cols;
    }

    static pair<vector<string>, vector<string>> getColsConsDiff(vector<string> targetCols, vector<string> refCols)
    {
        vector<string> targetDiff = {};
        vector<string> refDiff = {};
        sort(targetCols.begin(), targetCols.end());
        sort(refCols.begin(), refCols.end());
        set_difference(targetCols.begin(), targetCols.end(), refCols.begin(), refCols.end(), back_inserter(targetDiff));
        set_difference(refCols.begin(), refCols.end(), targetCols.begin(), targetCols.end(), back_inserter(refDiff));
        return {targetDiff, refDiff};
    }

    static vector<string> getColsConsIntersect(vector<string> targetCols, vector<string> refCols)
        {
        vector<string> intersect = {};
        sort(targetCols.begin(), targetCols.end());
        sort(refCols.begin(), refCols.end());
        set_intersection(targetCols.begin(), targetCols.end(), refCols.begin(), refCols.end(), 
            std::inserter(intersect, intersect.begin()));
        return intersect;
    }

    static map<string, string> initTablesSchema(const shared_ptr<IDbReader> &db, const vector<string> &tables)
    {
        map<string, string> result{};

        for (auto table : tables)
        {
            auto tableSchema = db->getTableSchema(table);
            stringstream ss;
            for (auto i = 0; i < tableSchema.size(); i++)
            {
                if (i)
                    ss << STR_SEPERATOR;
                ss << tableSchema[i];
            }
            result.insert({table, ss.str()});
        }

        return result;
    }

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

    static map<string, string> initTableTriggers(const shared_ptr<IDbReader> &db, const vector<string> &tables)
    {
        map<string, string> result{};

        for (auto table : tables)
        {
            auto tableTriggers = db->getTriggers(table);
            stringstream ss;
            for (auto i = 0; i < tableTriggers.size(); i++)
            {
                if (i)
                    ss << STR_SEPERATOR;
                ss << tableTriggers[i].first << VAL_SEPERATOR << tableTriggers[i].second;
            }
            result.insert({table, ss.str()});
        }

        return result;
    }

    DbCompare::DbCompare() : m_db1(nullptr), m_db2(nullptr), m_initialized(false)
    {
        INIT_VECTORS;
        INIT_MAPS;
    }

    DbCompare::DbCompare(std::shared_ptr<IDbReader> db1, std::shared_ptr<IDbReader> db2) : m_db1(db1), m_db2(db2), m_initialized(false)
    {
        INIT_VECTORS;
        INIT_MAPS;
    }

    DbCompare::~DbCompare()
    {
        CLEAR_MAPS;
    }

    bool DbCompare::initialize()
    {
        CHECK_INITIALIZED(m_initialized, MSG_ALREADY_INIT, FALSE);

        initDbTables();
        initDbTableSchema();
        initDbTableIndices();
        initDbTableTriggers();

        m_initialized = true;
        return m_initialized;
    }

    string DbCompare::compareAndMerge()
    {
        CHECK_INITIALIZED(!m_initialized, MSG_NOT_INIT, STR_NULL);

        string result{};
        // db1: source -> db2: target
        for (auto tblName_schema : m_db1TblSchema)
        {
            auto srcTblName = tblName_schema.first;
            // bool targetTblFound = (m_db2TblSchema.find(srcTblName) != m_db2TblSchema.end());
            bool targetTblFound = false;
            auto targetTblSchema = m_db2TblSchema[srcTblName];
            if (targetTblSchema != "")
                targetTblFound = true;
            if (targetTblFound)
            {
                // check for schemas
                auto srcTblSchema = tblName_schema.second;
                auto targetTblSchema = m_db2TblSchema[srcTblName];
                if (srcTblSchema == targetTblSchema)
                {
                    // check for the indices
                    auto srcTblIndices = m_db1TblIndices[srcTblName];
                    auto targetTblIndices = m_db2TblIndices[srcTblName];
                    if (srcTblIndices == targetTblIndices)
                    {
                        // check for triggers
                        auto srcTblTriggers = m_db1TblTriggers[srcTblName];
                        auto targetTblTriggers = m_db2TblTriggers[srcTblName];
                        if (srcTblTriggers == targetTblTriggers)
                        {
                            // TODO: complete it
                            // check for data
                        }
                        else
                        {
                            // TODO: complete it
                            // different triggers
                            cout << srcTblName << " is different in triggers" << endl;
                        }
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

    void DbCompare::testTableSchema()
    {
        cout << "db1 all the table schemas: " << endl;
        for (auto str : m_db1TblSchema)
            cout << str.first << ": " << str.second << endl;
        cout << endl
             << "db2 all the table schemas: " << endl;
        for (auto str : m_db2TblSchema)
            cout << str.first << ": " << str.second << endl;
    }

    void DbCompare::testTableIndices()
    {
        cout << "db1 all the table indices: " << endl;
        for (auto str : m_db1TblIndices)
            cout << str.first << ": " << str.second << endl;
        cout << endl
             << "db2 all the table indices: " << endl;
        for (auto str : m_db2TblIndices)
            cout << str.first << ": " << str.second << endl;
    }

    void DbCompare::testTableTriggers()
    {
        cout << "db1 all the table triggers: " << endl;
        for (auto str : m_db1TblTriggers)
            cout << str.first << ": " << str.second << endl;
        cout << endl
             << "db2 all the table triggers: " << endl;
        for (auto str : m_db2TblTriggers)
            cout << str.first << ": " << str.second << endl;
    }

    void DbCompare::testTableTriggers(string tableName)
    {
        cout << "db1::" << tableName << " table triggers: " << endl;
        cout << m_db1TblTriggers[tableName] << endl;
        cout << endl
             << "db2::" << tableName << " table triggers: " << endl;
        cout << m_db2TblTriggers[tableName] << endl;
    }

    void DbCompare::testCreateNewTbl()
    {
        string sql;
        // sql = createNewTbl("inv");
        sql = createNewTbl("countrySettingCfg");
        // sql = createNewTbl("accounts");
        // sql = createNewTbl("invCfg");
        print<vector<string>>("-> create table", {sql});
    }

    void DbCompare::initDbTables()
    {
        m_db1Tables = m_db1->getTables();
        m_db2Tables = m_db2->getTables();
    }

    void DbCompare::initDbTableSchema()
    {
        auto tblSchema1 = initTablesSchema(m_db1, m_db1Tables);
        m_db1TblSchema = std::move(tblSchema1);

        auto tblSchema2 = initTablesSchema(m_db2, m_db2Tables);
        m_db2TblSchema = std::move(tblSchema2);
    }

    void DbCompare::initDbTableIndices()
    {
        auto tableIndices1 = initTableIndices(m_db1, m_db1Tables);
        m_db1TblIndices = std::move(tableIndices1);

        auto tableIndices2 = initTableIndices(m_db2, m_db2Tables);
        m_db2TblIndices = std::move(tableIndices2);
    }

    void DbCompare::initDbTableTriggers()
    {
        auto tableTriggers1 = initTableTriggers(m_db1, m_db1Tables);
        m_db1TblTriggers = std::move(tableTriggers1);

        auto tableTriggers2 = initTableTriggers(m_db2, m_db2Tables);
        m_db2TblTriggers = std::move(tableTriggers2);
    }

    string DbCompare::createNewTbl(std::string tblName)
    {
        string sql = "";

        // db1: target, db2: reference
        auto targetCtCmd = m_db1->getCreateTblCmd(tblName);
        auto refCtCmd = m_db2->getCreateTblCmd(tblName);

        print<vector<string>>("-> targetCtCmd", {targetCtCmd});
        print<vector<string>>("-> refCtCmd", {refCtCmd});

        // columns and constraints
        auto targetColsCons = splitCreateTblCmd(targetCtCmd);
        auto refColsCons = splitCreateTblCmd(refCtCmd);

        print<vector<string>>("-> targetColsCons", targetColsCons);
        print<vector<string>>("-> refColsCons", refColsCons);

        auto diffColsCons = getColsConsDiff(targetColsCons, refColsCons);
        auto diffTargetColsCons = diffColsCons.first;
        auto diffRefColsCons = diffColsCons.second;
        auto sharedColsCons = getColsConsIntersect(targetColsCons, refColsCons);

        print("-> diffTargetColsCons", diffTargetColsCons);
        print("-> diffRefColsCons", diffRefColsCons);
        print("-> sharedColsCons", sharedColsCons);

        // TODO: update this part by paying attention to the value that have been read from json config file, 
        // for now it is considered as true
        bool keepColConst = true;

        stringstream ss_ct;
        string newTblName = tblName + "_tmp";
        ss_ct << "CREATE TABLE " << newTblName << " (";
        for (auto const &col : refColsCons)
            ss_ct << col << ", ";

        auto pairDiffTargetColsCons = getColsAndConstraints(diffTargetColsCons);
        auto umapDiffTargetCols = pairDiffTargetColsCons.first;
        auto diffTargetColNamesDetails = getColNamesDetails(umapDiffTargetCols);
        auto diffTargetColNames = diffTargetColNamesDetails.first;
        auto diffTargetColDetails = diffTargetColNamesDetails.second;
        auto diffTargetContraints = pairDiffTargetColsCons.second;

        // check for the columns which are in the target but not in ref
        int diffcols_size = diffTargetColNames.size();
        for (auto i = 0; i < diffcols_size; i++)
        {
            if (keepColConst)
                ss_ct << diffTargetColDetails[i] << ", ";
        }

        // check for the constraints which are in the target but not in ref
        int diffconst_size = diffTargetContraints.size();
        for (auto i = 0; i < diffconst_size; i++)
        {
            if(keepColConst)
                ss_ct << diffTargetContraints[i] << ", ";
        }

        ss_ct << ")";
        sql = ss_ct.str();
        auto pos = sql.find_last_of(",");
        sql.replace(pos, 2, "");
        
        // split columns and constraints 
        auto pairRefColsCons = getColsAndConstraints(refColsCons);
        auto umapRefCols = pairRefColsCons.first;
        auto refConstraints = pairRefColsCons.second;
        auto detailedRefCols = updateColNames(umapRefCols, SCHEMA_REF, tblName);
        detailedRefCols = updateRefTable(detailedRefCols, SCHEMA_REF);
        auto refColNames = getColNamesDetails(umapRefCols).first;
        auto detailedRefCons = updateColNameInConstraints(refConstraints, refColNames, SCHEMA_REF, tblName);
        auto detailedRefColsCons = mergeColsAndConstraint(detailedRefCols, detailedRefCons);
        
        print<vector<string>>("-> detailedRefColsCons", detailedRefColsCons); 
        
        auto detailedDiffTargetCols = updateColNames(umapDiffTargetCols, SCHEMA_MAIN, tblName);
        detailedDiffTargetCols = updateRefTable(detailedDiffTargetCols, SCHEMA_MAIN);
        auto detailedDiffTargetCons = updateColNameInConstraints(diffTargetContraints, diffTargetColNames, SCHEMA_MAIN, tblName);
        auto detailedDiffTargetColsCons = mergeColsAndConstraint(detailedDiffTargetCols, detailedDiffTargetCons);

        print<vector<string>>("-> detailedDiffTargetColsCons", detailedDiffTargetColsCons);


        return sql;
    }

} // namespace Kaco
