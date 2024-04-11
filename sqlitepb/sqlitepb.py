#!/usr/bin/env python3
import sys
from google.protobuf.compiler import plugin_pb2 as plugin_pb2
from google.protobuf import text_format
import os.path

SQLITE_FILE_EXTENSION = ".sqlite"
SQLITE_FILE_HEADER_CONTENT = '''/**
 * Autogenerated by <TODO THIS TOOL NAME> from {proto_file}
 * Date: {date}
 **/

'''

def protobuf_type_to_sqlite(type_enum):
    type_mapping = {
        # Maps Protocol Buffers type to SQLite type
        1: 'DOUBLE',    # TYPE_DOUBLE to SQLite REAL
        2: 'FLOAT',     # TYPE_FLOAT to SQLite REAL
        3: 'INTEGER',   # TYPE_INT64 to SQLite INTEGER
        4: 'INTEGER',   # TYPE_UINT64 to SQLite INTEGER
        5: 'INTEGER',   # TYPE_INT32 to SQLite INTEGER
        6: 'INTEGER',   # TYPE_FIXED64 to SQLite INTEGER
        7: 'INTEGER',   # TYPE_FIXED32 to SQLite INTEGER
        8: 'INTEGER',   # TYPE_BOOL to SQLite INTEGER
        9: 'TEXT',      # TYPE_STRING to SQLite TEXT
        10: 'BLOB',     # TYPE_GROUP (deprecated) not directly mappable, using BLOB as a placeholder
        11: 'BLOB',     # TYPE_MESSAGE (nested messages) treated as BLOB
        12: 'BLOB',     # TYPE_BYTES to SQLite BLOB
        13: 'INTEGER',  # TYPE_UINT32 to SQLite INTEGER
        14: 'TEXT',     # TYPE_ENUM to SQLite TEXT
        15: 'INTEGER',  # TYPE_SFIXED32 to SQLite INTEGER
        16: 'INTEGER',  # TYPE_SFIXED64 to SQLite INTEGER
        17: 'INTEGER',  # TYPE_SINT32 to SQLite INTEGER
        18: 'INTEGER'   # TYPE_SINT64 to SQLite INTEGER
    }
    return type_mapping.get(type_enum, 'BLOB')  # Default to BLOB if unknown

def process_request(request, response, args):

    for proto_file in request.proto_file:
        response_file = response.file.add()
        response_file.name = os.path.splitext(proto_file.name)[0] + SQLITE_FILE_EXTENSION
        response_file.content = SQLITE_FILE_HEADER_CONTENT
        for message_type in proto_file.message_type:
            table_name = message_type.name
            field_statements = []
            view_columns = []
            view_fields = []
            for field in message_type.field:
                sql_type = protobuf_type_to_sqlite(field.type)
                field_statements.append(f"  `{field.number}` {sql_type}")
                view_columns.append(f"`{field.number}` AS `{field.name}`")
                view_fields.append(f"`{field.name}`")

            # Generate CREATE TABLE statement
            table_statement = f"CREATE TABLE `t_{table_name}` (\n" + ",\n".join(field_statements) + "\n);\n\n"
            response_file.content += table_statement

            # Generate CREATE VIEW statement
            view_statement = f"CREATE VIEW `{table_name}` AS SELECT\n" + ",\n".join(view_columns) + f"\nFROM `t_{table_name}`;\n\n"
            response_file.content += view_statement

###
# Invocation from command line
###
import shlex
import io
import sys

def check_python_version(min_major, min_minor):
    if sys.version_info < (min_major, min_minor):
        raise SystemExit(f"Python {min_major}.{min_minor} or higher is required. Detected Python {sys.version_info.major}.{sys.version_info.minor}.")

# Invocation from protoc plugin
def main_plugin():
    data = io.open(sys.stdin.fileno(), "rb").read()

    request = plugin_pb2.CodeGeneratorRequest.FromString(data)
    params = request.parameter

    # Protoc separates options passed to plugins by comma
    # This allows also giving --<plugin>_opt option multiple times.
    lex = shlex.shlex(params)
    lex.whitespace_split = True
    lex.whitespace = ','
    lex.commenters = ''
    args = list(lex)

    # TODO: Parse parameters

    # Initialize the response to protoc
    response = plugin_pb2.CodeGeneratorResponse()
    if hasattr(plugin_pb2.CodeGeneratorResponse, "FEATURE_PROTO3_OPTIONAL"):
        response.supported_features = plugin_pb2.CodeGeneratorResponse.FEATURE_PROTO3_OPTIONAL

    process_request(request, response, args)

    # Respond to protoc
    io.open(sys.stdout.fileno(), "wb").write(response.SerializeToString())


def main():
    check_python_version(3, 8)
    main_plugin()

if __name__ == '__main__':
    main()
