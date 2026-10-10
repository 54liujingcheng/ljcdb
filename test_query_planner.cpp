#include "query.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;
using Rows = std::vector<Row>;
using Selector = std::pair<std::pair<std::string, std::string>, AggregateFunc>;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TemporaryWorkspace {
public:
    TemporaryWorkspace() : previous_(fs::current_path()) {
        char pattern[] = "/tmp/ljcdb_query_planner_XXXXXX";
        char* created = mkdtemp(pattern);
        if (created == nullptr) {
            throw std::runtime_error("mkdtemp failed");
        }
        root_ = created;
        fs::current_path(root_);
    }

    TemporaryWorkspace(const TemporaryWorkspace&) = delete;
    TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

    ~TemporaryWorkspace() {
        std::error_code error;
        fs::current_path(previous_, error);
        fs::remove_all(root_, error);
    }

private:
    fs::path previous_;
    fs::path root_;
};

char* mutableString(const char* value) {
    return const_cast<char*>(value);
}

struct ColumnSpec {
    const char* name;
    int type;
    int len;
    bool nullable;
};

Schema makeSchema(std::initializer_list<ColumnSpec> columns) {
    require(columns.size() != 0, "test schema cannot be empty");
    Schema schema{};
    schema.col = static_cast<int>(columns.size());
    schema.nullen = (schema.col + 7) / 8;
    int offset = schema.nullen;
    int cid = 0;
    for (const ColumnSpec& column : columns) {
        require(std::strlen(column.name) < MAX_STR,
                "test column name is too long");
        std::strcpy(schema.desc[cid].name, column.name);
        schema.desc[cid].type = column.type;
        schema.desc[cid].len = column.len;
        schema.desc[cid].nullable = column.nullable ? 1 : 0;
        schema.offset[cid] = offset;
        offset += column.len;
        ++cid;
    }
    schema.offset[schema.col] = offset;
    schema.recordsize = offset;
    require(schema.recordsize <= 2048, "test record is too large");
    return schema;
}

bool successfulRid(ull rid) {
    return rid >= RID(1, 0);
}

void insertRow(DbManager& dbm, const std::string& table, Row row) {
    const ull rid = dbm.InsertRow(table, row);
    require(successfulRid(rid), "InsertRow failed for table " + table);
}

class Fixture {
public:
    TemporaryWorkspace workspace;
    DbManager dbm;

    Fixture() {
        std::string database = "planner_test";
        require(dbm.CreateDb(database) == 0, "CreateDb failed");
        require(dbm.UseDb(database) == 0, "UseDb failed");

        Schema users = makeSchema({
            {"id", 0, static_cast<int>(sizeof(int)), false},
            {"dept_id", 0, static_cast<int>(sizeof(int)), true},
            {"age", 0, static_cast<int>(sizeof(int)), false},
            {"name", 2, 16, true},
            {"joined", 3, static_cast<int>(sizeof(DATE)), false},
        });
        Schema departments = makeSchema({
            {"id", 0, static_cast<int>(sizeof(int)), false},
            {"label", 2, 16, false},
            {"region_id", 0, static_cast<int>(sizeof(int)), false},
        });
        Schema regions = makeSchema({
            {"id", 0, static_cast<int>(sizeof(int)), false},
            {"name", 2, 16, false},
        });
        Schema badges = makeSchema({
            {"user_id", 0, static_cast<int>(sizeof(int)), false},
            {"badge", 2, 16, false},
        });
        Schema metrics = makeSchema({
            {"grp", 0, static_cast<int>(sizeof(int)), false},
            {"score", 1, static_cast<int>(sizeof(double)), true},
        });

        require(dbm.CreateTable(mutableString("users"), users) != nullptr,
                "CreateTable users failed");
        require(dbm.CreateTable(mutableString("departments"), departments) !=
                    nullptr,
                "CreateTable departments failed");
        require(dbm.CreateTable(mutableString("regions"), regions) != nullptr,
                "CreateTable regions failed");
        require(dbm.CreateTable(mutableString("badges"), badges) != nullptr,
                "CreateTable badges failed");
        require(dbm.CreateTable(mutableString("empty_badges"), badges) !=
                    nullptr,
                "CreateTable empty_badges failed");
        require(dbm.CreateTable(mutableString("metrics"), metrics) != nullptr,
                "CreateTable metrics failed");

        insertRow(dbm, "users", {1, 10, 20, std::string("alice"), DATE{20200102}});
        insertRow(dbm, "users", {2, 10, 30, std::string("bob"), DATE{20210304}});
        insertRow(dbm, "users", {3, 20, 40, std::string("anna"), DATE{20220506}});
        insertRow(dbm, "users", {4, false, 25, false, DATE{20230708}});
        insertRow(dbm, "users", {5, 30, 18, std::string("alex"), DATE{20191231}});

        insertRow(dbm, "departments", {10, std::string("eng"), 100});
        insertRow(dbm, "departments", {20, std::string("sales"), 200});
        insertRow(dbm, "departments", {30, std::string("ops"), 300});

        insertRow(dbm, "regions", {100, std::string("east")});
        insertRow(dbm, "regions", {200, std::string("west")});

        insertRow(dbm, "badges", {1, std::string("gold")});
        insertRow(dbm, "badges", {1, std::string("silver")});
        insertRow(dbm, "badges", {2, std::string("bronze")});
        insertRow(dbm, "badges", {3, std::string("gold")});
        insertRow(dbm, "badges", {99, std::string("ghost")});

        insertRow(dbm, "metrics", {1, 1.5});
        insertRow(dbm, "metrics", {1, 2.5});
        insertRow(dbm, "metrics", {2, false});
        insertRow(dbm, "metrics", {2, -1.0});
    }
};

Selector selectColumn(std::string table, std::string column) {
    return {{std::move(table), std::move(column)}, AggregateFunc::NONE};
}

Selector selectAggregate(std::string table, std::string column,
                         AggregateFunc aggregate) {
    return {{std::move(table), std::move(column)}, aggregate};
}

Selector countStar() {
    return {{"", ""}, AggregateFunc::COUNT};
}

std::vector<Selector> selectStar() {
    return {{{"", "*"}, AggregateFunc::NONE}};
}

UnboundOprand column(std::string table, std::string name) {
    UnboundOprand operand;
    operand.is_val = false;
    operand.col_name = {std::move(table), std::move(name)};
    return operand;
}

UnboundOprand literal(Value value) {
    UnboundOprand operand;
    operand.is_val = true;
    operand.val = std::move(value);
    return operand;
}

UnboundCondition compare(UnboundOprand left, CompareOp op,
                         UnboundOprand right) {
    UnboundCondition condition;
    condition.type = ConditionType::COMPARE;
    condition.left = std::move(left);
    condition.op = op;
    condition.right = std::move(right);
    return condition;
}

UnboundCondition unaryCondition(ConditionType type, UnboundOprand left) {
    UnboundCondition condition;
    condition.type = type;
    condition.left = std::move(left);
    return condition;
}

UnboundCondition inList(UnboundOprand left, std::vector<Value> values) {
    UnboundCondition condition =
        unaryCondition(ConditionType::IN_LIST, std::move(left));
    condition.values = std::move(values);
    return condition;
}

UnboundCondition likeCondition(UnboundOprand left, std::string pattern) {
    UnboundCondition condition =
        unaryCondition(ConditionType::LIKE, std::move(left));
    condition.pattern = std::move(pattern);
    return condition;
}

UnboundCondition subqueryCondition(ConditionType type, UnboundOprand left,
                                   CompareOp op,
                                   UnboundSelectRequest subquery) {
    UnboundCondition condition;
    condition.type = type;
    condition.left = std::move(left);
    condition.op = op;
    condition.sub =
        std::make_shared<UnboundSelectRequest>(std::move(subquery));
    return condition;
}

UnboundSelectRequest request(std::vector<std::string> tables,
                             std::vector<Selector> selectors,
                             std::vector<UnboundCondition> conditions = {},
                             int limit = -1, int offset = 0) {
    UnboundSelectRequest result;
    result.FROM = std::move(tables);
    result.col_items = std::move(selectors);
    result.conditions = std::move(conditions);
    result.LIMIT = limit;
    result.OFFSET = offset;
    return result;
}

std::unique_ptr<Operator> bindAndPlan(DbManager& dbm,
                                      UnboundSelectRequest request) {
    BoundSelectRequest bound = BindingSelectRequest(dbm, request);
    require(bound.errorcode == 0, "BindingSelectRequest unexpectedly failed");
    std::unique_ptr<Operator> plan = Planner(&dbm, bound);
    require(plan != nullptr, "Planner unexpectedly returned nullptr");
    return plan;
}

struct QueryResult {
    std::vector<LayoutEntry> layout;
    Rows rows;
};

QueryResult runPlan(Operator& plan) {
    QueryResult result;
    result.layout = plan.layout;
    require(plan.Open() == 0, "plan Open failed");
    for (int steps = 0; steps < 100000; ++steps) {
        Tuple tuple;
        const int status = plan.Next(tuple);
        if (status == 1) {
            plan.Close();
            return result;
        }
        if (status == -1) {
            plan.Close();
            throw std::runtime_error("plan Next returned ERROR");
        }
        require(status == 0, "plan Next returned an unknown status");
        result.rows.push_back(std::move(tuple.values));
    }
    plan.Close();
    throw std::runtime_error("plan did not terminate within 100000 Next calls");
}

QueryResult execute(DbManager& dbm, UnboundSelectRequest request) {
    std::unique_ptr<Operator> plan = bindAndPlan(dbm, std::move(request));
    return runPlan(*plan);
}

void setBatchSize(Operator* root, int batchSize) {
    if (auto* join = dynamic_cast<BNLJ*>(root)) {
        join->batch_size = batchSize;
        setBatchSize(join->outer_child.get(), batchSize);
        setBatchSize(join->inner_child.get(), batchSize);
    } else if (auto* filter = dynamic_cast<Filter*>(root)) {
        setBatchSize(filter->child.get(), batchSize);
    } else if (auto* project = dynamic_cast<Project*>(root)) {
        setBatchSize(project->child.get(), batchSize);
    } else if (auto* limit = dynamic_cast<Limit*>(root)) {
        setBatchSize(limit->child.get(), batchSize);
    }
}

template <typename Target>
bool planContains(Operator* root) {
    if (dynamic_cast<Target*>(root) != nullptr) return true;
    if (auto* join = dynamic_cast<BNLJ*>(root)) {
        return planContains<Target>(join->outer_child.get()) ||
               planContains<Target>(join->inner_child.get());
    }
    if (auto* filter = dynamic_cast<Filter*>(root)) {
        return planContains<Target>(filter->child.get());
    }
    if (auto* project = dynamic_cast<Project*>(root)) {
        return planContains<Target>(project->child.get());
    }
    if (auto* limit = dynamic_cast<Limit*>(root)) {
        return planContains<Target>(limit->child.get());
    }
    if (auto* sort = dynamic_cast<Sort*>(root)) {
        return planContains<Target>(sort->child.get());
    }
    if (auto* aggregate = dynamic_cast<Aggregate*>(root)) {
        return planContains<Target>(aggregate->child.get());
    }
    return false;
}

std::string valueText(const Value& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    }
    if (std::holds_alternative<double>(value)) {
        std::ostringstream stream;
        stream << std::get<double>(value);
        return stream.str();
    }
    if (std::holds_alternative<std::string>(value)) {
        return '"' + std::get<std::string>(value) + '"';
    }
    if (std::holds_alternative<DATE>(value)) {
        return "DATE(" + std::to_string(std::get<DATE>(value).date) + ')';
    }
    return "NULL";
}

std::string rowsText(const Rows& rows) {
    std::ostringstream stream;
    for (const Row& row : rows) {
        stream << '[';
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i != 0) stream << ',';
            stream << valueText(row[i]);
        }
        stream << ']';
    }
    return stream.str();
}

void expectRowsUnordered(Rows actual, Rows expected,
                         const std::string& context) {
    std::sort(actual.begin(), actual.end());
    std::sort(expected.begin(), expected.end());
    require(actual == expected,
            context + ": expected " + rowsText(expected) + ", got " +
                rowsText(actual));
}

void expectLayout(const QueryResult& result,
                  const std::vector<std::string>& expected) {
    require(result.layout.size() == expected.size(),
            "layout has an unexpected column count");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(result.layout[i].print_name == expected[i],
                "layout column " + std::to_string(i) + " should be " +
                    expected[i] + ", got " + result.layout[i].print_name);
    }
}

void testSingleTableScanProjectionAndReopen() {
    Fixture fixture;
    const Rows allUsers{
        {1, 10, 20, std::string("alice"), DATE{20200102}},
        {2, 10, 30, std::string("bob"), DATE{20210304}},
        {3, 20, 40, std::string("anna"), DATE{20220506}},
        {4, false, 25, false, DATE{20230708}},
        {5, 30, 18, std::string("alex"), DATE{20191231}},
    };

    QueryResult star = execute(fixture.dbm, request({"users"}, selectStar()));
    expectRowsUnordered(star.rows, allUsers, "single-table SELECT *");
    expectLayout(star, {"users.id", "users.dept_id", "users.age",
                        "users.name", "users.joined"});

    auto projectedRequest = request(
        {"users"},
        {selectColumn("users", "name"), selectColumn("users", "id")});
    std::unique_ptr<Operator> plan =
        bindAndPlan(fixture.dbm, std::move(projectedRequest));
    QueryResult first = runPlan(*plan);
    QueryResult second = runPlan(*plan);
    const Rows expected{{std::string("alice"), 1}, {std::string("bob"), 2},
                        {std::string("anna"), 3}, {false, 4},
                        {std::string("alex"), 5}};
    expectRowsUnordered(first.rows, expected, "single-table projection");
    expectRowsUnordered(second.rows, expected, "plan reopen");
    expectLayout(first, {"users.name", "users.id"});
}

void testComparisonOperatorsAndConjunction() {
    Fixture fixture;
    struct ComparisonCase {
        CompareOp op;
        Rows expected;
    };
    const ComparisonCase cases[] = {
        {CompareOp::EQ, {{3}}},
        {CompareOp::NE, {{1}, {2}, {4}, {5}}},
        {CompareOp::LT, {{1}, {2}}},
        {CompareOp::LE, {{1}, {2}, {3}}},
        {CompareOp::GT, {{4}, {5}}},
        {CompareOp::GE, {{3}, {4}, {5}}},
    };

    for (const ComparisonCase& item : cases) {
        QueryResult result = execute(
            fixture.dbm,
            request({"users"}, {selectColumn("users", "id")},
                    {compare(column("users", "id"), item.op, literal(3))}));
        expectRowsUnordered(result.rows, item.expected,
                            "comparison operator query");
    }

    QueryResult constantLeft = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {compare(literal(3), CompareOp::LT,
                         column("users", "id"))}));
    expectRowsUnordered(constantLeft.rows, {{4}, {5}},
                        "constant-left comparison");

    QueryResult conjunction = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {compare(column("users", "age"), CompareOp::GE, literal(20)),
                 compare(column("users", "age"), CompareOp::LT, literal(40))}));
    expectRowsUnordered(conjunction.rows, {{1}, {2}, {4}},
                        "AND condition query");
}

void testNullInLikeAndDateConditions() {
    Fixture fixture;

    QueryResult isNull = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {unaryCondition(ConditionType::IS_NULL,
                                column("users", "name"))}));
    expectRowsUnordered(isNull.rows, {{4}}, "IS NULL");

    QueryResult isNotNull = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {unaryCondition(ConditionType::IS_NOT_NULL,
                                column("users", "name"))}));
    expectRowsUnordered(isNotNull.rows, {{1}, {2}, {3}, {5}}, "IS NOT NULL");

    QueryResult in = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {inList(column("users", "id"), {1, 3, 5})}));
    expectRowsUnordered(in.rows, {{1}, {3}, {5}}, "IN list");

    QueryResult percentLike = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {likeCondition(column("users", "name"), "a%")}));
    expectRowsUnordered(percentLike.rows, {{1}, {3}, {5}}, "LIKE percent");

    QueryResult underscoreLike = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {likeCondition(column("users", "name"), "a_i_e")}));
    expectRowsUnordered(underscoreLike.rows, {{1}}, "LIKE underscore");

    QueryResult nullComparison = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {compare(column("users", "name"), CompareOp::NE,
                         literal(std::string("alice")))}));
    expectRowsUnordered(nullComparison.rows, {{2}, {3}, {5}},
                        "NULL comparison semantics");

    QueryResult date = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {compare(column("users", "joined"), CompareOp::GE,
                         literal(std::string("2021-01-01")))}));
    expectRowsUnordered(date.rows, {{2}, {3}, {4}}, "DATE comparison binding");
}

void testLimitAndOffset() {
    Fixture fixture;
    QueryResult limited = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")}, {}, 2, 1));
    require(limited.rows == Rows{{2}, {3}},
            "LIMIT/OFFSET did not preserve the sequential scan slice");

    QueryResult zero = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")}, {}, 0, 0));
    require(zero.rows.empty(), "LIMIT 0 returned rows");

    QueryResult pastEnd = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")}, {}, 3, 20));
    require(pastEnd.rows.empty(), "OFFSET past end returned rows");
}

void testTwoTableJoinLocalFiltersAndDuplicates() {
    Fixture fixture;
    const UnboundCondition join =
        compare(column("users", "id"), CompareOp::EQ,
                column("badges", "user_id"));

    QueryResult duplicateMatches = execute(
        fixture.dbm,
        request({"users", "badges"},
                {selectColumn("users", "id"),
                 selectColumn("badges", "badge")},
                {join}));
    expectRowsUnordered(duplicateMatches.rows,
                        {{1, std::string("gold")},
                         {1, std::string("silver")},
                         {2, std::string("bronze")},
                         {3, std::string("gold")}},
                        "two-table duplicate join matches");

    QueryResult filtered = execute(
        fixture.dbm,
        request({"users", "badges"},
                {selectColumn("users", "id"),
                 selectColumn("badges", "badge")},
                {join,
                 compare(column("users", "age"), CompareOp::GE, literal(25)),
                 likeCondition(column("badges", "badge"), "%o%")}));
    expectRowsUnordered(filtered.rows,
                        {{2, std::string("bronze")},
                         {3, std::string("gold")}},
                        "local filters on both join inputs");
}

void testCartesianProductAndEmptyInputs() {
    Fixture fixture;
    QueryResult product = execute(
        fixture.dbm,
        request({"departments", "regions"},
                {selectColumn("departments", "id"),
                 selectColumn("regions", "id")}));
    expectRowsUnordered(product.rows,
                        {{10, 100}, {10, 200}, {20, 100},
                         {20, 200}, {30, 100}, {30, 200}},
                        "Cartesian product");

    QueryResult emptyJoin = execute(
        fixture.dbm,
        request({"users", "empty_badges"}, selectStar()));
    require(emptyJoin.rows.empty(), "join with an empty input returned rows");
    require(emptyJoin.layout.size() == 7,
            "empty join did not preserve the combined layout");

    QueryResult noMatch = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {compare(column("users", "id"), CompareOp::GT, literal(1000))}));
    require(noMatch.rows.empty(), "no-match filter returned rows");
}

void testThreeTableJoinAndConditionPlacement() {
    Fixture fixture;
    std::vector<UnboundCondition> conditions;
    conditions.push_back(compare(column("users", "dept_id"), CompareOp::EQ,
                                 column("departments", "id")));
    conditions.push_back(compare(column("departments", "region_id"),
                                 CompareOp::EQ, column("regions", "id")));
    conditions.push_back(compare(column("users", "age"), CompareOp::GE,
                                 literal(20)));
    conditions.push_back(inList(column("departments", "label"),
                                {std::string("eng"), std::string("sales")}));
    conditions.push_back(likeCondition(column("regions", "name"), "%st"));

    QueryResult result = execute(
        fixture.dbm,
        request({"users", "departments", "regions"},
                {selectColumn("regions", "name"),
                 selectColumn("users", "name"),
                 selectColumn("departments", "label")},
                std::move(conditions)));
    expectRowsUnordered(result.rows,
                        {{std::string("east"), std::string("alice"),
                          std::string("eng")},
                         {std::string("east"), std::string("bob"),
                          std::string("eng")},
                         {std::string("west"), std::string("anna"),
                          std::string("sales")}},
                        "three-table join");
    expectLayout(result,
                 {"regions.name", "users.name", "departments.label"});
}

void testNestedJoinCrossBatchAndReopen() {
    Fixture fixture;
    auto plan = bindAndPlan(
        fixture.dbm,
        request({"users", "departments", "badges"},
                {selectColumn("users", "id"),
                 selectColumn("departments", "label"),
                 selectColumn("badges", "badge")},
                {compare(column("users", "dept_id"), CompareOp::EQ,
                         column("departments", "id")),
                 compare(column("users", "id"), CompareOp::EQ,
                         column("badges", "user_id"))}));
    setBatchSize(plan.get(), 2);

    const Rows expected{{1, std::string("eng"), std::string("gold")},
                        {1, std::string("eng"), std::string("silver")},
                        {2, std::string("eng"), std::string("bronze")},
                        {3, std::string("sales"), std::string("gold")}};
    QueryResult first = runPlan(*plan);
    QueryResult second = runPlan(*plan);
    expectRowsUnordered(first.rows, expected,
                        "nested join crossing BNLJ batches");
    expectRowsUnordered(second.rows, expected,
                        "nested join reopen after batch execution");
}

void testSortFullTopKAndNullOrdering() {
    Fixture fixture;

    UnboundSelectRequest full = request(
        {"users"},
        {selectColumn("users", "id"), selectColumn("users", "age")});
    full.ORDERBY = {{{"users", "age"}, OrderBy::DESC},
                    {{"users", "id"}, OrderBy::ASC}};
    QueryResult fullResult = execute(fixture.dbm, std::move(full));
    require(fullResult.rows == Rows({{3, 40}, {2, 30}, {4, 25},
                                    {1, 20}, {5, 18}}),
            "full Sort returned the wrong order");

    UnboundSelectRequest topK = request(
        {"users"},
        {selectColumn("users", "id"), selectColumn("users", "age")},
        {}, 2, 1);
    topK.ORDERBY = {{{"users", "age"}, OrderBy::DESC},
                    {{"users", "id"}, OrderBy::ASC}};
    QueryResult topKResult = execute(fixture.dbm, std::move(topK));
    require(topKResult.rows == Rows({{2, 30}, {4, 25}}),
            "Top-K Sort did not apply ORDER BY/LIMIT/OFFSET correctly");

    UnboundSelectRequest nullable = request(
        {"users"},
        {selectColumn("users", "id"), selectColumn("users", "name")});
    nullable.ORDERBY = {{{"users", "name"}, OrderBy::ASC}};
    QueryResult nullableResult = execute(fixture.dbm, std::move(nullable));
    require(nullableResult.rows ==
                Rows({{4, false}, {5, std::string("alex")},
                      {1, std::string("alice")}, {3, std::string("anna")},
                      {2, std::string("bob")}}),
            "Sort did not order NULL/VARCHAR values correctly");
}

void testOrderByNonProjectedColumn() {
    Fixture fixture;
    UnboundSelectRequest sorted =
        request({"users"}, {selectColumn("users", "id")});
    sorted.ORDERBY = {{{"users", "age"}, OrderBy::DESC}};
    QueryResult result = execute(fixture.dbm, std::move(sorted));
    require(result.rows == Rows({{3}, {2}, {4}, {1}, {5}}),
            "ORDER BY on a non-projected column returned the wrong order");
}

void testGlobalAndEmptyAggregates() {
    Fixture fixture;
    UnboundSelectRequest aggregate = request(
        {"users"},
        {countStar(),
         selectAggregate("users", "name", AggregateFunc::COUNT),
         selectAggregate("users", "age", AggregateFunc::SUM),
         selectAggregate("users", "age", AggregateFunc::AVG),
         selectAggregate("users", "name", AggregateFunc::MIN),
         selectAggregate("users", "joined", AggregateFunc::MAX)});
    QueryResult result = execute(fixture.dbm, std::move(aggregate));
    require(result.rows ==
                Rows({{5, 4, static_cast<long long>(133), 26.6,
                       std::string("alex"), DATE{20230708}}}),
            "global aggregate values are incorrect");
    expectLayout(result,
                 {"COUNT(*)", "COUNT(users.name)", "SUM(users.age)",
                  "AVG(users.age)", "MIN(users.name)",
                  "MAX(users.joined)"});

    UnboundSelectRequest empty = request(
        {"empty_badges"},
        {countStar(),
         selectAggregate("empty_badges", "user_id", AggregateFunc::SUM),
         selectAggregate("empty_badges", "badge", AggregateFunc::MIN)});
    QueryResult emptyResult = execute(fixture.dbm, std::move(empty));
    require(emptyResult.rows == Rows({{0, false, false}}),
            "global aggregates over an empty input have wrong SQL semantics");
}

void testGroupedAggregateAndOrdering() {
    Fixture fixture;
    UnboundSelectRequest grouped = request(
        {"users"},
        {selectColumn("users", "dept_id"), countStar(),
         selectAggregate("users", "age", AggregateFunc::SUM),
         selectAggregate("users", "age", AggregateFunc::AVG)});
    grouped.GROUPBY = {{"users", "dept_id"}};
    grouped.ORDERBY = {{{"users", "dept_id"}, OrderBy::ASC}};
    QueryResult result = execute(fixture.dbm, std::move(grouped));
    require(result.rows ==
                Rows({{false, 1, static_cast<long long>(25), 25.0},
                      {10, 2, static_cast<long long>(50), 25.0},
                      {20, 1, static_cast<long long>(40), 40.0},
                      {30, 1, static_cast<long long>(18), 18.0}}),
            "GROUP BY aggregates or aggregate ordering are incorrect");

    UnboundSelectRequest emptyGrouped = request(
        {"empty_badges"},
        {selectColumn("empty_badges", "user_id"), countStar()});
    emptyGrouped.GROUPBY = {{"empty_badges", "user_id"}};
    QueryResult emptyResult = execute(fixture.dbm, std::move(emptyGrouped));
    require(emptyResult.rows.empty(),
            "GROUP BY over an empty input unexpectedly produced a group");
}

void testSelectedColumnFunctionallyDependentOnGroupKey() {
    Fixture fixture;
    UnboundSelectRequest grouped = request(
        {"users", "departments"},
        {selectColumn("departments", "label"), countStar()},
        {compare(column("departments", "id"), CompareOp::EQ,
                 column("users", "dept_id"))});
    grouped.GROUPBY = {{"users", "dept_id"}};

    QueryResult result = execute(fixture.dbm, std::move(grouped));
    expectRowsUnordered(result.rows,
                        {{std::string("eng"), 2},
                         {std::string("sales"), 1},
                         {std::string("ops"), 1}},
                        "selected column determined by a joined group key");
    expectLayout(result, {"departments.label", "COUNT(*)"});
}

void testFloatingPointAggregateAndSort() {
    Fixture fixture;
    UnboundSelectRequest grouped = request(
        {"metrics"},
        {selectColumn("metrics", "grp"),
         selectAggregate("metrics", "score", AggregateFunc::COUNT),
         selectAggregate("metrics", "score", AggregateFunc::SUM),
         selectAggregate("metrics", "score", AggregateFunc::AVG),
         selectAggregate("metrics", "score", AggregateFunc::MIN),
         selectAggregate("metrics", "score", AggregateFunc::MAX)});
    grouped.GROUPBY = {{"metrics", "grp"}};
    grouped.ORDERBY = {{{"metrics", "grp"}, OrderBy::ASC}};
    QueryResult groupedResult = execute(fixture.dbm, std::move(grouped));
    require(groupedResult.rows ==
                Rows({{1, 2, 4.0, 2.0, 1.5, 2.5},
                      {2, 1, -1.0, -1.0, -1.0, -1.0}}),
            "FLOAT aggregates or NULL skipping are incorrect");

    UnboundSelectRequest sorted = request(
        {"metrics"},
        {selectColumn("metrics", "grp"),
         selectColumn("metrics", "score")});
    sorted.ORDERBY = {{{"metrics", "score"}, OrderBy::DESC},
                      {{"metrics", "grp"}, OrderBy::ASC}};
    QueryResult sortedResult = execute(fixture.dbm, std::move(sorted));
    require(sortedResult.rows ==
                Rows({{1, 2.5}, {1, 1.5}, {2, -1.0}, {2, false}}),
            "FLOAT/NULL descending Sort is incorrect");
}

void testIndexScanSelectionAndResults() {
    Fixture fixture;
    std::vector<int> idColumn{0};
    std::vector<int> departmentAge{1, 2};
    std::vector<int> nameColumn{3};
    require(fixture.dbm.CreateIndex(mutableString("users"),
                                    mutableString("idx_users_id"), 1,
                                    idColumn, 0, 0, 1) != nullptr,
            "id index creation failed");
    require(fixture.dbm.CreateIndex(mutableString("users"),
                                    mutableString("idx_users_dept_age"), 2,
                                    departmentAge, 0, 0, 1) != nullptr,
            "composite index creation failed");
    require(fixture.dbm.CreateIndex(mutableString("users"),
                                    mutableString("idx_users_name"), 1,
                                    nameColumn, 0, 0, 1) != nullptr,
            "VARCHAR index creation failed");

    UnboundSelectRequest range = request(
        {"users"}, {selectColumn("users", "id")},
        {compare(column("users", "id"), CompareOp::GE, literal(2)),
         compare(column("users", "id"), CompareOp::LT, literal(5))});
    std::unique_ptr<Operator> rangePlan =
        bindAndPlan(fixture.dbm, std::move(range));
    require(planContains<IndexScan>(rangePlan.get()),
            "Planner did not choose IndexScan for an indexed range");
    expectRowsUnordered(runPlan(*rangePlan).rows, {{2}, {3}, {4}},
                        "indexed integer range");

    UnboundSelectRequest composite = request(
        {"users"}, {selectColumn("users", "id")},
        {compare(column("users", "dept_id"), CompareOp::EQ, literal(10)),
         compare(column("users", "age"), CompareOp::GE, literal(25))});
    std::unique_ptr<Operator> compositePlan =
        bindAndPlan(fixture.dbm, std::move(composite));
    require(planContains<IndexScan>(compositePlan.get()),
            "Planner did not choose the composite prefix/range index");
    expectRowsUnordered(runPlan(*compositePlan).rows, {{2}},
                        "composite index range");

    UnboundSelectRequest varcharRange = request(
        {"users"}, {selectColumn("users", "id")},
        {compare(column("users", "name"), CompareOp::GE,
                 literal(std::string("alex"))),
         compare(column("users", "name"), CompareOp::LE,
                 literal(std::string("anna")))});
    std::unique_ptr<Operator> varcharPlan =
        bindAndPlan(fixture.dbm, std::move(varcharRange));
    require(planContains<IndexScan>(varcharPlan.get()),
            "Planner did not choose IndexScan for a VARCHAR range");
    expectRowsUnordered(runPlan(*varcharPlan).rows, {{1}, {3}, {5}},
                        "indexed VARCHAR range");

    UnboundSelectRequest nonPrefix = request(
        {"users"}, {selectColumn("users", "id")},
        {compare(column("users", "age"), CompareOp::GE, literal(30))});
    std::unique_ptr<Operator> nonPrefixPlan =
        bindAndPlan(fixture.dbm, std::move(nonPrefix));
    require(!planContains<IndexScan>(nonPrefixPlan.get()),
            "Planner used a composite index without constraining its prefix");
    expectRowsUnordered(runPlan(*nonPrefixPlan).rows, {{2}, {3}},
                        "non-prefix sequential fallback");
}

void testScalarAndInSubqueries() {
    Fixture fixture;
    UnboundSelectRequest engineering = request(
        {"departments"}, {selectColumn("departments", "id")},
        {compare(column("departments", "label"), CompareOp::EQ,
                 literal(std::string("eng")))});
    UnboundCondition scalar = subqueryCondition(
        ConditionType::SUB_COMPARE, column("users", "dept_id"),
        CompareOp::EQ, std::move(engineering));
    QueryResult scalarResult = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {std::move(scalar),
                 compare(column("users", "id"), CompareOp::GT,
                         literal(1))}));
    expectRowsUnordered(scalarResult.rows, {{2}},
                        "scalar subquery combined with an outer predicate");

    UnboundSelectRequest knownDepartments = request(
        {"departments"}, {selectColumn("departments", "id")},
        {compare(column("departments", "region_id"), CompareOp::LE,
                 literal(200))});
    UnboundCondition inSubquery = subqueryCondition(
        ConditionType::SUB_IN, column("users", "dept_id"), CompareOp::EQ,
        std::move(knownDepartments));
    QueryResult inResult = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {std::move(inSubquery)}));
    expectRowsUnordered(inResult.rows, {{1}, {2}, {3}}, "IN subquery");

    UnboundSelectRequest minimumAge = request(
        {"users"},
        {selectAggregate("users", "age", AggregateFunc::MIN)});
    UnboundCondition aggregateScalar = subqueryCondition(
        ConditionType::SUB_COMPARE, column("users", "age"), CompareOp::EQ,
        std::move(minimumAge));
    QueryResult aggregateScalarResult = execute(
        fixture.dbm,
        request({"users"}, {selectColumn("users", "id")},
                {std::move(aggregateScalar)}));
    expectRowsUnordered(aggregateScalarResult.rows, {{5}},
                        "aggregate scalar subquery");
}

void testScalarSubqueryCardinalityCheck() {
    Fixture fixture;
    UnboundSelectRequest manyRows = request(
        {"departments"}, {selectColumn("departments", "id")});
    UnboundCondition scalar = subqueryCondition(
        ConditionType::SUB_COMPARE, column("users", "dept_id"),
        CompareOp::EQ, std::move(manyRows));
    UnboundSelectRequest outer = request(
        {"users"}, {selectColumn("users", "id")}, {std::move(scalar)});
    BoundSelectRequest bound = BindingSelectRequest(fixture.dbm, outer);
    require(bound.errorcode == 0,
            "valid scalar-subquery syntax failed during binding");
    require(Planner(&fixture.dbm, bound) == nullptr,
            "scalar subquery silently accepted more than one row");
}

void testSubqueryBindingValidation() {
    Fixture fixture;

    UnboundSelectRequest missingTable =
        request({"missing_table"}, {selectColumn("", "id")});
    UnboundCondition missingCondition = subqueryCondition(
        ConditionType::SUB_IN, column("users", "dept_id"), CompareOp::EQ,
        std::move(missingTable));
    UnboundSelectRequest missingOuter = request(
        {"users"}, {selectColumn("users", "id")},
        {std::move(missingCondition)});
    const int missingError =
        BindingSelectRequest(fixture.dbm, missingOuter).errorcode;

    UnboundSelectRequest twoColumns = request(
        {"departments"},
        {selectColumn("departments", "id"),
         selectColumn("departments", "label")});
    UnboundCondition widthCondition = subqueryCondition(
        ConditionType::SUB_IN, column("users", "dept_id"), CompareOp::EQ,
        std::move(twoColumns));
    UnboundSelectRequest widthOuter = request(
        {"users"}, {selectColumn("users", "id")},
        {std::move(widthCondition)});
    const int widthError = BindingSelectRequest(fixture.dbm, widthOuter).errorcode;

    UnboundSelectRequest wrongType = request(
        {"departments"}, {selectColumn("departments", "label")});
    UnboundCondition typeCondition = subqueryCondition(
        ConditionType::SUB_COMPARE, column("users", "dept_id"),
        CompareOp::EQ, std::move(wrongType));
    UnboundSelectRequest typeOuter = request(
        {"users"}, {selectColumn("users", "id")},
        {std::move(typeCondition)});
    const int typeError = BindingSelectRequest(fixture.dbm, typeOuter).errorcode;

    require(missingError != 0 && widthError != 0 && typeError != 0,
            "subquery binding failed to reject nested binding error, "
            "multi-column output, or incompatible output type");
}

void testBindingForDmlDatesAndAggregateRules() {
    Fixture fixture;

    UnboundInsertRequest insertion{
        "users",
        {{6, 30, 50, std::string("cara"), std::string("2024-02-29")}}};
    BoundInsertRequest boundInsert =
        BindingInsertRequest(fixture.dbm, insertion);
    require(boundInsert.errorcode == 0 &&
                std::holds_alternative<DATE>(boundInsert.rows[0][4]) &&
                std::get<DATE>(boundInsert.rows[0][4]).date == 20240229,
            "INSERT binding did not convert a valid DATE");

    UnboundInsertRequest invalidDate{
        "users",
        {{6, 30, 50, std::string("cara"), std::string("2023-02-29")}}};
    require(BindingInsertRequest(fixture.dbm, invalidDate).errorcode ==
                ERR_DATE,
            "INSERT binding accepted an invalid calendar date");

    UnboundUpdateRequest update;
    update.FROM = "users";
    update.col_names = {{"users", "joined"}};
    update.values = {std::string("2024-01-31")};
    update.conditions = {
        compare(column("users", "id"), CompareOp::EQ, literal(1))};
    BoundUpdateRequest boundUpdate = BindingUpdateRequest(fixture.dbm, update);
    require(boundUpdate.errorcode == 0 &&
                std::holds_alternative<DATE>(boundUpdate.values[0]),
            "UPDATE binding did not convert DATE or bind WHERE");

    UnboundDeleteRequest deletion{
        "users",
        {likeCondition(column("users", "name"), "a%")}};
    BoundDeleteRequest boundDelete = BindingDeleteRequest(fixture.dbm, deletion);
    require(boundDelete.errorcode == 0 && boundDelete.conditions.size() == 1,
            "DELETE binding failed for a valid WHERE clause");

    UnboundSelectRequest invalidAggregate = request(
        {"users"},
        {selectColumn("users", "name"),
         selectAggregate("users", "age", AggregateFunc::SUM)});
    require(BindingSelectRequest(fixture.dbm, invalidAggregate).errorcode != 0,
            "binding accepted a non-grouped projected aggregate column");

    UnboundSelectRequest invalidSum = request(
        {"users"},
        {selectAggregate("users", "name", AggregateFunc::SUM)});
    require(BindingSelectRequest(fixture.dbm, invalidSum).errorcode != 0,
            "binding accepted SUM over VARCHAR");
}

void testBindingFailureStopsPlanner() {
    Fixture fixture;
    UnboundSelectRequest ambiguous = request(
        {"users", "departments"}, {selectColumn("", "id")});
    BoundSelectRequest ambiguousBound =
        BindingSelectRequest(fixture.dbm, ambiguous);
    require(ambiguousBound.errorcode != 0,
            "ambiguous unqualified column was accepted");
    require(Planner(&fixture.dbm, ambiguousBound) == nullptr,
            "Planner accepted a failed bound request");

    UnboundSelectRequest missing =
        request({"does_not_exist"}, selectStar());
    BoundSelectRequest missingBound = BindingSelectRequest(fixture.dbm, missing);
    require(missingBound.errorcode != 0, "missing table was accepted");
    require(Planner(&fixture.dbm, missingBound) == nullptr,
            "Planner accepted a request with a missing table");

    UnboundSelectRequest repeated =
        request({"users", "users"}, {selectColumn("users", "id")});
    BoundSelectRequest repeatedBound =
        BindingSelectRequest(fixture.dbm, repeated);
    require(repeatedBound.errorcode != 0,
            "repeated FROM table was accepted although self-join aliases are "
            "not supported");
}

struct TestCase {
    const char* name;
    std::function<void()> run;
};

bool runIsolated(const TestCase& test) {
    std::cout.flush();
    std::cerr.flush();
    const pid_t child = fork();
    if (child == 0) {
        MyBitMap::initConst();
        try {
            test.run();
            std::_Exit(0);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            std::cerr.flush();
            std::_Exit(1);
        }
    }
    if (child < 0) {
        std::cerr << "[FAIL] " << test.name << ": fork failed\n";
        return false;
    }

    int status = 0;
    if (waitpid(child, &status, 0) < 0) {
        std::cerr << "[FAIL] " << test.name << ": waitpid failed\n";
        return false;
    }
    if (WIFSIGNALED(status)) {
        std::cout << "[FAIL] " << test.name << ": signal "
                  << WTERMSIG(status) << '\n';
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cout << "[FAIL] " << test.name << '\n';
        return false;
    }
    std::cout << "[PASS] " << test.name << '\n';
    return true;
}

}  // namespace

int main() {
    const TestCase tests[] = {
        {"single-table scan, projection, and reopen",
         testSingleTableScanProjectionAndReopen},
        {"comparison operators and conjunction",
         testComparisonOperatorsAndConjunction},
        {"NULL, IN, LIKE, and DATE conditions",
         testNullInLikeAndDateConditions},
        {"LIMIT and OFFSET", testLimitAndOffset},
        {"two-table join, local filters, and duplicates",
         testTwoTableJoinLocalFiltersAndDuplicates},
        {"Cartesian product and empty inputs",
         testCartesianProductAndEmptyInputs},
        {"three-table join and condition placement",
         testThreeTableJoinAndConditionPlacement},
        {"nested join across batches and reopen",
         testNestedJoinCrossBatchAndReopen},
        {"full Sort, Top-K, and NULL ordering",
         testSortFullTopKAndNullOrdering},
        {"ORDER BY a non-projected column", testOrderByNonProjectedColumn},
        {"global and empty-input aggregates", testGlobalAndEmptyAggregates},
        {"grouped aggregate and ordering", testGroupedAggregateAndOrdering},
        {"selected column determined by joined group key",
         testSelectedColumnFunctionallyDependentOnGroupKey},
        {"FLOAT aggregate and Sort", testFloatingPointAggregateAndSort},
        {"IndexScan selection and results", testIndexScanSelectionAndResults},
        {"scalar and IN subqueries", testScalarAndInSubqueries},
        {"scalar subquery cardinality check",
         testScalarSubqueryCardinalityCheck},
        {"subquery binding validation", testSubqueryBindingValidation},
        {"DML DATE and aggregate binding rules",
         testBindingForDmlDatesAndAggregateRules},
        {"binding failure stops Planner", testBindingFailureStopsPlanner},
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        if (!runIsolated(test)) ++failures;
    }
    const int total = static_cast<int>(sizeof(tests) / sizeof(tests[0]));
    std::cout << total - failures << '/' << total << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
