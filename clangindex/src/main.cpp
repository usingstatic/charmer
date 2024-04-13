#include <iostream>
#include <fstream>

#include <sqlite3.h>
#include <clang-c/Index.h>
#include <iostream>
#include <vector>
#include <string>
#include "functioninfo.pb.h"

// Visitor function prototype
CXChildVisitResult visitFunctionNode(CXCursor cursor, CXCursor parent, CXClientData client_data);

std::vector<FunctionInfo> selectClangFunctions(const char* path) {
    std::vector<FunctionInfo> functions;

    // Create index using clang_createIndex
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit;

    // Parse the input file and create a translation unit
    CXErrorCode error = clang_parseTranslationUnit2(
        index,
        path,
        nullptr, 0,  // Command line args for the file
        nullptr, 0,  // Unsaved files
        CXTranslationUnit_None,
        &unit
    );

    if (error != CXError_Success) {
        std::cerr << "Failed to parse translation unit." << std::endl;
        return functions;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, visitFunctionNode, &functions);

    // Cleanup
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return functions;
}

CXChildVisitResult visitFunctionNode(CXCursor cursor, CXCursor parent, CXClientData client_data) {
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
        CXString name = clang_getCursorSpelling(cursor);
        CXType type = clang_getCursorType(cursor);

        CXString signature = clang_getTypeSpelling(type);
        CXType resultType = clang_getResultType(type);
        CXString returnType = clang_getTypeSpelling(resultType);

        bool isStatic = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
        bool isMethod = kind == CXCursor_CXXMethod;

        std::vector<FunctionInfo>* functions = static_cast<std::vector<FunctionInfo>*>(client_data);
        FunctionInfo fn;
        fn.set_name(clang_getCString(name));
        fn.set_signature(clang_getCString(signature));
        fn.set_return_type(clang_getCString(returnType));
        fn.set_args_count(clang_Cursor_getNumArguments(cursor));
        fn.set_is_method(isMethod);
        fn.set_is_static(isStatic);
        functions->push_back(fn);

        clang_disposeString(name);
        clang_disposeString(signature);
        clang_disposeString(returnType);
    }

    return CXChildVisit_Continue;
}

class SQLite {
    private:
        sqlite3 *db;    // Use unique_ptr etc.
    public:
        SQLite()
        {
            db = nullptr;
        }

        int init(const char *db_name)
        {
            if (sqlite3_open(db_name, &db) != SQLITE_OK) {
                char* errMsg = nullptr;
                std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_free(errMsg);
                return 1;
            }
        }

        ~SQLite()
        {
            if (db != nullptr) {
                std::cerr << "Closing handle to DB" << std::endl;
                sqlite3_close(db);
            }
        }
        // Delete copy constructor and copy assignment operator to prevent copying.
        SQLite(const SQLite&) = delete;
        SQLite& operator=(const SQLite&) = delete;

        sqlite3* get() const { return db; }
};

int initDb(SQLite& db, const char *schema)
{
    // Read SQL from file
    std::ifstream sqlFile(schema);
    std::string sql((std::istreambuf_iterator<char>(sqlFile)), std::istreambuf_iterator<char>());

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db.get(), sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return 1;
    }
    return 0;
}

// TODO: Autogenerate, never use unquoted values. This is unsafe and vulnerable to any SQL shenanigans. Will use a structured insert properly. For PoC purposes only
std::string insertCommand(const FunctionInfo& fn)
{
    std::ostringstream oss;
    oss << "INSERT INTO `t_FunctionInfo` (`1`, `2`, `3`, `4`, `5`, `6`) VALUES ("
        <<  "\"" << fn.name() << "\""
        << ",\"" << fn.signature() << "\""
        << "," << std::dec << fn.args_count() << ""
        << ",\"" << fn.return_type() << "\""
        << "," << (fn.is_method() ? "TRUE" : "FALSE") << ""
        << "," << (fn.is_static() ? "TRUE" : "FALSE") << ""
        << ")"
        << ";";

    return oss.str();
}

int main(int argc, char** argv) {
    if (argc <= 2) {
        std::cerr << "Usage: " << argv[0] << " <SQL_script_file> <source_file>" << std::endl;
        return 1;
    }
    SQLite db;
    db.init("example.db");
    if (initDb(db, argv[1]) != 0) {
        return 1;
    }
    std::cout << "SQL execution successful." << std::endl;

    auto functions = selectClangFunctions(argv[2]);
    std::ostringstream oss;
    for (const FunctionInfo& fn : functions) {
        oss << insertCommand(fn) << std::endl;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db.get(), oss.str().c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return 1;
    }
    return 0;
}
