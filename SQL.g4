grammar SQL;

EqualOrAssign: '=';
Less: '<';
LessEqual: '<=';
Greater: '>';
GreaterEqual: '>=';
NotEqual: '<>';

Count: 'COUNT';
Average: 'AVG';
Max: 'MAX';
Min: 'MIN';
Sum: 'SUM';
Null: 'NULL';

Identifier: [a-zA-Z_] [a-zA-Z_0-9]*;
UnsignedInteger: [0-9]+;
String: '\'' ~['\r\n]* '\'';
Float
    : '-'? (
        [0-9]+ '.' [0-9]* ExponentPart?
        | '.' [0-9]+ ExponentPart?
        | [0-9]+ ExponentPart
    )
    ;
Minus: '-';
Whitespace: [ \t\n\r]+ -> skip;
LineComment: '--' ~[\r\n]* -> skip;

fragment ExponentPart: [eE] [+-]? [0-9]+;

program
    : statement* EOF
    ;

statement
    : db_statement ';'
    | table_statement ';'
    | alter_statement ';'
    | Null ';'
    ;

db_statement
    : 'CREATE' 'DATABASE' Identifier    # create_db
    | 'DROP' 'DATABASE' Identifier      # drop_db
    | 'SHOW' 'DATABASES'                # show_dbs
    | 'USE' Identifier                  # use_db                  
    | 'SHOW' 'TABLES'                   # show_tables
	| 'SHOW' 'INDEXES'					# show_indexes
    ;

table_statement
    : 'CREATE' 'TABLE' Identifier '(' field_list ')'                                                # create_table
    | 'DROP' 'TABLE' Identifier                                                                     # drop_table
    | 'DESC' Identifier                                                                             # describe_table
    | 'LOAD' 'DATA' 'INFILE' String 'INTO' 'TABLE' Identifier 'FIELDS' 'TERMINATED' 'BY' String     # load_table
    | 'INSERT' 'INTO' Identifier 'VALUES' value_lists                                               # insert_into_table
    | 'DELETE' 'FROM' Identifier ('WHERE' where_and_clause)?                                        # delete_from_table
    | 'UPDATE' Identifier 'SET' set_clause ('WHERE' where_and_clause)?                               # update_table
    | select_table                                                                                  # select_table_
    ;

select_table
    : 'SELECT' selectors 'FROM' identifiers where_part? group_by_part? order_by_part? limit_part?
    ;

where_part
    : 'WHERE' where_and_clause
    ;

group_by_part
    : 'GROUP' 'BY' column_list
    ;

order_by_part
    : 'ORDER' 'BY' order_item (',' order_item)*
    ;

order_item
    : column order?
    ;

limit_part
    : 'LIMIT' UnsignedInteger ('OFFSET' UnsignedInteger)?
    ;

alter_statement
    : 'ALTER' 'TABLE' Identifier 'ADD' 'INDEX' (Identifier)? '(' identifiers ')'   			                # alter_add_index
    | 'ALTER' 'TABLE' Identifier 'DROP' 'INDEX' Identifier                                                  # alter_drop_index
    | 'ALTER' 'TABLE' Identifier 'DROP' 'PRIMARY' 'KEY' (Identifier)?                                       # alter_table_drop_pk
    | 'ALTER' 'TABLE' Identifier 'DROP' 'FOREIGN' 'KEY' Identifier                                          # alter_table_drop_foreign_key
    | 'ALTER' 'TABLE' Identifier 'ADD' ('CONSTRAINT' (Identifier)?)? 'PRIMARY' 'KEY' '(' identifiers ')'    # alter_table_add_pk
    | 'ALTER' 'TABLE' Identifier 'ADD' ('CONSTRAINT' (Identifier)?)? 'FOREIGN' 'KEY' '(' identifiers ')' 'REFERENCES' Identifier '(' identifiers ')'  # alter_table_add_foreign_key
    | 'ALTER' 'TABLE' Identifier 'ADD' 'UNIQUE' (Identifier)? '(' identifiers ')'                           # alter_table_add_unique
    ;

field_list
    : field (',' field)*
    ;

field
    : Identifier type_ ('NOT' Null)? ('DEFAULT' value)?                                               # normal_field
    | 'PRIMARY' 'KEY' (Identifier)? '(' identifiers ')'                                               # primary_key_field
    | 'FOREIGN' 'KEY' (Identifier)? '(' identifiers ')' 'REFERENCES' Identifier '(' identifiers ')'   # foreign_key_field
    | 'UNIQUE' (Identifier)? '(' identifiers ')'                                                      # unique_field
    ;

type_
    : 'INT'
    | ('VARCHAR' | 'CHAR') '(' UnsignedInteger ')'
    | 'FLOAT'
    | 'DOUBLE'
    | 'DATE'
    ;

order
    : 'ASC'
    | 'DESC'
    ;

value_lists
    : value_list (',' value_list)*
    ;

value_list
    : '(' value (',' value)* ')'
    ;

value
    : signed_integer
    | String
    | Float
    | Null
    ;

signed_integer
    : Minus? UnsignedInteger
    ;

where_and_clause
    : where_clause ('AND' where_clause)*
    ;

where_clause
    : column operator_ expression            # where_operator_expression
    | value operator_ column                 # where_value_operator_column
    | column operator_ '(' select_table ')'  # where_operator_select
    | column 'IS' ('NOT')? Null              # where_null
    | column 'IN' value_list                 # where_in_list
    | column 'IN' '(' select_table ')'       # where_in_select
    | column 'LIKE' String                   # where_like_string
    ;

column
    : (Identifier '.')? Identifier
    ;

expression
    : value
    | column
    ;

set_clause
    : assignment (',' assignment)*
    ;

assignment
    : column EqualOrAssign value
    ;

selectors
    : '*'
    | selector (',' selector)*
    ;

selector
    : column
    | aggregator '(' column ')'
    | Count '(' '*' ')'
    ;

identifiers
    : Identifier (',' Identifier)*
    ;

column_list
    : column (',' column)*
    ;

operator_
    : EqualOrAssign
    | Less
    | LessEqual
    | Greater
    | GreaterEqual
    | NotEqual
    ;


aggregator
    : Count
    | Average
    | Max
    | Min
    | Sum
    ;
