# Charmer
Charmer is a C/C++ code indexer based on CLang AST using SQLite so it is fully query-able with standard SQL.
The indexed code (should) reference Git object instead of standard file:line so it can work on top of GitQl.

*** Extremely initial, not working!! ***

The idea is based on this wonderful project https://github.com/AmrDeveloper/ClangQL

# Design

The code here is built of several layers:

a. sqlitepb/ - Protoc plugin that generates SQLite schema commands from .proto files.
b. proto/ - A folder containing the schema for the data structures used throughout the project.
c. clangindex/ - Tool to index C/C++ source code into SQLite.
d. charmer - Rule based code structure/code style

## Db Schema
The database schema reflects the Protobuf structure. Source tables with the prefix t_ (e.g. t_FunctionInfo) use the field id as column name and a view is constructed with the human readable names (e.g. FunctionInfo).
This allows both faster insert and resilience to renaming fields.

# Build

*Step 1: Generate the table schemas:*
```
protoc --plugin=protoc-gen-sqlite=./sqlitepb/protoc-gen-sqlite --sqlite_out=generated --proto_path=./proto ./proto/functioninfo.proto
```

*Step 2: Build the indexer:*
```
cd clangindex/build
cmake ../src
make
```

*Step 3: Run the tool on a source file*
```
./SQLiteExample ../../generated/functioninfo.sqlite <some C/C++ file>
```

This should create the database (called example.db in the current run folder), initialize the tables, scan the source file and create INSERT commands for each function. Didn't execute the insert yet

*Step 4: Query the database:*

After the database has been written to file , query it like:
```
sqlite3 example.db
SQLite version 3.43.2 2023-10-10 13:08:14
Enter ".help" for usage hints.

sqlite> .headers on
sqlite> .mode column
sqlite> SELECT * FROM FunctionInfo;
```
