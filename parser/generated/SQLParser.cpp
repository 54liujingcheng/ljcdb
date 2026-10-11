
// Generated from SQL.g4 by ANTLR 4.13.2


#include "SQLVisitor.h"

#include "SQLParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct SQLParserStaticData final {
  SQLParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SQLParserStaticData(const SQLParserStaticData&) = delete;
  SQLParserStaticData(SQLParserStaticData&&) = delete;
  SQLParserStaticData& operator=(const SQLParserStaticData&) = delete;
  SQLParserStaticData& operator=(SQLParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag sqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<SQLParserStaticData> sqlParserStaticData = nullptr;

void sqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (sqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(sqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<SQLParserStaticData>(
    std::vector<std::string>{
      "program", "statement", "db_statement", "table_statement", "select_table", 
      "where_part", "group_by_part", "order_by_part", "order_item", "limit_part", 
      "alter_statement", "field_list", "field", "type_", "order", "value_lists", 
      "value_list", "value", "signed_integer", "where_and_clause", "where_clause", 
      "column", "expression", "set_clause", "assignment", "selectors", "selector", 
      "identifiers", "column_list", "operator_", "aggregator"
    },
    std::vector<std::string>{
      "", "';'", "'CREATE'", "'DATABASE'", "'DROP'", "'SHOW'", "'DATABASES'", 
      "'USE'", "'TABLES'", "'INDEXES'", "'TABLE'", "'('", "')'", "'DESC'", 
      "'LOAD'", "'DATA'", "'INFILE'", "'INTO'", "'FIELDS'", "'TERMINATED'", 
      "'BY'", "'INSERT'", "'VALUES'", "'DELETE'", "'FROM'", "'WHERE'", "'UPDATE'", 
      "'SET'", "'SELECT'", "'GROUP'", "'ORDER'", "','", "'LIMIT'", "'OFFSET'", 
      "'ALTER'", "'ADD'", "'INDEX'", "'PRIMARY'", "'KEY'", "'FOREIGN'", 
      "'CONSTRAINT'", "'REFERENCES'", "'UNIQUE'", "'NOT'", "'DEFAULT'", 
      "'INT'", "'VARCHAR'", "'CHAR'", "'FLOAT'", "'DOUBLE'", "'DATE'", "'ASC'", 
      "'AND'", "'IS'", "'IN'", "'LIKE'", "'.'", "'*'", "'='", "'<'", "'<='", 
      "'>'", "'>='", "'<>'", "'COUNT'", "'AVG'", "'MAX'", "'MIN'", "'SUM'", 
      "'NULL'", "", "", "", "", "'-'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "EqualOrAssign", "Less", "LessEqual", 
      "Greater", "GreaterEqual", "NotEqual", "Count", "Average", "Max", 
      "Min", "Sum", "Null", "Identifier", "UnsignedInteger", "String", "Float", 
      "Minus", "Whitespace", "LineComment"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,76,477,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,1,0,5,0,64,8,0,10,0,12,0,67,9,0,1,0,1,0,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,82,8,1,1,2,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,98,8,2,1,3,1,3,1,3,1,3,1,3,1,
  	3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
  	1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,133,8,3,1,3,1,3,1,3,1,3,1,
  	3,1,3,3,3,141,8,3,1,3,3,3,144,8,3,1,4,1,4,1,4,1,4,1,4,3,4,151,8,4,1,4,
  	3,4,154,8,4,1,4,3,4,157,8,4,1,4,3,4,160,8,4,1,5,1,5,1,5,1,6,1,6,1,6,1,
  	6,1,7,1,7,1,7,1,7,1,7,5,7,174,8,7,10,7,12,7,177,9,7,1,8,1,8,3,8,181,8,
  	8,1,9,1,9,1,9,1,9,3,9,187,8,9,1,10,1,10,1,10,1,10,1,10,1,10,3,10,195,
  	8,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,
  	1,10,1,10,1,10,1,10,3,10,214,8,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,
  	1,10,1,10,1,10,1,10,1,10,1,10,3,10,229,8,10,3,10,231,8,10,1,10,1,10,1,
  	10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,3,10,245,8,10,3,10,247,
  	8,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,
  	1,10,1,10,1,10,1,10,3,10,266,8,10,1,10,1,10,1,10,1,10,3,10,272,8,10,1,
  	11,1,11,1,11,5,11,277,8,11,10,11,12,11,280,9,11,1,12,1,12,1,12,1,12,3,
  	12,286,8,12,1,12,1,12,3,12,290,8,12,1,12,1,12,1,12,3,12,295,8,12,1,12,
  	1,12,1,12,1,12,1,12,1,12,1,12,3,12,304,8,12,1,12,1,12,1,12,1,12,1,12,
  	1,12,1,12,1,12,1,12,1,12,1,12,3,12,317,8,12,1,12,1,12,1,12,1,12,3,12,
  	323,8,12,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,3,13,333,8,13,1,14,1,
  	14,1,15,1,15,1,15,5,15,340,8,15,10,15,12,15,343,9,15,1,16,1,16,1,16,1,
  	16,5,16,349,8,16,10,16,12,16,352,9,16,1,16,1,16,1,17,1,17,1,17,1,17,3,
  	17,360,8,17,1,18,3,18,363,8,18,1,18,1,18,1,19,1,19,1,19,5,19,370,8,19,
  	10,19,12,19,373,9,19,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,1,20,3,20,392,8,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,3,20,410,
  	8,20,1,21,1,21,3,21,414,8,21,1,21,1,21,1,22,1,22,3,22,420,8,22,1,23,1,
  	23,1,23,5,23,425,8,23,10,23,12,23,428,9,23,1,24,1,24,1,24,1,24,1,25,1,
  	25,1,25,1,25,5,25,438,8,25,10,25,12,25,441,9,25,3,25,443,8,25,1,26,1,
  	26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,455,8,26,1,27,1,27,1,
  	27,5,27,460,8,27,10,27,12,27,463,9,27,1,28,1,28,1,28,5,28,468,8,28,10,
  	28,12,28,471,9,28,1,29,1,29,1,30,1,30,1,30,0,0,31,0,2,4,6,8,10,12,14,
  	16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,
  	0,4,1,0,46,47,2,0,13,13,51,51,1,0,58,63,1,0,64,68,519,0,65,1,0,0,0,2,
  	81,1,0,0,0,4,97,1,0,0,0,6,143,1,0,0,0,8,145,1,0,0,0,10,161,1,0,0,0,12,
  	164,1,0,0,0,14,168,1,0,0,0,16,178,1,0,0,0,18,182,1,0,0,0,20,271,1,0,0,
  	0,22,273,1,0,0,0,24,322,1,0,0,0,26,332,1,0,0,0,28,334,1,0,0,0,30,336,
  	1,0,0,0,32,344,1,0,0,0,34,359,1,0,0,0,36,362,1,0,0,0,38,366,1,0,0,0,40,
  	409,1,0,0,0,42,413,1,0,0,0,44,419,1,0,0,0,46,421,1,0,0,0,48,429,1,0,0,
  	0,50,442,1,0,0,0,52,454,1,0,0,0,54,456,1,0,0,0,56,464,1,0,0,0,58,472,
  	1,0,0,0,60,474,1,0,0,0,62,64,3,2,1,0,63,62,1,0,0,0,64,67,1,0,0,0,65,63,
  	1,0,0,0,65,66,1,0,0,0,66,68,1,0,0,0,67,65,1,0,0,0,68,69,5,0,0,1,69,1,
  	1,0,0,0,70,71,3,4,2,0,71,72,5,1,0,0,72,82,1,0,0,0,73,74,3,6,3,0,74,75,
  	5,1,0,0,75,82,1,0,0,0,76,77,3,20,10,0,77,78,5,1,0,0,78,82,1,0,0,0,79,
  	80,5,69,0,0,80,82,5,1,0,0,81,70,1,0,0,0,81,73,1,0,0,0,81,76,1,0,0,0,81,
  	79,1,0,0,0,82,3,1,0,0,0,83,84,5,2,0,0,84,85,5,3,0,0,85,98,5,70,0,0,86,
  	87,5,4,0,0,87,88,5,3,0,0,88,98,5,70,0,0,89,90,5,5,0,0,90,98,5,6,0,0,91,
  	92,5,7,0,0,92,98,5,70,0,0,93,94,5,5,0,0,94,98,5,8,0,0,95,96,5,5,0,0,96,
  	98,5,9,0,0,97,83,1,0,0,0,97,86,1,0,0,0,97,89,1,0,0,0,97,91,1,0,0,0,97,
  	93,1,0,0,0,97,95,1,0,0,0,98,5,1,0,0,0,99,100,5,2,0,0,100,101,5,10,0,0,
  	101,102,5,70,0,0,102,103,5,11,0,0,103,104,3,22,11,0,104,105,5,12,0,0,
  	105,144,1,0,0,0,106,107,5,4,0,0,107,108,5,10,0,0,108,144,5,70,0,0,109,
  	110,5,13,0,0,110,144,5,70,0,0,111,112,5,14,0,0,112,113,5,15,0,0,113,114,
  	5,16,0,0,114,115,5,72,0,0,115,116,5,17,0,0,116,117,5,10,0,0,117,118,5,
  	70,0,0,118,119,5,18,0,0,119,120,5,19,0,0,120,121,5,20,0,0,121,144,5,72,
  	0,0,122,123,5,21,0,0,123,124,5,17,0,0,124,125,5,70,0,0,125,126,5,22,0,
  	0,126,144,3,30,15,0,127,128,5,23,0,0,128,129,5,24,0,0,129,132,5,70,0,
  	0,130,131,5,25,0,0,131,133,3,38,19,0,132,130,1,0,0,0,132,133,1,0,0,0,
  	133,144,1,0,0,0,134,135,5,26,0,0,135,136,5,70,0,0,136,137,5,27,0,0,137,
  	140,3,46,23,0,138,139,5,25,0,0,139,141,3,38,19,0,140,138,1,0,0,0,140,
  	141,1,0,0,0,141,144,1,0,0,0,142,144,3,8,4,0,143,99,1,0,0,0,143,106,1,
  	0,0,0,143,109,1,0,0,0,143,111,1,0,0,0,143,122,1,0,0,0,143,127,1,0,0,0,
  	143,134,1,0,0,0,143,142,1,0,0,0,144,7,1,0,0,0,145,146,5,28,0,0,146,147,
  	3,50,25,0,147,148,5,24,0,0,148,150,3,54,27,0,149,151,3,10,5,0,150,149,
  	1,0,0,0,150,151,1,0,0,0,151,153,1,0,0,0,152,154,3,12,6,0,153,152,1,0,
  	0,0,153,154,1,0,0,0,154,156,1,0,0,0,155,157,3,14,7,0,156,155,1,0,0,0,
  	156,157,1,0,0,0,157,159,1,0,0,0,158,160,3,18,9,0,159,158,1,0,0,0,159,
  	160,1,0,0,0,160,9,1,0,0,0,161,162,5,25,0,0,162,163,3,38,19,0,163,11,1,
  	0,0,0,164,165,5,29,0,0,165,166,5,20,0,0,166,167,3,56,28,0,167,13,1,0,
  	0,0,168,169,5,30,0,0,169,170,5,20,0,0,170,175,3,16,8,0,171,172,5,31,0,
  	0,172,174,3,16,8,0,173,171,1,0,0,0,174,177,1,0,0,0,175,173,1,0,0,0,175,
  	176,1,0,0,0,176,15,1,0,0,0,177,175,1,0,0,0,178,180,3,42,21,0,179,181,
  	3,28,14,0,180,179,1,0,0,0,180,181,1,0,0,0,181,17,1,0,0,0,182,183,5,32,
  	0,0,183,186,5,71,0,0,184,185,5,33,0,0,185,187,5,71,0,0,186,184,1,0,0,
  	0,186,187,1,0,0,0,187,19,1,0,0,0,188,189,5,34,0,0,189,190,5,10,0,0,190,
  	191,5,70,0,0,191,192,5,35,0,0,192,194,5,36,0,0,193,195,5,70,0,0,194,193,
  	1,0,0,0,194,195,1,0,0,0,195,196,1,0,0,0,196,197,5,11,0,0,197,198,3,54,
  	27,0,198,199,5,12,0,0,199,272,1,0,0,0,200,201,5,34,0,0,201,202,5,10,0,
  	0,202,203,5,70,0,0,203,204,5,4,0,0,204,205,5,36,0,0,205,272,5,70,0,0,
  	206,207,5,34,0,0,207,208,5,10,0,0,208,209,5,70,0,0,209,210,5,4,0,0,210,
  	211,5,37,0,0,211,213,5,38,0,0,212,214,5,70,0,0,213,212,1,0,0,0,213,214,
  	1,0,0,0,214,272,1,0,0,0,215,216,5,34,0,0,216,217,5,10,0,0,217,218,5,70,
  	0,0,218,219,5,4,0,0,219,220,5,39,0,0,220,221,5,38,0,0,221,272,5,70,0,
  	0,222,223,5,34,0,0,223,224,5,10,0,0,224,225,5,70,0,0,225,230,5,35,0,0,
  	226,228,5,40,0,0,227,229,5,70,0,0,228,227,1,0,0,0,228,229,1,0,0,0,229,
  	231,1,0,0,0,230,226,1,0,0,0,230,231,1,0,0,0,231,232,1,0,0,0,232,233,5,
  	37,0,0,233,234,5,38,0,0,234,235,5,11,0,0,235,236,3,54,27,0,236,237,5,
  	12,0,0,237,272,1,0,0,0,238,239,5,34,0,0,239,240,5,10,0,0,240,241,5,70,
  	0,0,241,246,5,35,0,0,242,244,5,40,0,0,243,245,5,70,0,0,244,243,1,0,0,
  	0,244,245,1,0,0,0,245,247,1,0,0,0,246,242,1,0,0,0,246,247,1,0,0,0,247,
  	248,1,0,0,0,248,249,5,39,0,0,249,250,5,38,0,0,250,251,5,11,0,0,251,252,
  	3,54,27,0,252,253,5,12,0,0,253,254,5,41,0,0,254,255,5,70,0,0,255,256,
  	5,11,0,0,256,257,3,54,27,0,257,258,5,12,0,0,258,272,1,0,0,0,259,260,5,
  	34,0,0,260,261,5,10,0,0,261,262,5,70,0,0,262,263,5,35,0,0,263,265,5,42,
  	0,0,264,266,5,70,0,0,265,264,1,0,0,0,265,266,1,0,0,0,266,267,1,0,0,0,
  	267,268,5,11,0,0,268,269,3,54,27,0,269,270,5,12,0,0,270,272,1,0,0,0,271,
  	188,1,0,0,0,271,200,1,0,0,0,271,206,1,0,0,0,271,215,1,0,0,0,271,222,1,
  	0,0,0,271,238,1,0,0,0,271,259,1,0,0,0,272,21,1,0,0,0,273,278,3,24,12,
  	0,274,275,5,31,0,0,275,277,3,24,12,0,276,274,1,0,0,0,277,280,1,0,0,0,
  	278,276,1,0,0,0,278,279,1,0,0,0,279,23,1,0,0,0,280,278,1,0,0,0,281,282,
  	5,70,0,0,282,285,3,26,13,0,283,284,5,43,0,0,284,286,5,69,0,0,285,283,
  	1,0,0,0,285,286,1,0,0,0,286,289,1,0,0,0,287,288,5,44,0,0,288,290,3,34,
  	17,0,289,287,1,0,0,0,289,290,1,0,0,0,290,323,1,0,0,0,291,292,5,37,0,0,
  	292,294,5,38,0,0,293,295,5,70,0,0,294,293,1,0,0,0,294,295,1,0,0,0,295,
  	296,1,0,0,0,296,297,5,11,0,0,297,298,3,54,27,0,298,299,5,12,0,0,299,323,
  	1,0,0,0,300,301,5,39,0,0,301,303,5,38,0,0,302,304,5,70,0,0,303,302,1,
  	0,0,0,303,304,1,0,0,0,304,305,1,0,0,0,305,306,5,11,0,0,306,307,3,54,27,
  	0,307,308,5,12,0,0,308,309,5,41,0,0,309,310,5,70,0,0,310,311,5,11,0,0,
  	311,312,3,54,27,0,312,313,5,12,0,0,313,323,1,0,0,0,314,316,5,42,0,0,315,
  	317,5,70,0,0,316,315,1,0,0,0,316,317,1,0,0,0,317,318,1,0,0,0,318,319,
  	5,11,0,0,319,320,3,54,27,0,320,321,5,12,0,0,321,323,1,0,0,0,322,281,1,
  	0,0,0,322,291,1,0,0,0,322,300,1,0,0,0,322,314,1,0,0,0,323,25,1,0,0,0,
  	324,333,5,45,0,0,325,326,7,0,0,0,326,327,5,11,0,0,327,328,5,71,0,0,328,
  	333,5,12,0,0,329,333,5,48,0,0,330,333,5,49,0,0,331,333,5,50,0,0,332,324,
  	1,0,0,0,332,325,1,0,0,0,332,329,1,0,0,0,332,330,1,0,0,0,332,331,1,0,0,
  	0,333,27,1,0,0,0,334,335,7,1,0,0,335,29,1,0,0,0,336,341,3,32,16,0,337,
  	338,5,31,0,0,338,340,3,32,16,0,339,337,1,0,0,0,340,343,1,0,0,0,341,339,
  	1,0,0,0,341,342,1,0,0,0,342,31,1,0,0,0,343,341,1,0,0,0,344,345,5,11,0,
  	0,345,350,3,34,17,0,346,347,5,31,0,0,347,349,3,34,17,0,348,346,1,0,0,
  	0,349,352,1,0,0,0,350,348,1,0,0,0,350,351,1,0,0,0,351,353,1,0,0,0,352,
  	350,1,0,0,0,353,354,5,12,0,0,354,33,1,0,0,0,355,360,3,36,18,0,356,360,
  	5,72,0,0,357,360,5,73,0,0,358,360,5,69,0,0,359,355,1,0,0,0,359,356,1,
  	0,0,0,359,357,1,0,0,0,359,358,1,0,0,0,360,35,1,0,0,0,361,363,5,74,0,0,
  	362,361,1,0,0,0,362,363,1,0,0,0,363,364,1,0,0,0,364,365,5,71,0,0,365,
  	37,1,0,0,0,366,371,3,40,20,0,367,368,5,52,0,0,368,370,3,40,20,0,369,367,
  	1,0,0,0,370,373,1,0,0,0,371,369,1,0,0,0,371,372,1,0,0,0,372,39,1,0,0,
  	0,373,371,1,0,0,0,374,375,3,42,21,0,375,376,3,58,29,0,376,377,3,44,22,
  	0,377,410,1,0,0,0,378,379,3,34,17,0,379,380,3,58,29,0,380,381,3,42,21,
  	0,381,410,1,0,0,0,382,383,3,42,21,0,383,384,3,58,29,0,384,385,5,11,0,
  	0,385,386,3,8,4,0,386,387,5,12,0,0,387,410,1,0,0,0,388,389,3,42,21,0,
  	389,391,5,53,0,0,390,392,5,43,0,0,391,390,1,0,0,0,391,392,1,0,0,0,392,
  	393,1,0,0,0,393,394,5,69,0,0,394,410,1,0,0,0,395,396,3,42,21,0,396,397,
  	5,54,0,0,397,398,3,32,16,0,398,410,1,0,0,0,399,400,3,42,21,0,400,401,
  	5,54,0,0,401,402,5,11,0,0,402,403,3,8,4,0,403,404,5,12,0,0,404,410,1,
  	0,0,0,405,406,3,42,21,0,406,407,5,55,0,0,407,408,5,72,0,0,408,410,1,0,
  	0,0,409,374,1,0,0,0,409,378,1,0,0,0,409,382,1,0,0,0,409,388,1,0,0,0,409,
  	395,1,0,0,0,409,399,1,0,0,0,409,405,1,0,0,0,410,41,1,0,0,0,411,412,5,
  	70,0,0,412,414,5,56,0,0,413,411,1,0,0,0,413,414,1,0,0,0,414,415,1,0,0,
  	0,415,416,5,70,0,0,416,43,1,0,0,0,417,420,3,34,17,0,418,420,3,42,21,0,
  	419,417,1,0,0,0,419,418,1,0,0,0,420,45,1,0,0,0,421,426,3,48,24,0,422,
  	423,5,31,0,0,423,425,3,48,24,0,424,422,1,0,0,0,425,428,1,0,0,0,426,424,
  	1,0,0,0,426,427,1,0,0,0,427,47,1,0,0,0,428,426,1,0,0,0,429,430,3,42,21,
  	0,430,431,5,58,0,0,431,432,3,34,17,0,432,49,1,0,0,0,433,443,5,57,0,0,
  	434,439,3,52,26,0,435,436,5,31,0,0,436,438,3,52,26,0,437,435,1,0,0,0,
  	438,441,1,0,0,0,439,437,1,0,0,0,439,440,1,0,0,0,440,443,1,0,0,0,441,439,
  	1,0,0,0,442,433,1,0,0,0,442,434,1,0,0,0,443,51,1,0,0,0,444,455,3,42,21,
  	0,445,446,3,60,30,0,446,447,5,11,0,0,447,448,3,42,21,0,448,449,5,12,0,
  	0,449,455,1,0,0,0,450,451,5,64,0,0,451,452,5,11,0,0,452,453,5,57,0,0,
  	453,455,5,12,0,0,454,444,1,0,0,0,454,445,1,0,0,0,454,450,1,0,0,0,455,
  	53,1,0,0,0,456,461,5,70,0,0,457,458,5,31,0,0,458,460,5,70,0,0,459,457,
  	1,0,0,0,460,463,1,0,0,0,461,459,1,0,0,0,461,462,1,0,0,0,462,55,1,0,0,
  	0,463,461,1,0,0,0,464,469,3,42,21,0,465,466,5,31,0,0,466,468,3,42,21,
  	0,467,465,1,0,0,0,468,471,1,0,0,0,469,467,1,0,0,0,469,470,1,0,0,0,470,
  	57,1,0,0,0,471,469,1,0,0,0,472,473,7,2,0,0,473,59,1,0,0,0,474,475,7,3,
  	0,0,475,61,1,0,0,0,44,65,81,97,132,140,143,150,153,156,159,175,180,186,
  	194,213,228,230,244,246,265,271,278,285,289,294,303,316,322,332,341,350,
  	359,362,371,391,409,413,419,426,439,442,454,461,469
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sqlParserStaticData = std::move(staticData);
}

}

SQLParser::SQLParser(TokenStream *input) : SQLParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SQLParser::SQLParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SQLParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sqlParserStaticData->atn, sqlParserStaticData->decisionToDFA, sqlParserStaticData->sharedContextCache, options);
}

SQLParser::~SQLParser() {
  delete _interpreter;
}

const atn::ATN& SQLParser::getATN() const {
  return *sqlParserStaticData->atn;
}

std::string SQLParser::getGrammarFileName() const {
  return "SQL.g4";
}

const std::vector<std::string>& SQLParser::getRuleNames() const {
  return sqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& SQLParser::getVocabulary() const {
  return sqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SQLParser::getSerializedATN() const {
  return sqlParserStaticData->serializedATN;
}


//----------------- ProgramContext ------------------------------------------------------------------

SQLParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SQLParser::ProgramContext::EOF() {
  return getToken(SQLParser::EOF, 0);
}

std::vector<SQLParser::StatementContext *> SQLParser::ProgramContext::statement() {
  return getRuleContexts<SQLParser::StatementContext>();
}

SQLParser::StatementContext* SQLParser::ProgramContext::statement(size_t i) {
  return getRuleContext<SQLParser::StatementContext>(i);
}


size_t SQLParser::ProgramContext::getRuleIndex() const {
  return SQLParser::RuleProgram;
}


std::any SQLParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::ProgramContext* SQLParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, SQLParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(65);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 17525924020) != 0) || _la == SQLParser::Null) {
      setState(62);
      statement();
      setState(67);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(68);
    match(SQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

SQLParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::Db_statementContext* SQLParser::StatementContext::db_statement() {
  return getRuleContext<SQLParser::Db_statementContext>(0);
}

SQLParser::Table_statementContext* SQLParser::StatementContext::table_statement() {
  return getRuleContext<SQLParser::Table_statementContext>(0);
}

SQLParser::Alter_statementContext* SQLParser::StatementContext::alter_statement() {
  return getRuleContext<SQLParser::Alter_statementContext>(0);
}

tree::TerminalNode* SQLParser::StatementContext::Null() {
  return getToken(SQLParser::Null, 0);
}


size_t SQLParser::StatementContext::getRuleIndex() const {
  return SQLParser::RuleStatement;
}


std::any SQLParser::StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitStatement(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::StatementContext* SQLParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 2, SQLParser::RuleStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(81);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(70);
      db_statement();
      setState(71);
      match(SQLParser::T__0);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(73);
      table_statement();
      setState(74);
      match(SQLParser::T__0);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(76);
      alter_statement();
      setState(77);
      match(SQLParser::T__0);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(79);
      match(SQLParser::Null);
      setState(80);
      match(SQLParser::T__0);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Db_statementContext ------------------------------------------------------------------

SQLParser::Db_statementContext::Db_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::Db_statementContext::getRuleIndex() const {
  return SQLParser::RuleDb_statement;
}

void SQLParser::Db_statementContext::copyFrom(Db_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Show_dbsContext ------------------------------------------------------------------

SQLParser::Show_dbsContext::Show_dbsContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Show_dbsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitShow_dbs(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Drop_dbContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Drop_dbContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Drop_dbContext::Drop_dbContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Drop_dbContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitDrop_db(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Show_tablesContext ------------------------------------------------------------------

SQLParser::Show_tablesContext::Show_tablesContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Show_tablesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitShow_tables(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Create_dbContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Create_dbContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Create_dbContext::Create_dbContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Create_dbContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitCreate_db(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Use_dbContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Use_dbContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Use_dbContext::Use_dbContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Use_dbContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitUse_db(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Show_indexesContext ------------------------------------------------------------------

SQLParser::Show_indexesContext::Show_indexesContext(Db_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Show_indexesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitShow_indexes(this);
  else
    return visitor->visitChildren(this);
}
SQLParser::Db_statementContext* SQLParser::db_statement() {
  Db_statementContext *_localctx = _tracker.createInstance<Db_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, SQLParser::RuleDb_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(97);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SQLParser::Create_dbContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(83);
      match(SQLParser::T__1);
      setState(84);
      match(SQLParser::T__2);
      setState(85);
      match(SQLParser::Identifier);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SQLParser::Drop_dbContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(86);
      match(SQLParser::T__3);
      setState(87);
      match(SQLParser::T__2);
      setState(88);
      match(SQLParser::Identifier);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SQLParser::Show_dbsContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(89);
      match(SQLParser::T__4);
      setState(90);
      match(SQLParser::T__5);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SQLParser::Use_dbContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(91);
      match(SQLParser::T__6);
      setState(92);
      match(SQLParser::Identifier);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SQLParser::Show_tablesContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(93);
      match(SQLParser::T__4);
      setState(94);
      match(SQLParser::T__7);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SQLParser::Show_indexesContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(95);
      match(SQLParser::T__4);
      setState(96);
      match(SQLParser::T__8);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_statementContext ------------------------------------------------------------------

SQLParser::Table_statementContext::Table_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::Table_statementContext::getRuleIndex() const {
  return SQLParser::RuleTable_statement;
}

void SQLParser::Table_statementContext::copyFrom(Table_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Load_tableContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Load_tableContext::String() {
  return getTokens(SQLParser::String);
}

tree::TerminalNode* SQLParser::Load_tableContext::String(size_t i) {
  return getToken(SQLParser::String, i);
}

tree::TerminalNode* SQLParser::Load_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Load_tableContext::Load_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Load_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitLoad_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Delete_from_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Delete_from_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Where_and_clauseContext* SQLParser::Delete_from_tableContext::where_and_clause() {
  return getRuleContext<SQLParser::Where_and_clauseContext>(0);
}

SQLParser::Delete_from_tableContext::Delete_from_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Delete_from_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitDelete_from_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Insert_into_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Insert_into_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Value_listsContext* SQLParser::Insert_into_tableContext::value_lists() {
  return getRuleContext<SQLParser::Value_listsContext>(0);
}

SQLParser::Insert_into_tableContext::Insert_into_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Insert_into_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitInsert_into_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Create_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Create_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Field_listContext* SQLParser::Create_tableContext::field_list() {
  return getRuleContext<SQLParser::Field_listContext>(0);
}

SQLParser::Create_tableContext::Create_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Create_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitCreate_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Describe_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Describe_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Describe_tableContext::Describe_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Describe_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitDescribe_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Select_table_Context ------------------------------------------------------------------

SQLParser::Select_tableContext* SQLParser::Select_table_Context::select_table() {
  return getRuleContext<SQLParser::Select_tableContext>(0);
}

SQLParser::Select_table_Context::Select_table_Context(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Select_table_Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSelect_table_(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Drop_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Drop_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Drop_tableContext::Drop_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Drop_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitDrop_table(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Update_tableContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Update_tableContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Set_clauseContext* SQLParser::Update_tableContext::set_clause() {
  return getRuleContext<SQLParser::Set_clauseContext>(0);
}

SQLParser::Where_and_clauseContext* SQLParser::Update_tableContext::where_and_clause() {
  return getRuleContext<SQLParser::Where_and_clauseContext>(0);
}

SQLParser::Update_tableContext::Update_tableContext(Table_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Update_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitUpdate_table(this);
  else
    return visitor->visitChildren(this);
}
SQLParser::Table_statementContext* SQLParser::table_statement() {
  Table_statementContext *_localctx = _tracker.createInstance<Table_statementContext>(_ctx, getState());
  enterRule(_localctx, 6, SQLParser::RuleTable_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(143);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::T__1: {
        _localctx = _tracker.createInstance<SQLParser::Create_tableContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(99);
        match(SQLParser::T__1);
        setState(100);
        match(SQLParser::T__9);
        setState(101);
        match(SQLParser::Identifier);
        setState(102);
        match(SQLParser::T__10);
        setState(103);
        field_list();
        setState(104);
        match(SQLParser::T__11);
        break;
      }

      case SQLParser::T__3: {
        _localctx = _tracker.createInstance<SQLParser::Drop_tableContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(106);
        match(SQLParser::T__3);
        setState(107);
        match(SQLParser::T__9);
        setState(108);
        match(SQLParser::Identifier);
        break;
      }

      case SQLParser::T__12: {
        _localctx = _tracker.createInstance<SQLParser::Describe_tableContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(109);
        match(SQLParser::T__12);
        setState(110);
        match(SQLParser::Identifier);
        break;
      }

      case SQLParser::T__13: {
        _localctx = _tracker.createInstance<SQLParser::Load_tableContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(111);
        match(SQLParser::T__13);
        setState(112);
        match(SQLParser::T__14);
        setState(113);
        match(SQLParser::T__15);
        setState(114);
        match(SQLParser::String);
        setState(115);
        match(SQLParser::T__16);
        setState(116);
        match(SQLParser::T__9);
        setState(117);
        match(SQLParser::Identifier);
        setState(118);
        match(SQLParser::T__17);
        setState(119);
        match(SQLParser::T__18);
        setState(120);
        match(SQLParser::T__19);
        setState(121);
        match(SQLParser::String);
        break;
      }

      case SQLParser::T__20: {
        _localctx = _tracker.createInstance<SQLParser::Insert_into_tableContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(122);
        match(SQLParser::T__20);
        setState(123);
        match(SQLParser::T__16);
        setState(124);
        match(SQLParser::Identifier);
        setState(125);
        match(SQLParser::T__21);
        setState(126);
        value_lists();
        break;
      }

      case SQLParser::T__22: {
        _localctx = _tracker.createInstance<SQLParser::Delete_from_tableContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(127);
        match(SQLParser::T__22);
        setState(128);
        match(SQLParser::T__23);
        setState(129);
        match(SQLParser::Identifier);
        setState(132);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::T__24) {
          setState(130);
          match(SQLParser::T__24);
          setState(131);
          where_and_clause();
        }
        break;
      }

      case SQLParser::T__25: {
        _localctx = _tracker.createInstance<SQLParser::Update_tableContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(134);
        match(SQLParser::T__25);
        setState(135);
        match(SQLParser::Identifier);
        setState(136);
        match(SQLParser::T__26);
        setState(137);
        set_clause();
        setState(140);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::T__24) {
          setState(138);
          match(SQLParser::T__24);
          setState(139);
          where_and_clause();
        }
        break;
      }

      case SQLParser::T__27: {
        _localctx = _tracker.createInstance<SQLParser::Select_table_Context>(_localctx);
        enterOuterAlt(_localctx, 8);
        setState(142);
        select_table();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_tableContext ------------------------------------------------------------------

SQLParser::Select_tableContext::Select_tableContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::SelectorsContext* SQLParser::Select_tableContext::selectors() {
  return getRuleContext<SQLParser::SelectorsContext>(0);
}

SQLParser::IdentifiersContext* SQLParser::Select_tableContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

SQLParser::Where_partContext* SQLParser::Select_tableContext::where_part() {
  return getRuleContext<SQLParser::Where_partContext>(0);
}

SQLParser::Group_by_partContext* SQLParser::Select_tableContext::group_by_part() {
  return getRuleContext<SQLParser::Group_by_partContext>(0);
}

SQLParser::Order_by_partContext* SQLParser::Select_tableContext::order_by_part() {
  return getRuleContext<SQLParser::Order_by_partContext>(0);
}

SQLParser::Limit_partContext* SQLParser::Select_tableContext::limit_part() {
  return getRuleContext<SQLParser::Limit_partContext>(0);
}


size_t SQLParser::Select_tableContext::getRuleIndex() const {
  return SQLParser::RuleSelect_table;
}


std::any SQLParser::Select_tableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSelect_table(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Select_tableContext* SQLParser::select_table() {
  Select_tableContext *_localctx = _tracker.createInstance<Select_tableContext>(_ctx, getState());
  enterRule(_localctx, 8, SQLParser::RuleSelect_table);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(145);
    match(SQLParser::T__27);
    setState(146);
    selectors();
    setState(147);
    match(SQLParser::T__23);
    setState(148);
    identifiers();
    setState(150);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__24) {
      setState(149);
      where_part();
    }
    setState(153);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__28) {
      setState(152);
      group_by_part();
    }
    setState(156);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__29) {
      setState(155);
      order_by_part();
    }
    setState(159);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__31) {
      setState(158);
      limit_part();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Where_partContext ------------------------------------------------------------------

SQLParser::Where_partContext::Where_partContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::Where_and_clauseContext* SQLParser::Where_partContext::where_and_clause() {
  return getRuleContext<SQLParser::Where_and_clauseContext>(0);
}


size_t SQLParser::Where_partContext::getRuleIndex() const {
  return SQLParser::RuleWhere_part;
}


std::any SQLParser::Where_partContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_part(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Where_partContext* SQLParser::where_part() {
  Where_partContext *_localctx = _tracker.createInstance<Where_partContext>(_ctx, getState());
  enterRule(_localctx, 10, SQLParser::RuleWhere_part);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(161);
    match(SQLParser::T__24);
    setState(162);
    where_and_clause();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Group_by_partContext ------------------------------------------------------------------

SQLParser::Group_by_partContext::Group_by_partContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::Column_listContext* SQLParser::Group_by_partContext::column_list() {
  return getRuleContext<SQLParser::Column_listContext>(0);
}


size_t SQLParser::Group_by_partContext::getRuleIndex() const {
  return SQLParser::RuleGroup_by_part;
}


std::any SQLParser::Group_by_partContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitGroup_by_part(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Group_by_partContext* SQLParser::group_by_part() {
  Group_by_partContext *_localctx = _tracker.createInstance<Group_by_partContext>(_ctx, getState());
  enterRule(_localctx, 12, SQLParser::RuleGroup_by_part);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(164);
    match(SQLParser::T__28);
    setState(165);
    match(SQLParser::T__19);
    setState(166);
    column_list();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Order_by_partContext ------------------------------------------------------------------

SQLParser::Order_by_partContext::Order_by_partContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::Order_itemContext *> SQLParser::Order_by_partContext::order_item() {
  return getRuleContexts<SQLParser::Order_itemContext>();
}

SQLParser::Order_itemContext* SQLParser::Order_by_partContext::order_item(size_t i) {
  return getRuleContext<SQLParser::Order_itemContext>(i);
}


size_t SQLParser::Order_by_partContext::getRuleIndex() const {
  return SQLParser::RuleOrder_by_part;
}


std::any SQLParser::Order_by_partContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitOrder_by_part(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Order_by_partContext* SQLParser::order_by_part() {
  Order_by_partContext *_localctx = _tracker.createInstance<Order_by_partContext>(_ctx, getState());
  enterRule(_localctx, 14, SQLParser::RuleOrder_by_part);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(168);
    match(SQLParser::T__29);
    setState(169);
    match(SQLParser::T__19);
    setState(170);
    order_item();
    setState(175);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(171);
      match(SQLParser::T__30);
      setState(172);
      order_item();
      setState(177);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Order_itemContext ------------------------------------------------------------------

SQLParser::Order_itemContext::Order_itemContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::ColumnContext* SQLParser::Order_itemContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::OrderContext* SQLParser::Order_itemContext::order() {
  return getRuleContext<SQLParser::OrderContext>(0);
}


size_t SQLParser::Order_itemContext::getRuleIndex() const {
  return SQLParser::RuleOrder_item;
}


std::any SQLParser::Order_itemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitOrder_item(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Order_itemContext* SQLParser::order_item() {
  Order_itemContext *_localctx = _tracker.createInstance<Order_itemContext>(_ctx, getState());
  enterRule(_localctx, 16, SQLParser::RuleOrder_item);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(178);
    column();
    setState(180);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__12

    || _la == SQLParser::T__50) {
      setState(179);
      order();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Limit_partContext ------------------------------------------------------------------

SQLParser::Limit_partContext::Limit_partContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> SQLParser::Limit_partContext::UnsignedInteger() {
  return getTokens(SQLParser::UnsignedInteger);
}

tree::TerminalNode* SQLParser::Limit_partContext::UnsignedInteger(size_t i) {
  return getToken(SQLParser::UnsignedInteger, i);
}


size_t SQLParser::Limit_partContext::getRuleIndex() const {
  return SQLParser::RuleLimit_part;
}


std::any SQLParser::Limit_partContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitLimit_part(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Limit_partContext* SQLParser::limit_part() {
  Limit_partContext *_localctx = _tracker.createInstance<Limit_partContext>(_ctx, getState());
  enterRule(_localctx, 18, SQLParser::RuleLimit_part);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(182);
    match(SQLParser::T__31);
    setState(183);
    match(SQLParser::UnsignedInteger);
    setState(186);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::T__32) {
      setState(184);
      match(SQLParser::T__32);
      setState(185);
      match(SQLParser::UnsignedInteger);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Alter_statementContext ------------------------------------------------------------------

SQLParser::Alter_statementContext::Alter_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::Alter_statementContext::getRuleIndex() const {
  return SQLParser::RuleAlter_statement;
}

void SQLParser::Alter_statementContext::copyFrom(Alter_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Alter_table_drop_pkContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_table_drop_pkContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_table_drop_pkContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::Alter_table_drop_pkContext::Alter_table_drop_pkContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_table_drop_pkContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_table_drop_pk(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_table_add_foreign_keyContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_table_add_foreign_keyContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_table_add_foreign_keyContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

std::vector<SQLParser::IdentifiersContext *> SQLParser::Alter_table_add_foreign_keyContext::identifiers() {
  return getRuleContexts<SQLParser::IdentifiersContext>();
}

SQLParser::IdentifiersContext* SQLParser::Alter_table_add_foreign_keyContext::identifiers(size_t i) {
  return getRuleContext<SQLParser::IdentifiersContext>(i);
}

SQLParser::Alter_table_add_foreign_keyContext::Alter_table_add_foreign_keyContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_table_add_foreign_keyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_table_add_foreign_key(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_table_add_uniqueContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_table_add_uniqueContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_table_add_uniqueContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::IdentifiersContext* SQLParser::Alter_table_add_uniqueContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

SQLParser::Alter_table_add_uniqueContext::Alter_table_add_uniqueContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_table_add_uniqueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_table_add_unique(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_drop_indexContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_drop_indexContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_drop_indexContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::Alter_drop_indexContext::Alter_drop_indexContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_drop_indexContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_drop_index(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_add_indexContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_add_indexContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_add_indexContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::IdentifiersContext* SQLParser::Alter_add_indexContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

SQLParser::Alter_add_indexContext::Alter_add_indexContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_add_indexContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_add_index(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_table_drop_foreign_keyContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_table_drop_foreign_keyContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_table_drop_foreign_keyContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::Alter_table_drop_foreign_keyContext::Alter_table_drop_foreign_keyContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_table_drop_foreign_keyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_table_drop_foreign_key(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Alter_table_add_pkContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> SQLParser::Alter_table_add_pkContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Alter_table_add_pkContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::IdentifiersContext* SQLParser::Alter_table_add_pkContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

SQLParser::Alter_table_add_pkContext::Alter_table_add_pkContext(Alter_statementContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Alter_table_add_pkContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAlter_table_add_pk(this);
  else
    return visitor->visitChildren(this);
}
SQLParser::Alter_statementContext* SQLParser::alter_statement() {
  Alter_statementContext *_localctx = _tracker.createInstance<Alter_statementContext>(_ctx, getState());
  enterRule(_localctx, 20, SQLParser::RuleAlter_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(271);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SQLParser::Alter_add_indexContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(188);
      match(SQLParser::T__33);
      setState(189);
      match(SQLParser::T__9);
      setState(190);
      match(SQLParser::Identifier);
      setState(191);
      match(SQLParser::T__34);
      setState(192);
      match(SQLParser::T__35);
      setState(194);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::Identifier) {
        setState(193);
        match(SQLParser::Identifier);
      }
      setState(196);
      match(SQLParser::T__10);
      setState(197);
      identifiers();
      setState(198);
      match(SQLParser::T__11);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SQLParser::Alter_drop_indexContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(200);
      match(SQLParser::T__33);
      setState(201);
      match(SQLParser::T__9);
      setState(202);
      match(SQLParser::Identifier);
      setState(203);
      match(SQLParser::T__3);
      setState(204);
      match(SQLParser::T__35);
      setState(205);
      match(SQLParser::Identifier);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SQLParser::Alter_table_drop_pkContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(206);
      match(SQLParser::T__33);
      setState(207);
      match(SQLParser::T__9);
      setState(208);
      match(SQLParser::Identifier);
      setState(209);
      match(SQLParser::T__3);
      setState(210);
      match(SQLParser::T__36);
      setState(211);
      match(SQLParser::T__37);
      setState(213);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::Identifier) {
        setState(212);
        match(SQLParser::Identifier);
      }
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SQLParser::Alter_table_drop_foreign_keyContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(215);
      match(SQLParser::T__33);
      setState(216);
      match(SQLParser::T__9);
      setState(217);
      match(SQLParser::Identifier);
      setState(218);
      match(SQLParser::T__3);
      setState(219);
      match(SQLParser::T__38);
      setState(220);
      match(SQLParser::T__37);
      setState(221);
      match(SQLParser::Identifier);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SQLParser::Alter_table_add_pkContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(222);
      match(SQLParser::T__33);
      setState(223);
      match(SQLParser::T__9);
      setState(224);
      match(SQLParser::Identifier);
      setState(225);
      match(SQLParser::T__34);
      setState(230);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::T__39) {
        setState(226);
        match(SQLParser::T__39);
        setState(228);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::Identifier) {
          setState(227);
          match(SQLParser::Identifier);
        }
      }
      setState(232);
      match(SQLParser::T__36);
      setState(233);
      match(SQLParser::T__37);
      setState(234);
      match(SQLParser::T__10);
      setState(235);
      identifiers();
      setState(236);
      match(SQLParser::T__11);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SQLParser::Alter_table_add_foreign_keyContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(238);
      match(SQLParser::T__33);
      setState(239);
      match(SQLParser::T__9);
      setState(240);
      match(SQLParser::Identifier);
      setState(241);
      match(SQLParser::T__34);
      setState(246);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::T__39) {
        setState(242);
        match(SQLParser::T__39);
        setState(244);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::Identifier) {
          setState(243);
          match(SQLParser::Identifier);
        }
      }
      setState(248);
      match(SQLParser::T__38);
      setState(249);
      match(SQLParser::T__37);
      setState(250);
      match(SQLParser::T__10);
      setState(251);
      identifiers();
      setState(252);
      match(SQLParser::T__11);
      setState(253);
      match(SQLParser::T__40);
      setState(254);
      match(SQLParser::Identifier);
      setState(255);
      match(SQLParser::T__10);
      setState(256);
      identifiers();
      setState(257);
      match(SQLParser::T__11);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<SQLParser::Alter_table_add_uniqueContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(259);
      match(SQLParser::T__33);
      setState(260);
      match(SQLParser::T__9);
      setState(261);
      match(SQLParser::Identifier);
      setState(262);
      match(SQLParser::T__34);
      setState(263);
      match(SQLParser::T__41);
      setState(265);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::Identifier) {
        setState(264);
        match(SQLParser::Identifier);
      }
      setState(267);
      match(SQLParser::T__10);
      setState(268);
      identifiers();
      setState(269);
      match(SQLParser::T__11);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_listContext ------------------------------------------------------------------

SQLParser::Field_listContext::Field_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::FieldContext *> SQLParser::Field_listContext::field() {
  return getRuleContexts<SQLParser::FieldContext>();
}

SQLParser::FieldContext* SQLParser::Field_listContext::field(size_t i) {
  return getRuleContext<SQLParser::FieldContext>(i);
}


size_t SQLParser::Field_listContext::getRuleIndex() const {
  return SQLParser::RuleField_list;
}


std::any SQLParser::Field_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitField_list(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Field_listContext* SQLParser::field_list() {
  Field_listContext *_localctx = _tracker.createInstance<Field_listContext>(_ctx, getState());
  enterRule(_localctx, 22, SQLParser::RuleField_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(273);
    field();
    setState(278);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(274);
      match(SQLParser::T__30);
      setState(275);
      field();
      setState(280);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FieldContext ------------------------------------------------------------------

SQLParser::FieldContext::FieldContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::FieldContext::getRuleIndex() const {
  return SQLParser::RuleField;
}

void SQLParser::FieldContext::copyFrom(FieldContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Primary_key_fieldContext ------------------------------------------------------------------

SQLParser::IdentifiersContext* SQLParser::Primary_key_fieldContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

tree::TerminalNode* SQLParser::Primary_key_fieldContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Primary_key_fieldContext::Primary_key_fieldContext(FieldContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Primary_key_fieldContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitPrimary_key_field(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Foreign_key_fieldContext ------------------------------------------------------------------

std::vector<SQLParser::IdentifiersContext *> SQLParser::Foreign_key_fieldContext::identifiers() {
  return getRuleContexts<SQLParser::IdentifiersContext>();
}

SQLParser::IdentifiersContext* SQLParser::Foreign_key_fieldContext::identifiers(size_t i) {
  return getRuleContext<SQLParser::IdentifiersContext>(i);
}

std::vector<tree::TerminalNode *> SQLParser::Foreign_key_fieldContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::Foreign_key_fieldContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}

SQLParser::Foreign_key_fieldContext::Foreign_key_fieldContext(FieldContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Foreign_key_fieldContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitForeign_key_field(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Normal_fieldContext ------------------------------------------------------------------

tree::TerminalNode* SQLParser::Normal_fieldContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Type_Context* SQLParser::Normal_fieldContext::type_() {
  return getRuleContext<SQLParser::Type_Context>(0);
}

tree::TerminalNode* SQLParser::Normal_fieldContext::Null() {
  return getToken(SQLParser::Null, 0);
}

SQLParser::ValueContext* SQLParser::Normal_fieldContext::value() {
  return getRuleContext<SQLParser::ValueContext>(0);
}

SQLParser::Normal_fieldContext::Normal_fieldContext(FieldContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Normal_fieldContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitNormal_field(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Unique_fieldContext ------------------------------------------------------------------

SQLParser::IdentifiersContext* SQLParser::Unique_fieldContext::identifiers() {
  return getRuleContext<SQLParser::IdentifiersContext>(0);
}

tree::TerminalNode* SQLParser::Unique_fieldContext::Identifier() {
  return getToken(SQLParser::Identifier, 0);
}

SQLParser::Unique_fieldContext::Unique_fieldContext(FieldContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Unique_fieldContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitUnique_field(this);
  else
    return visitor->visitChildren(this);
}
SQLParser::FieldContext* SQLParser::field() {
  FieldContext *_localctx = _tracker.createInstance<FieldContext>(_ctx, getState());
  enterRule(_localctx, 24, SQLParser::RuleField);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(322);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::Identifier: {
        _localctx = _tracker.createInstance<SQLParser::Normal_fieldContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(281);
        match(SQLParser::Identifier);
        setState(282);
        type_();
        setState(285);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::T__42) {
          setState(283);
          match(SQLParser::T__42);
          setState(284);
          match(SQLParser::Null);
        }
        setState(289);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::T__43) {
          setState(287);
          match(SQLParser::T__43);
          setState(288);
          value();
        }
        break;
      }

      case SQLParser::T__36: {
        _localctx = _tracker.createInstance<SQLParser::Primary_key_fieldContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(291);
        match(SQLParser::T__36);
        setState(292);
        match(SQLParser::T__37);
        setState(294);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::Identifier) {
          setState(293);
          match(SQLParser::Identifier);
        }
        setState(296);
        match(SQLParser::T__10);
        setState(297);
        identifiers();
        setState(298);
        match(SQLParser::T__11);
        break;
      }

      case SQLParser::T__38: {
        _localctx = _tracker.createInstance<SQLParser::Foreign_key_fieldContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(300);
        match(SQLParser::T__38);
        setState(301);
        match(SQLParser::T__37);
        setState(303);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::Identifier) {
          setState(302);
          match(SQLParser::Identifier);
        }
        setState(305);
        match(SQLParser::T__10);
        setState(306);
        identifiers();
        setState(307);
        match(SQLParser::T__11);
        setState(308);
        match(SQLParser::T__40);
        setState(309);
        match(SQLParser::Identifier);
        setState(310);
        match(SQLParser::T__10);
        setState(311);
        identifiers();
        setState(312);
        match(SQLParser::T__11);
        break;
      }

      case SQLParser::T__41: {
        _localctx = _tracker.createInstance<SQLParser::Unique_fieldContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(314);
        match(SQLParser::T__41);
        setState(316);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SQLParser::Identifier) {
          setState(315);
          match(SQLParser::Identifier);
        }
        setState(318);
        match(SQLParser::T__10);
        setState(319);
        identifiers();
        setState(320);
        match(SQLParser::T__11);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Type_Context ------------------------------------------------------------------

SQLParser::Type_Context::Type_Context(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SQLParser::Type_Context::UnsignedInteger() {
  return getToken(SQLParser::UnsignedInteger, 0);
}


size_t SQLParser::Type_Context::getRuleIndex() const {
  return SQLParser::RuleType_;
}


std::any SQLParser::Type_Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitType_(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Type_Context* SQLParser::type_() {
  Type_Context *_localctx = _tracker.createInstance<Type_Context>(_ctx, getState());
  enterRule(_localctx, 26, SQLParser::RuleType_);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(332);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::T__44: {
        enterOuterAlt(_localctx, 1);
        setState(324);
        match(SQLParser::T__44);
        break;
      }

      case SQLParser::T__45:
      case SQLParser::T__46: {
        enterOuterAlt(_localctx, 2);
        setState(325);
        _la = _input->LA(1);
        if (!(_la == SQLParser::T__45

        || _la == SQLParser::T__46)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(326);
        match(SQLParser::T__10);
        setState(327);
        match(SQLParser::UnsignedInteger);
        setState(328);
        match(SQLParser::T__11);
        break;
      }

      case SQLParser::T__47: {
        enterOuterAlt(_localctx, 3);
        setState(329);
        match(SQLParser::T__47);
        break;
      }

      case SQLParser::T__48: {
        enterOuterAlt(_localctx, 4);
        setState(330);
        match(SQLParser::T__48);
        break;
      }

      case SQLParser::T__49: {
        enterOuterAlt(_localctx, 5);
        setState(331);
        match(SQLParser::T__49);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OrderContext ------------------------------------------------------------------

SQLParser::OrderContext::OrderContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::OrderContext::getRuleIndex() const {
  return SQLParser::RuleOrder;
}


std::any SQLParser::OrderContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitOrder(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::OrderContext* SQLParser::order() {
  OrderContext *_localctx = _tracker.createInstance<OrderContext>(_ctx, getState());
  enterRule(_localctx, 28, SQLParser::RuleOrder);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(334);
    _la = _input->LA(1);
    if (!(_la == SQLParser::T__12

    || _la == SQLParser::T__50)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Value_listsContext ------------------------------------------------------------------

SQLParser::Value_listsContext::Value_listsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::Value_listContext *> SQLParser::Value_listsContext::value_list() {
  return getRuleContexts<SQLParser::Value_listContext>();
}

SQLParser::Value_listContext* SQLParser::Value_listsContext::value_list(size_t i) {
  return getRuleContext<SQLParser::Value_listContext>(i);
}


size_t SQLParser::Value_listsContext::getRuleIndex() const {
  return SQLParser::RuleValue_lists;
}


std::any SQLParser::Value_listsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitValue_lists(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Value_listsContext* SQLParser::value_lists() {
  Value_listsContext *_localctx = _tracker.createInstance<Value_listsContext>(_ctx, getState());
  enterRule(_localctx, 30, SQLParser::RuleValue_lists);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(336);
    value_list();
    setState(341);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(337);
      match(SQLParser::T__30);
      setState(338);
      value_list();
      setState(343);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Value_listContext ------------------------------------------------------------------

SQLParser::Value_listContext::Value_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::ValueContext *> SQLParser::Value_listContext::value() {
  return getRuleContexts<SQLParser::ValueContext>();
}

SQLParser::ValueContext* SQLParser::Value_listContext::value(size_t i) {
  return getRuleContext<SQLParser::ValueContext>(i);
}


size_t SQLParser::Value_listContext::getRuleIndex() const {
  return SQLParser::RuleValue_list;
}


std::any SQLParser::Value_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitValue_list(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Value_listContext* SQLParser::value_list() {
  Value_listContext *_localctx = _tracker.createInstance<Value_listContext>(_ctx, getState());
  enterRule(_localctx, 32, SQLParser::RuleValue_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(344);
    match(SQLParser::T__10);
    setState(345);
    value();
    setState(350);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(346);
      match(SQLParser::T__30);
      setState(347);
      value();
      setState(352);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(353);
    match(SQLParser::T__11);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueContext ------------------------------------------------------------------

SQLParser::ValueContext::ValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::Signed_integerContext* SQLParser::ValueContext::signed_integer() {
  return getRuleContext<SQLParser::Signed_integerContext>(0);
}

tree::TerminalNode* SQLParser::ValueContext::String() {
  return getToken(SQLParser::String, 0);
}

tree::TerminalNode* SQLParser::ValueContext::Float() {
  return getToken(SQLParser::Float, 0);
}

tree::TerminalNode* SQLParser::ValueContext::Null() {
  return getToken(SQLParser::Null, 0);
}


size_t SQLParser::ValueContext::getRuleIndex() const {
  return SQLParser::RuleValue;
}


std::any SQLParser::ValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitValue(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::ValueContext* SQLParser::value() {
  ValueContext *_localctx = _tracker.createInstance<ValueContext>(_ctx, getState());
  enterRule(_localctx, 34, SQLParser::RuleValue);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(359);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::UnsignedInteger:
      case SQLParser::Minus: {
        enterOuterAlt(_localctx, 1);
        setState(355);
        signed_integer();
        break;
      }

      case SQLParser::String: {
        enterOuterAlt(_localctx, 2);
        setState(356);
        match(SQLParser::String);
        break;
      }

      case SQLParser::Float: {
        enterOuterAlt(_localctx, 3);
        setState(357);
        match(SQLParser::Float);
        break;
      }

      case SQLParser::Null: {
        enterOuterAlt(_localctx, 4);
        setState(358);
        match(SQLParser::Null);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_integerContext ------------------------------------------------------------------

SQLParser::Signed_integerContext::Signed_integerContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SQLParser::Signed_integerContext::UnsignedInteger() {
  return getToken(SQLParser::UnsignedInteger, 0);
}

tree::TerminalNode* SQLParser::Signed_integerContext::Minus() {
  return getToken(SQLParser::Minus, 0);
}


size_t SQLParser::Signed_integerContext::getRuleIndex() const {
  return SQLParser::RuleSigned_integer;
}


std::any SQLParser::Signed_integerContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSigned_integer(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Signed_integerContext* SQLParser::signed_integer() {
  Signed_integerContext *_localctx = _tracker.createInstance<Signed_integerContext>(_ctx, getState());
  enterRule(_localctx, 36, SQLParser::RuleSigned_integer);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(362);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SQLParser::Minus) {
      setState(361);
      match(SQLParser::Minus);
    }
    setState(364);
    match(SQLParser::UnsignedInteger);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Where_and_clauseContext ------------------------------------------------------------------

SQLParser::Where_and_clauseContext::Where_and_clauseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::Where_clauseContext *> SQLParser::Where_and_clauseContext::where_clause() {
  return getRuleContexts<SQLParser::Where_clauseContext>();
}

SQLParser::Where_clauseContext* SQLParser::Where_and_clauseContext::where_clause(size_t i) {
  return getRuleContext<SQLParser::Where_clauseContext>(i);
}


size_t SQLParser::Where_and_clauseContext::getRuleIndex() const {
  return SQLParser::RuleWhere_and_clause;
}


std::any SQLParser::Where_and_clauseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_and_clause(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Where_and_clauseContext* SQLParser::where_and_clause() {
  Where_and_clauseContext *_localctx = _tracker.createInstance<Where_and_clauseContext>(_ctx, getState());
  enterRule(_localctx, 38, SQLParser::RuleWhere_and_clause);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(366);
    where_clause();
    setState(371);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__51) {
      setState(367);
      match(SQLParser::T__51);
      setState(368);
      where_clause();
      setState(373);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Where_clauseContext ------------------------------------------------------------------

SQLParser::Where_clauseContext::Where_clauseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SQLParser::Where_clauseContext::getRuleIndex() const {
  return SQLParser::RuleWhere_clause;
}

void SQLParser::Where_clauseContext::copyFrom(Where_clauseContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- Where_value_operator_columnContext ------------------------------------------------------------------

SQLParser::ValueContext* SQLParser::Where_value_operator_columnContext::value() {
  return getRuleContext<SQLParser::ValueContext>(0);
}

SQLParser::Operator_Context* SQLParser::Where_value_operator_columnContext::operator_() {
  return getRuleContext<SQLParser::Operator_Context>(0);
}

SQLParser::ColumnContext* SQLParser::Where_value_operator_columnContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::Where_value_operator_columnContext::Where_value_operator_columnContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_value_operator_columnContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_value_operator_column(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_in_listContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_in_listContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::Value_listContext* SQLParser::Where_in_listContext::value_list() {
  return getRuleContext<SQLParser::Value_listContext>(0);
}

SQLParser::Where_in_listContext::Where_in_listContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_in_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_in_list(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_operator_selectContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_operator_selectContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::Operator_Context* SQLParser::Where_operator_selectContext::operator_() {
  return getRuleContext<SQLParser::Operator_Context>(0);
}

SQLParser::Select_tableContext* SQLParser::Where_operator_selectContext::select_table() {
  return getRuleContext<SQLParser::Select_tableContext>(0);
}

SQLParser::Where_operator_selectContext::Where_operator_selectContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_operator_selectContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_operator_select(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_nullContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_nullContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

tree::TerminalNode* SQLParser::Where_nullContext::Null() {
  return getToken(SQLParser::Null, 0);
}

SQLParser::Where_nullContext::Where_nullContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_nullContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_null(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_operator_expressionContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_operator_expressionContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::Operator_Context* SQLParser::Where_operator_expressionContext::operator_() {
  return getRuleContext<SQLParser::Operator_Context>(0);
}

SQLParser::ExpressionContext* SQLParser::Where_operator_expressionContext::expression() {
  return getRuleContext<SQLParser::ExpressionContext>(0);
}

SQLParser::Where_operator_expressionContext::Where_operator_expressionContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_operator_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_operator_expression(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_in_selectContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_in_selectContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::Select_tableContext* SQLParser::Where_in_selectContext::select_table() {
  return getRuleContext<SQLParser::Select_tableContext>(0);
}

SQLParser::Where_in_selectContext::Where_in_selectContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_in_selectContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_in_select(this);
  else
    return visitor->visitChildren(this);
}
//----------------- Where_like_stringContext ------------------------------------------------------------------

SQLParser::ColumnContext* SQLParser::Where_like_stringContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

tree::TerminalNode* SQLParser::Where_like_stringContext::String() {
  return getToken(SQLParser::String, 0);
}

SQLParser::Where_like_stringContext::Where_like_stringContext(Where_clauseContext *ctx) { copyFrom(ctx); }


std::any SQLParser::Where_like_stringContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitWhere_like_string(this);
  else
    return visitor->visitChildren(this);
}
SQLParser::Where_clauseContext* SQLParser::where_clause() {
  Where_clauseContext *_localctx = _tracker.createInstance<Where_clauseContext>(_ctx, getState());
  enterRule(_localctx, 40, SQLParser::RuleWhere_clause);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(409);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SQLParser::Where_operator_expressionContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(374);
      column();
      setState(375);
      operator_();
      setState(376);
      expression();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SQLParser::Where_value_operator_columnContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(378);
      value();
      setState(379);
      operator_();
      setState(380);
      column();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SQLParser::Where_operator_selectContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(382);
      column();
      setState(383);
      operator_();
      setState(384);
      match(SQLParser::T__10);
      setState(385);
      select_table();
      setState(386);
      match(SQLParser::T__11);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SQLParser::Where_nullContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(388);
      column();
      setState(389);
      match(SQLParser::T__52);
      setState(391);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SQLParser::T__42) {
        setState(390);
        match(SQLParser::T__42);
      }
      setState(393);
      match(SQLParser::Null);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SQLParser::Where_in_listContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(395);
      column();
      setState(396);
      match(SQLParser::T__53);
      setState(397);
      value_list();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SQLParser::Where_in_selectContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(399);
      column();
      setState(400);
      match(SQLParser::T__53);
      setState(401);
      match(SQLParser::T__10);
      setState(402);
      select_table();
      setState(403);
      match(SQLParser::T__11);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<SQLParser::Where_like_stringContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(405);
      column();
      setState(406);
      match(SQLParser::T__54);
      setState(407);
      match(SQLParser::String);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ColumnContext ------------------------------------------------------------------

SQLParser::ColumnContext::ColumnContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> SQLParser::ColumnContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::ColumnContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}


size_t SQLParser::ColumnContext::getRuleIndex() const {
  return SQLParser::RuleColumn;
}


std::any SQLParser::ColumnContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitColumn(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::ColumnContext* SQLParser::column() {
  ColumnContext *_localctx = _tracker.createInstance<ColumnContext>(_ctx, getState());
  enterRule(_localctx, 42, SQLParser::RuleColumn);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(413);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx)) {
    case 1: {
      setState(411);
      match(SQLParser::Identifier);
      setState(412);
      match(SQLParser::T__55);
      break;
    }

    default:
      break;
    }
    setState(415);
    match(SQLParser::Identifier);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

SQLParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::ValueContext* SQLParser::ExpressionContext::value() {
  return getRuleContext<SQLParser::ValueContext>(0);
}

SQLParser::ColumnContext* SQLParser::ExpressionContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}


size_t SQLParser::ExpressionContext::getRuleIndex() const {
  return SQLParser::RuleExpression;
}


std::any SQLParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::ExpressionContext* SQLParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 44, SQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(419);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::Null:
      case SQLParser::UnsignedInteger:
      case SQLParser::String:
      case SQLParser::Float:
      case SQLParser::Minus: {
        enterOuterAlt(_localctx, 1);
        setState(417);
        value();
        break;
      }

      case SQLParser::Identifier: {
        enterOuterAlt(_localctx, 2);
        setState(418);
        column();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Set_clauseContext ------------------------------------------------------------------

SQLParser::Set_clauseContext::Set_clauseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::AssignmentContext *> SQLParser::Set_clauseContext::assignment() {
  return getRuleContexts<SQLParser::AssignmentContext>();
}

SQLParser::AssignmentContext* SQLParser::Set_clauseContext::assignment(size_t i) {
  return getRuleContext<SQLParser::AssignmentContext>(i);
}


size_t SQLParser::Set_clauseContext::getRuleIndex() const {
  return SQLParser::RuleSet_clause;
}


std::any SQLParser::Set_clauseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSet_clause(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Set_clauseContext* SQLParser::set_clause() {
  Set_clauseContext *_localctx = _tracker.createInstance<Set_clauseContext>(_ctx, getState());
  enterRule(_localctx, 46, SQLParser::RuleSet_clause);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(421);
    assignment();
    setState(426);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(422);
      match(SQLParser::T__30);
      setState(423);
      assignment();
      setState(428);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AssignmentContext ------------------------------------------------------------------

SQLParser::AssignmentContext::AssignmentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::ColumnContext* SQLParser::AssignmentContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

tree::TerminalNode* SQLParser::AssignmentContext::EqualOrAssign() {
  return getToken(SQLParser::EqualOrAssign, 0);
}

SQLParser::ValueContext* SQLParser::AssignmentContext::value() {
  return getRuleContext<SQLParser::ValueContext>(0);
}


size_t SQLParser::AssignmentContext::getRuleIndex() const {
  return SQLParser::RuleAssignment;
}


std::any SQLParser::AssignmentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAssignment(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::AssignmentContext* SQLParser::assignment() {
  AssignmentContext *_localctx = _tracker.createInstance<AssignmentContext>(_ctx, getState());
  enterRule(_localctx, 48, SQLParser::RuleAssignment);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(429);
    column();
    setState(430);
    match(SQLParser::EqualOrAssign);
    setState(431);
    value();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SelectorsContext ------------------------------------------------------------------

SQLParser::SelectorsContext::SelectorsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::SelectorContext *> SQLParser::SelectorsContext::selector() {
  return getRuleContexts<SQLParser::SelectorContext>();
}

SQLParser::SelectorContext* SQLParser::SelectorsContext::selector(size_t i) {
  return getRuleContext<SQLParser::SelectorContext>(i);
}


size_t SQLParser::SelectorsContext::getRuleIndex() const {
  return SQLParser::RuleSelectors;
}


std::any SQLParser::SelectorsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSelectors(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::SelectorsContext* SQLParser::selectors() {
  SelectorsContext *_localctx = _tracker.createInstance<SelectorsContext>(_ctx, getState());
  enterRule(_localctx, 50, SQLParser::RuleSelectors);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(442);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SQLParser::T__56: {
        enterOuterAlt(_localctx, 1);
        setState(433);
        match(SQLParser::T__56);
        break;
      }

      case SQLParser::Count:
      case SQLParser::Average:
      case SQLParser::Max:
      case SQLParser::Min:
      case SQLParser::Sum:
      case SQLParser::Identifier: {
        enterOuterAlt(_localctx, 2);
        setState(434);
        selector();
        setState(439);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SQLParser::T__30) {
          setState(435);
          match(SQLParser::T__30);
          setState(436);
          selector();
          setState(441);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SelectorContext ------------------------------------------------------------------

SQLParser::SelectorContext::SelectorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SQLParser::ColumnContext* SQLParser::SelectorContext::column() {
  return getRuleContext<SQLParser::ColumnContext>(0);
}

SQLParser::AggregatorContext* SQLParser::SelectorContext::aggregator() {
  return getRuleContext<SQLParser::AggregatorContext>(0);
}

tree::TerminalNode* SQLParser::SelectorContext::Count() {
  return getToken(SQLParser::Count, 0);
}


size_t SQLParser::SelectorContext::getRuleIndex() const {
  return SQLParser::RuleSelector;
}


std::any SQLParser::SelectorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitSelector(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::SelectorContext* SQLParser::selector() {
  SelectorContext *_localctx = _tracker.createInstance<SelectorContext>(_ctx, getState());
  enterRule(_localctx, 52, SQLParser::RuleSelector);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(454);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(444);
      column();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(445);
      aggregator();
      setState(446);
      match(SQLParser::T__10);
      setState(447);
      column();
      setState(448);
      match(SQLParser::T__11);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(450);
      match(SQLParser::Count);
      setState(451);
      match(SQLParser::T__10);
      setState(452);
      match(SQLParser::T__56);
      setState(453);
      match(SQLParser::T__11);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IdentifiersContext ------------------------------------------------------------------

SQLParser::IdentifiersContext::IdentifiersContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> SQLParser::IdentifiersContext::Identifier() {
  return getTokens(SQLParser::Identifier);
}

tree::TerminalNode* SQLParser::IdentifiersContext::Identifier(size_t i) {
  return getToken(SQLParser::Identifier, i);
}


size_t SQLParser::IdentifiersContext::getRuleIndex() const {
  return SQLParser::RuleIdentifiers;
}


std::any SQLParser::IdentifiersContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitIdentifiers(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::IdentifiersContext* SQLParser::identifiers() {
  IdentifiersContext *_localctx = _tracker.createInstance<IdentifiersContext>(_ctx, getState());
  enterRule(_localctx, 54, SQLParser::RuleIdentifiers);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(456);
    match(SQLParser::Identifier);
    setState(461);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(457);
      match(SQLParser::T__30);
      setState(458);
      match(SQLParser::Identifier);
      setState(463);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_listContext ------------------------------------------------------------------

SQLParser::Column_listContext::Column_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SQLParser::ColumnContext *> SQLParser::Column_listContext::column() {
  return getRuleContexts<SQLParser::ColumnContext>();
}

SQLParser::ColumnContext* SQLParser::Column_listContext::column(size_t i) {
  return getRuleContext<SQLParser::ColumnContext>(i);
}


size_t SQLParser::Column_listContext::getRuleIndex() const {
  return SQLParser::RuleColumn_list;
}


std::any SQLParser::Column_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitColumn_list(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Column_listContext* SQLParser::column_list() {
  Column_listContext *_localctx = _tracker.createInstance<Column_listContext>(_ctx, getState());
  enterRule(_localctx, 56, SQLParser::RuleColumn_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(464);
    column();
    setState(469);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SQLParser::T__30) {
      setState(465);
      match(SQLParser::T__30);
      setState(466);
      column();
      setState(471);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Operator_Context ------------------------------------------------------------------

SQLParser::Operator_Context::Operator_Context(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SQLParser::Operator_Context::EqualOrAssign() {
  return getToken(SQLParser::EqualOrAssign, 0);
}

tree::TerminalNode* SQLParser::Operator_Context::Less() {
  return getToken(SQLParser::Less, 0);
}

tree::TerminalNode* SQLParser::Operator_Context::LessEqual() {
  return getToken(SQLParser::LessEqual, 0);
}

tree::TerminalNode* SQLParser::Operator_Context::Greater() {
  return getToken(SQLParser::Greater, 0);
}

tree::TerminalNode* SQLParser::Operator_Context::GreaterEqual() {
  return getToken(SQLParser::GreaterEqual, 0);
}

tree::TerminalNode* SQLParser::Operator_Context::NotEqual() {
  return getToken(SQLParser::NotEqual, 0);
}


size_t SQLParser::Operator_Context::getRuleIndex() const {
  return SQLParser::RuleOperator_;
}


std::any SQLParser::Operator_Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitOperator_(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::Operator_Context* SQLParser::operator_() {
  Operator_Context *_localctx = _tracker.createInstance<Operator_Context>(_ctx, getState());
  enterRule(_localctx, 58, SQLParser::RuleOperator_);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(472);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & -288230376151711744) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AggregatorContext ------------------------------------------------------------------

SQLParser::AggregatorContext::AggregatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SQLParser::AggregatorContext::Count() {
  return getToken(SQLParser::Count, 0);
}

tree::TerminalNode* SQLParser::AggregatorContext::Average() {
  return getToken(SQLParser::Average, 0);
}

tree::TerminalNode* SQLParser::AggregatorContext::Max() {
  return getToken(SQLParser::Max, 0);
}

tree::TerminalNode* SQLParser::AggregatorContext::Min() {
  return getToken(SQLParser::Min, 0);
}

tree::TerminalNode* SQLParser::AggregatorContext::Sum() {
  return getToken(SQLParser::Sum, 0);
}


size_t SQLParser::AggregatorContext::getRuleIndex() const {
  return SQLParser::RuleAggregator;
}


std::any SQLParser::AggregatorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SQLVisitor*>(visitor))
    return parserVisitor->visitAggregator(this);
  else
    return visitor->visitChildren(this);
}

SQLParser::AggregatorContext* SQLParser::aggregator() {
  AggregatorContext *_localctx = _tracker.createInstance<AggregatorContext>(_ctx, getState());
  enterRule(_localctx, 60, SQLParser::RuleAggregator);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(474);
    _la = _input->LA(1);
    if (!(((((_la - 64) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 64)) & 31) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void SQLParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  sqlParserInitialize();
#else
  ::antlr4::internal::call_once(sqlParserOnceFlag, sqlParserInitialize);
#endif
}
