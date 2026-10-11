#include "system_management.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

namespace fs = std::filesystem;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TemporaryWorkspace {
public:
    TemporaryWorkspace() : previous_(fs::current_path()) {
        char pattern[] = "/tmp/ljcdb_system_management_XXXXXX";
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
    require(columns.size() != 0 && columns.size() <= 10,
            "test schema must contain between one and ten columns");

    Schema schema{};
    schema.col = static_cast<int>(columns.size());
    schema.nullen = (schema.col + 7) / 8;
    int offset = schema.nullen;
    int column = 0;
    for (const ColumnSpec& spec : columns) {
        require(std::strlen(spec.name) < MAX_STR, "column name is too long");
        std::strcpy(schema.desc[column].name, spec.name);
        schema.desc[column].type = spec.type;
        schema.desc[column].len = spec.len;
        schema.desc[column].nullable = spec.nullable ? 1 : 0;
        schema.offset[column] = offset;
        offset += spec.len;
        ++column;
    }
    schema.offset[schema.col] = offset;
    schema.recordsize = offset;
    require(schema.recordsize <= 2048,
            "test record exceeds the supported 2048-byte limit");
    return schema;
}

Schema oneIntSchema(bool nullable = false) {
    return makeSchema({{"value", 0, static_cast<int>(sizeof(int)), nullable}});
}

Schema twoIntSchema(const char* first, const char* second,
                    bool secondNullable = false) {
    return makeSchema({
        {first, 0, static_cast<int>(sizeof(int)), false},
        {second, 0, static_cast<int>(sizeof(int)), secondNullable},
    });
}

bool successfulRid(ull rid) {
    // InsertRow currently uses 0..8 as error codes. Real records start at page 1.
    return rid >= RID(1, 0);
}

void createAndUse(DbManager& manager, const char* databaseName = "testdb") {
    std::string name = databaseName;
    require(manager.CreateDb(name) == 0, "CreateDb failed");
    require(manager.UseDb(name) == 0, "UseDb failed");
}

Table* createTable(DbManager& manager, const char* name, Schema& schema) {
    Table* table = manager.CreateTable(mutableString(name), schema);
    require(table != nullptr, std::string("CreateTable failed for ") + name);
    return table;
}

Index* createIndex(DbManager& manager, const char* tableName,
                   const char* indexName, std::vector<int> columns,
                   int unique = 0, int implicit = 0, int type = 1) {
    return manager.CreateIndex(mutableString(tableName), mutableString(indexName),
                               static_cast<int>(columns.size()), columns,
                               unique, implicit, type);
}

int recordCount(Table* table) {
    int count = 0;
    for (ull rid = table->mng->FirstRecord(); rid != 0;
         rid = table->mng->NextRecord(rid)) {
        ++count;
    }
    return count;
}

void expectIntRow(DbManager& manager, const char* tableName, ull rid,
                  const Row& expected) {
    Row actual = manager.ReadRow(tableName, rid);
    require(actual == expected, "ReadRow returned an unexpected row");
}

void testDatabaseLifecycle() {
    TemporaryWorkspace workspace;
    DbManager manager;
    std::string name = "catalog";
    require(manager.CreateDb(name) == 0, "initial CreateDb failed");
    require(manager.CreateDb(name) == -2, "duplicate database was accepted");
    require(manager.UseDb(name) == 0, "UseDb failed");

    std::vector<std::string> names = manager.ShowAllDb();
    require(std::find(names.begin(), names.end(), name) != names.end(),
            "ShowAllDb omitted the created database");

    require(manager.CloseDb() == 0, "CloseDb failed");
    require(manager.DropDb(name) == 0, "DropDb failed");
    require(!fs::exists(fs::path("data") / name),
            "DropDb left the database directory behind");
    require(manager.UseDb(name) == -1,
            "UseDb accepted a deleted database");
}

void testTableLifecycleAndDuplicateName() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = oneIntSchema();
    Table* table = createTable(manager, "items", schema);
    const fs::path tableFile = table->file_name;

    require(manager.CreateTable(mutableString("items"), schema) == nullptr,
            "duplicate table name was accepted");
    require(manager.table_num == 1, "duplicate table changed table_num");

    manager.DropTable(mutableString("items"));
    require(manager.name_table.count("items") == 0,
            "DropTable left the name mapping behind");
    require(manager.table_num == 0, "DropTable did not decrement table_num");
    require(!fs::exists(tableFile), "DropTable left the data file behind");
}

void testRowValidationAndReadback() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = makeSchema({
        {"id", 0, static_cast<int>(sizeof(int)), false},
        {"score", 1, static_cast<int>(sizeof(double)), false},
        {"name", 2, 8, true},
    });
    Table* table = createTable(manager, "rows", schema);

    Row valid{7, 2.5, std::string("alice")};
    const ull rid = manager.InsertRow("rows", valid);
    require(successfulRid(rid), "valid row insertion failed");
    expectIntRow(manager, "rows", rid, valid);

    Row wrongCount{7, 2.5};
    Row wrongType{std::string("seven"), 2.5, std::string("alice")};
    Row tooLong{8, 3.5, std::string("123456789")};
    Row nullNotAllowed{false, 3.5, std::string("ok")};
    require(!successfulRid(manager.InsertRow("rows", wrongCount)),
            "row with the wrong column count was inserted");
    require(!successfulRid(manager.InsertRow("rows", wrongType)),
            "row with a wrong type was inserted");
    require(!successfulRid(manager.InsertRow("rows", tooLong)),
            "overlength VARCHAR was inserted");
    require(!successfulRid(manager.InsertRow("rows", nullNotAllowed)),
            "NULL was inserted into a NOT NULL column");
    require(recordCount(table) == 1,
            "invalid rows changed the table contents");

    Row missing = manager.ReadRow("rows", RID(999999, 999999));
    require(missing.empty(), "invalid ReadRow unexpectedly produced data");
    require(manager.DeleteRow("rows", RID(999999, 999999)) == ERR_ELSE,
            "DeleteRow accepted an invalid RID");
}

void testPrimaryKeyInsertionRules() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = oneIntSchema(true);
    createTable(manager, "items", schema);
    Index* primary = createIndex(manager, "items", "pk_items", {0}, 1, 1, 0);
    require(primary != nullptr, "primary-key index creation failed");

    Row first{7};
    Row duplicate{7};
    Row nullValue{false};
    require(successfulRid(manager.InsertRow("items", first)),
            "first primary-key value was rejected");
    require(manager.InsertRow("items", duplicate) == ERR_PRIMARY,
            "duplicate primary-key value was not rejected");
    require(manager.InsertRow("items", nullValue) == ERR_PRIMARY,
            "NULL primary-key value was not rejected");
}

void testPrimaryKeyBuildRollback() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);

    Schema nullable = oneIntSchema(true);
    Table* nullTable = createTable(manager, "null_values", nullable);
    Row nullValue{false};
    require(successfulRid(manager.InsertRow("null_values", nullValue)),
            "fixture NULL insertion failed");
    require(createIndex(manager, "null_values", "pk_null", {0}, 1, 1, 0) ==
                nullptr,
            "primary key was created over an existing NULL");
    require(nullTable->pkid == -1, "failed primary key left pkid behind");
    require(manager.idname_idx.count({nullTable->id, "pk_null"}) == 0,
            "failed primary key left index metadata behind");

    Schema nonnull = oneIntSchema(false);
    Table* duplicateTable = createTable(manager, "duplicates", nonnull);
    Row seven{7};
    require(successfulRid(manager.InsertRow("duplicates", seven)),
            "first duplicate fixture insertion failed");
    require(successfulRid(manager.InsertRow("duplicates", seven)),
            "second duplicate fixture insertion failed");
    require(createIndex(manager, "duplicates", "pk_duplicate", {0}, 1, 1,
                        0) == nullptr,
            "primary key was created over duplicate values");
    require(duplicateTable->pkid == -1,
            "duplicate primary-key build left pkid behind");
}

void testUniqueNullSemantics() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = makeSchema({
        {"a", 0, static_cast<int>(sizeof(int)), true},
        {"b", 0, static_cast<int>(sizeof(int)), true},
    });
    Table* table = createTable(manager, "values", schema);
    Index* unique = createIndex(manager, "values", "uq_ab", {0, 1}, 1, 0, 1);
    require(unique != nullptr, "composite UNIQUE creation failed");

    Row nullFirst{false, 1};
    Row nullSecond{1, false};
    Row concrete{1, 2};
    require(successfulRid(manager.InsertRow("values", nullFirst)),
            "first NULL-containing UNIQUE key was rejected");
    require(successfulRid(manager.InsertRow("values", nullFirst)),
            "repeated NULL-containing UNIQUE key was rejected");
    require(successfulRid(manager.InsertRow("values", nullSecond)),
            "second NULL shape was rejected");
    require(successfulRid(manager.InsertRow("values", nullSecond)),
            "repeated second NULL shape was rejected");
    require(successfulRid(manager.InsertRow("values", concrete)),
            "first concrete UNIQUE key was rejected");
    require(manager.InsertRow("values", concrete) == ERR_DUPULICATE,
            "duplicate concrete UNIQUE key was accepted");
    require(recordCount(table) == 5,
            "UNIQUE NULL semantics produced the wrong record count");
}

void testUniqueBuildRejectsExistingDuplicates() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = oneIntSchema(false);
    Table* table = createTable(manager, "values", schema);
    Row seven{7};
    require(successfulRid(manager.InsertRow("values", seven)),
            "first fixture insertion failed");
    require(successfulRid(manager.InsertRow("values", seven)),
            "second fixture insertion failed");

    require(createIndex(manager, "values", "uq_value", {0}, 1, 0, 1) ==
                nullptr,
            "UNIQUE index was created over duplicate existing values");
    require(manager.idname_idx.count({table->id, "uq_value"}) == 0,
            "failed UNIQUE build left name metadata behind");
    require(manager.idx_num == 0,
            "failed UNIQUE build left idx_num inconsistent");
}

void testIndexIdentityRules() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = twoIntSchema("id", "payload");
    Table* table = createTable(manager, "items", schema);

    Index* primary = createIndex(manager, "items", "shared_name", {0}, 1, 1, 0);
    require(primary != nullptr, "primary index fixture creation failed");
    require(createIndex(manager, "items", "shared_name", {1}, 0, 0, 1) ==
                nullptr,
            "normal index reused the primary index name");
    require(manager.idname_idx.at({table->id, "shared_name"}) == primary,
            "rejected name collision overwrote idname_idx");

    Index* first = createIndex(manager, "items", "payload_first", {1});
    require(first != nullptr, "first explicit index creation failed");
    require(createIndex(manager, "items", "payload_second", {1}) == nullptr,
            "duplicate explicit index column tuple was accepted");
}

void testForeignInsertAndDeleteRules() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema parentSchema = twoIntSchema("id", "other");
    Schema childSchema = twoIntSchema("payload", "parent_id", true);
    Table* parent = createTable(manager, "parent", parentSchema);
    Table* child = createTable(manager, "child", childSchema);
    Index* primary = createIndex(manager, "parent", "pk_parent", {0}, 1, 1, 0);
    require(primary != nullptr, "parent primary-key creation failed");

    Row parentSeven{7, 99};
    Row parentEight{8, 100};
    const ull parentSevenRid = manager.InsertRow("parent", parentSeven);
    const ull parentEightRid = manager.InsertRow("parent", parentEight);
    require(successfulRid(parentSevenRid) && successfulRid(parentEightRid),
            "parent fixture insertion failed");

    std::vector<int> childColumns{1};
    Foreign* foreign = manager.CreateForeign(
        mutableString("fk_parent"), parent->id, child->id, childColumns);
    require(foreign != nullptr, "foreign-key creation failed");

    require(manager.DeleteRow("parent", parentEightRid) == OK,
            "unreferenced parent row could not be deleted");

    Row validChild{123, 7};
    Row invalidChild{124, 42};
    Row nullChild{125, false};
    const ull childRid = manager.InsertRow("child", validChild);
    require(successfulRid(childRid), "valid child row was rejected");
    require(manager.InsertRow("child", invalidChild) == ERR_FORIEGN,
            "child row with a missing parent was accepted");
    require(successfulRid(manager.InsertRow("child", nullChild)),
            "NULL foreign-key row was rejected");

    require(manager.DeleteRow("parent", parentSevenRid) == ERR_FORIEGN,
            "referenced parent row was deleted");

    require(manager.DeleteRow("child", childRid) == OK,
            "child row could not be deleted");
    require(manager.DeleteRow("parent", parentSevenRid) == OK,
            "parent remained blocked after its child row was deleted");
}

void testForeignCreationChecksExistingRows() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema parentSchema = oneIntSchema(false);
    Schema childSchema = oneIntSchema(true);
    Table* parent = createTable(manager, "parent", parentSchema);
    Table* badChild = createTable(manager, "bad_child", childSchema);
    Table* nullChild = createTable(manager, "null_child", childSchema);
    require(createIndex(manager, "parent", "pk_parent", {0}, 1, 1, 0) !=
                nullptr,
            "parent primary-key creation failed");

    Row parentValue{7};
    Row badValue{8};
    Row nullValue{false};
    require(successfulRid(manager.InsertRow("parent", parentValue)),
            "parent fixture insertion failed");
    require(successfulRid(manager.InsertRow("bad_child", badValue)),
            "bad-child fixture insertion failed");
    require(successfulRid(manager.InsertRow("null_child", nullValue)),
            "NULL-child fixture insertion failed");

    std::vector<int> column{0};
    require(manager.CreateForeign(mutableString("fk_bad"), parent->id,
                                  badChild->id, column) == nullptr,
            "foreign key was created over orphaned child data");
    require(manager.CreateForeign(mutableString("fk_null"), parent->id,
                                  nullChild->id, column) != nullptr,
            "foreign key rejected an existing NULL child key");
}

void testForeignNameAndSharedImplicitIndex() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = oneIntSchema(false);
    Table* parentOne = createTable(manager, "parent_one", schema);
    Table* parentTwo = createTable(manager, "parent_two", schema);
    Table* child = createTable(manager, "child", schema);
    require(createIndex(manager, "parent_one", "pk_one", {0}, 1, 1, 0) !=
                nullptr,
            "first parent primary-key creation failed");
    require(createIndex(manager, "parent_two", "pk_two", {0}, 1, 1, 0) !=
                nullptr,
            "second parent primary-key creation failed");

    std::vector<int> column{0};
    Foreign* first = manager.CreateForeign(mutableString("fk_one"),
                                           parentOne->id, child->id, column);
    Foreign* second = manager.CreateForeign(mutableString("fk_two"),
                                            parentTwo->id, child->id, column);
    require(first != nullptr && second != nullptr,
            "shared foreign-key fixtures could not be created");
    require(first->ch_idx_id == second->ch_idx_id,
            "foreign keys on the same child columns did not share an index");
    const int sharedIndexId = first->ch_idx_id;

    require(manager.CreateForeign(mutableString("fk_two"), parentOne->id,
                                  child->id, column) == nullptr,
            "duplicate foreign-key name was accepted in one child table");

    manager.DropForeign(mutableString("child"), mutableString("fk_one"));
    require(manager.id_idx.count(sharedIndexId) == 1,
            "dropping one foreign key removed a still-shared child index");
    manager.DropForeign(mutableString("child"), mutableString("fk_two"));
    require(manager.id_idx.count(sharedIndexId) == 0,
            "last foreign key did not remove its implicit child index");
}

void testForeignReusesAndPreservesExplicitIndex() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = oneIntSchema(false);
    Table* parent = createTable(manager, "parent", schema);
    Table* child = createTable(manager, "child", schema);
    require(createIndex(manager, "parent", "pk_parent", {0}, 1, 1, 0) !=
                nullptr,
            "parent primary-key creation failed");
    Index* explicitIndex = createIndex(manager, "child", "child_value", {0});
    require(explicitIndex != nullptr, "explicit child index creation failed");
    const int explicitIndexId = explicitIndex->id;

    std::vector<int> column{0};
    Foreign* foreign = manager.CreateForeign(mutableString("fk_parent"),
                                             parent->id, child->id, column);
    require(foreign != nullptr, "foreign-key creation failed");
    require(foreign->ch_idx_id == explicitIndexId,
            "foreign key did not reuse the explicit child index");

    manager.DropIndex(mutableString("child"), mutableString("child_value"));
    require(manager.id_idx.count(explicitIndexId) == 1,
            "an index required by a foreign key was dropped");
    manager.DropForeign(mutableString("child"), mutableString("fk_parent"));
    require(manager.id_idx.count(explicitIndexId) == 1,
            "dropping a foreign key removed a reused explicit index");
    manager.DropIndex(mutableString("child"), mutableString("child_value"));
    require(manager.id_idx.count(explicitIndexId) == 0,
            "unreferenced explicit index could not be dropped");
}

void testIndexMaintenanceOnDelete() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = twoIntSchema("id", "value");
    Table* table = createTable(manager, "items", schema);
    Index* primary = createIndex(manager, "items", "pk_items", {0}, 1, 1, 0);
    Index* secondary = createIndex(manager, "items", "idx_value", {1});
    require(primary != nullptr && secondary != nullptr,
            "index fixture creation failed");

    Row row{7, 42};
    const ull rid = manager.InsertRow("items", row);
    require(successfulRid(rid), "indexed row insertion failed");
    char primaryKey[1 + sizeof(int)]{};
    char secondaryKey[1 + sizeof(int)]{};
    const int id = 7;
    const int value = 42;
    std::memcpy(primaryKey + 1, &id, sizeof(id));
    std::memcpy(secondaryKey + 1, &value, sizeof(value));
    require(primary->mng->Exist(primaryKey), "primary index missed inserted row");
    require(secondary->mng->Exist(secondaryKey),
            "secondary index missed inserted row");

    require(manager.DeleteRow("items", rid) == OK,
            "indexed row deletion failed");
    require(!primary->mng->Exist(primaryKey),
            "primary index retained a deleted row");
    require(!secondary->mng->Exist(secondaryKey),
            "secondary index retained a deleted row");
    require(recordCount(table) == 0, "deleted record remained in the table");
}

void testUpdateRowAndIndexMaintenance() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = twoIntSchema("id", "value");
    createTable(manager, "items", schema);
    Index* primary = createIndex(manager, "items", "pk_items", {0}, 1, 1, 0);
    Index* unique = createIndex(manager, "items", "uq_value", {1}, 1, 0, 1);
    require(primary != nullptr && unique != nullptr,
            "update index fixtures could not be created");

    Row first{1, 10};
    Row second{2, 20};
    const ull firstRid = manager.InsertRow("items", first);
    require(successfulRid(firstRid) &&
                successfulRid(manager.InsertRow("items", second)),
            "update fixture insertion failed");

    std::vector<int> valueColumn{1};
    Row fifteen{15};
    bool changed = false;
    require(manager.UpdateRow("items", firstRid, valueColumn, fifteen,
                              changed) == OK &&
                changed,
            "ordinary indexed update failed");
    expectIntRow(manager, "items", firstRid, {1, 15});

    char oldKey[1 + sizeof(int)]{};
    char newKey[1 + sizeof(int)]{};
    const int oldValue = 10;
    const int newValue = 15;
    std::memcpy(oldKey + 1, &oldValue, sizeof(oldValue));
    std::memcpy(newKey + 1, &newValue, sizeof(newValue));
    require(!unique->mng->Exist(oldKey) && unique->mng->Exist(newKey),
            "UpdateRow did not move the secondary-index entry");

    changed = true;
    require(manager.UpdateRow("items", firstRid, valueColumn, fifteen,
                              changed) == OK &&
                !changed,
            "no-op update incorrectly reported a change");

    Row duplicate{20};
    changed = true;
    require(manager.UpdateRow("items", firstRid, valueColumn, duplicate,
                              changed) == ERR_DUPULICATE &&
                !changed,
            "UNIQUE-violating update was accepted");
    expectIntRow(manager, "items", firstRid, {1, 15});
    require(unique->mng->Exist(newKey),
            "failed update damaged the existing secondary-index entry");

    Row nullValue{false};
    require(manager.UpdateRow("items", firstRid, valueColumn, nullValue,
                              changed) == ERR_NULL,
            "update put NULL into a NOT NULL column");
}

void testUpdateRowForeignKeyRules() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema parentSchema = oneIntSchema(false);
    Schema childSchema = twoIntSchema("payload", "parent_id", true);
    Table* parent = createTable(manager, "parent", parentSchema);
    Table* child = createTable(manager, "child", childSchema);
    require(createIndex(manager, "parent", "pk_parent", {0}, 1, 1, 0) !=
                nullptr,
            "foreign-update primary index creation failed");
    std::vector<int> childForeignColumn{1};
    require(manager.CreateForeign(mutableString("fk_parent"), parent->id,
                                  child->id, childForeignColumn) != nullptr,
            "foreign-update constraint creation failed");

    Row seven{7};
    Row eight{8};
    const ull sevenRid = manager.InsertRow("parent", seven);
    const ull eightRid = manager.InsertRow("parent", eight);
    Row childRow{100, 7};
    const ull childRid = manager.InsertRow("child", childRow);
    require(successfulRid(sevenRid) && successfulRid(eightRid) &&
                successfulRid(childRid),
            "foreign-update fixture insertion failed");

    bool changed = false;
    Row existingParent{8};
    require(manager.UpdateRow("child", childRid, childForeignColumn,
                              existingParent, changed) == OK &&
                changed,
            "child foreign key could not be changed to an existing parent");
    expectIntRow(manager, "child", childRid, {100, 8});

    Row missingParent{999};
    changed = true;
    require(manager.UpdateRow("child", childRid, childForeignColumn,
                              missingParent, changed) == ERR_FORIEGN &&
                !changed,
            "child foreign key was changed to a missing parent");
    expectIntRow(manager, "child", childRid, {100, 8});

    std::vector<int> parentKeyColumn{0};
    Row newParentKey{9};
    require(manager.UpdateRow("parent", eightRid, parentKeyColumn,
                              newParentKey, changed) == ERR_FORIEGN,
            "referenced parent key was updated");

    Row nullForeign{false};
    require(manager.UpdateRow("child", childRid, childForeignColumn,
                              nullForeign, changed) == OK &&
                changed,
            "nullable child foreign key could not be set to NULL");
    require(manager.UpdateRow("parent", eightRid, parentKeyColumn,
                              newParentKey, changed) == OK &&
                changed,
            "unreferenced parent key remained blocked");
}

void testPersistenceAcrossCloseAndUse() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager, "persistent");
    Schema parentSchema = twoIntSchema("id", "other");
    Schema childSchema = twoIntSchema("payload", "parent_id");
    Table* parent = createTable(manager, "parent", parentSchema);
    Table* child = createTable(manager, "child", childSchema);
    require(createIndex(manager, "parent", "pk_parent", {0}, 1, 1, 0) !=
                nullptr,
            "persistent primary-key creation failed");
    require(createIndex(manager, "child", "uq_payload", {0}, 1, 0, 1) !=
                nullptr,
            "persistent UNIQUE creation failed");
    std::vector<int> foreignColumn{1};
    require(manager.CreateForeign(mutableString("fk_parent"), parent->id,
                                  child->id, foreignColumn) != nullptr,
            "persistent foreign-key creation failed");

    Row parentRow{7, 99};
    Row childRow{123, 7};
    const ull parentRid = manager.InsertRow("parent", parentRow);
    const ull childRid = manager.InsertRow("child", childRow);
    require(successfulRid(parentRid) && successfulRid(childRid),
            "persistent fixture insertion failed");

    require(manager.CloseDb() == 0, "CloseDb failed during persistence test");
    std::string name = "persistent";
    require(manager.UseDb(name) == 0, "UseDb failed after CloseDb");
    require(manager.table_num == 2 && manager.idx_num == 3 && manager.fk_num == 1,
            "catalog counts changed after reopening");
    require(manager.name_table.count("parent") == 1 &&
                manager.name_table.count("child") == 1,
            "table mappings were not rebuilt after reopening");
    expectIntRow(manager, "parent", parentRid, parentRow);
    expectIntRow(manager, "child", childRid, childRow);

    Row duplicateParent{7, 100};
    Row orphanChild{124, 999};
    require(manager.InsertRow("parent", duplicateParent) == ERR_PRIMARY,
            "reopened primary key did not reject a duplicate");
    require(manager.InsertRow("child", orphanChild) == ERR_FORIEGN,
            "reopened foreign key did not reject an orphan");
}

void testMaximumSupportedRecordAndIndexKey() {
    TemporaryWorkspace workspace;
    DbManager manager;
    createAndUse(manager);
    Schema schema = makeSchema({{"payload", 2, 2047, false}});
    require(schema.recordsize == 2048,
            "maximum-record fixture has the wrong size");
    Table* table = createTable(manager, "large_rows", schema);

    Row first{std::string(2047, 'a')};
    const ull firstRid = manager.InsertRow("large_rows", first);
    require(successfulRid(firstRid), "2048-byte record insertion failed");
    Index* index = createIndex(manager, "large_rows", "idx_payload", {0});
    require(index != nullptr, "2048-byte key index creation failed");

    Row second{std::string(2047, 'b')};
    const ull secondRid = manager.InsertRow("large_rows", second);
    require(successfulRid(secondRid),
            "indexed 2048-byte record insertion failed");
    require(recordCount(table) == 2,
            "maximum-record test produced the wrong record count");
    expectIntRow(manager, "large_rows", firstRid, first);
    expectIntRow(manager, "large_rows", secondRid, second);
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
        {"database lifecycle", testDatabaseLifecycle},
        {"table lifecycle and duplicate name", testTableLifecycleAndDuplicateName},
        {"row validation and readback", testRowValidationAndReadback},
        {"primary-key insertion rules", testPrimaryKeyInsertionRules},
        {"primary-key build rollback", testPrimaryKeyBuildRollback},
        {"UNIQUE NULL semantics", testUniqueNullSemantics},
        {"UNIQUE build rejects duplicates", testUniqueBuildRejectsExistingDuplicates},
        {"index identity rules", testIndexIdentityRules},
        {"foreign insert and delete rules", testForeignInsertAndDeleteRules},
        {"foreign creation checks existing rows", testForeignCreationChecksExistingRows},
        {"foreign name and shared implicit index", testForeignNameAndSharedImplicitIndex},
        {"foreign reuses explicit index", testForeignReusesAndPreservesExplicitIndex},
        {"index maintenance on delete", testIndexMaintenanceOnDelete},
        {"UpdateRow and index maintenance", testUpdateRowAndIndexMaintenance},
        {"UpdateRow foreign-key rules", testUpdateRowForeignKeyRules},
        {"persistence across close/use", testPersistenceAcrossCloseAndUse},
        {"maximum supported record and key", testMaximumSupportedRecordAndIndexKey},
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        if (!runIsolated(test)) {
            ++failures;
        }
    }

    const int total = static_cast<int>(sizeof(tests) / sizeof(tests[0]));
    std::cout << total - failures << '/' << total << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
