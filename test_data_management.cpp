#include "record_management.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr int kRecordSize = 64;
using Record = std::array<char, kRecordSize>;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Schema makeSchema() {
    Schema schema{};
    schema.recordsize = kRecordSize;
    schema.nullen = 1;
    schema.col = 1;
    schema.offset[0] = schema.nullen;
    std::strcpy(schema.desc[0].name, "payload");
    schema.desc[0].type = 2;
    schema.desc[0].vcharlen = kRecordSize - schema.nullen;
    schema.desc[0].nullable = 1;
    schema.desc[0].defalutable = 0;
    return schema;
}

Schema makeMultiColumnSchema() {
    Schema schema{};
    schema.nullen = 1;
    schema.col = 3;
    schema.offset[0] = schema.nullen;
    schema.offset[1] = schema.offset[0] + static_cast<int>(sizeof(int));
    schema.offset[2] = schema.offset[1] + static_cast<int>(sizeof(double));
    schema.recordsize = schema.offset[2] + 17;

    std::strcpy(schema.desc[0].name, "id");
    schema.desc[0].type = 0;
    schema.desc[0].nullable = 0;
    schema.desc[0].defalutable = 1;
    schema.desc[0].default_offset = 0;
    schema.desc[0].default_len = sizeof(int);
    const int defaultId = 42;
    std::memcpy(schema.defval + schema.desc[0].default_offset, &defaultId,
                sizeof(defaultId));

    std::strcpy(schema.desc[1].name, "score");
    schema.desc[1].type = 1;
    schema.desc[1].nullable = 1;
    schema.desc[1].defalutable = 1;
    schema.desc[1].default_offset = -1;
    schema.desc[1].default_len = 0;

    std::strcpy(schema.desc[2].name, "name");
    schema.desc[2].type = 2;
    schema.desc[2].vcharlen = 17;
    schema.desc[2].nullable = 0;
    schema.desc[2].defalutable = 1;
    schema.desc[2].default_offset = sizeof(int);
    schema.desc[2].default_len = 7;
    std::memcpy(schema.defval + schema.desc[2].default_offset, "unknown", 7);
    return schema;
}

Record makeRecord(int id) {
    Record record{};
    std::memcpy(record.data() + 1, &id, sizeof(id));
    for (int i = 1 + sizeof(id); i < kRecordSize; ++i) {
        record[i] = static_cast<char>('A' + (id + i) % 26);
    }
    return record;
}

std::string testPath(const char* testName) {
    return "/tmp/ljcdb_" + std::to_string(getpid()) + "_" + testName + ".db";
}

RecordManager* createTable(const std::string& path) {
    Schema schema = makeSchema();
    std::remove(path.c_str());
    return new RecordManager(const_cast<char*>(path.c_str()), schema);
}

void flushTable(RecordManager* table) {
    table->bpm->close();
    table->fm->closeFile(table->fid);
}

void expectRecord(RecordManager* table, ull rid, const Record& expected) {
    Record actual{};
    table->LoadRecord(rid, actual.data());
    require(actual == expected,
            "record mismatch at page " + std::to_string(PID(rid)) +
                ", slot " + std::to_string(SID(rid)));
}

void expectStoredRecord(RecordManager* table, int pageID, int slotID,
                        const Record& expected) {
    int index = -1;
    BufType page = table->bpm->getPage(table->fid, pageID, index);
    Record actual{};
    const char* slot = reinterpret_cast<const char*>(page) + table->offset +
                       slotID * table->tbmeta.schema.recordsize;
    std::memcpy(actual.data(), slot, actual.size());
    require(actual == expected,
            "stored bytes mismatch at page " + std::to_string(pageID) +
                ", slot " + std::to_string(slotID));
}

void expectSchema(const Schema& actual, const Schema& expected) {
    require(actual.recordsize == expected.recordsize,
            "schema record size was not restored");
    require(actual.nullen == expected.nullen,
            "schema NULL bitmap length was not restored");
    require(actual.col == expected.col,
            "schema column count was not restored");
    for (int i = 0; i < expected.col; ++i) {
        require(actual.offset[i] == expected.offset[i],
                "column offset was not restored at column " +
                    std::to_string(i));
        require(std::memcmp(&actual.desc[i], &expected.desc[i], sizeof(Col)) ==
                    0,
                "column description was not restored at column " +
                    std::to_string(i));
    }
    require(std::memcmp(actual.defval, expected.defval,
                        sizeof(expected.defval)) == 0,
            "schema default-value area was not restored");
}

void testSchemaMetadata() {
    const std::string path = testPath("schema");
    RecordManager* table = createTable(path);

    require(table->tbmeta.schema.recordsize == kRecordSize,
            "record size was not derived from schema");
    require(table->tbmeta.schema.col == 1, "column count was not stored");
    require(table->tbmeta.schema.nullen == 1,
            "one column requires one byte in the NULL bitmap");
    require(table->tbmeta.page == 0, "a new table should have no data pages");
    require(table->tbmeta.head == 0,
            "a new table should have an empty free-page list");

    std::remove(path.c_str());
}

void testRidEncoding() {
    const ull rid = RID(123, 456);
    require(PID(rid) == 123, "RID did not preserve the page ID");
    require(SID(rid) == 456, "RID did not preserve the slot ID");
    require(RID(0, 0) == 0, "zero RID should be the scan-end sentinel");
}

void testInsertAndLoad() {
    const std::string path = testPath("insert_load");
    RecordManager* table = createTable(path);
    const Record first = makeRecord(1);
    const Record second = makeRecord(2);

    const ull firstRid = table->InsertRecord(const_cast<char*>(first.data()));
    const ull secondRid = table->InsertRecord(const_cast<char*>(second.data()));

    require(firstRid == RID(1, 0), "first insertion returned the wrong RID");
    require(secondRid == RID(1, 1), "second insertion returned the wrong RID");
    expectStoredRecord(table, 1, 0, first);
    expectStoredRecord(table, 1, 1, second);
    expectRecord(table, firstRid, first);
    expectRecord(table, secondRid, second);
    std::remove(path.c_str());
}

void testUpdate() {
    const std::string path = testPath("update");
    RecordManager* table = createTable(path);
    const Record original = makeRecord(3);
    const Record replacement = makeRecord(4);

    const ull rid = table->InsertRecord(const_cast<char*>(original.data()));
    table->UpdataRecord(rid, const_cast<char*>(replacement.data()));

    expectStoredRecord(table, 1, 0, replacement);
    std::remove(path.c_str());
}

void testEraseAndReuseSlot() {
    const std::string path = testPath("erase_reuse");
    RecordManager* table = createTable(path);
    const Record first = makeRecord(5);
    const Record second = makeRecord(6);
    const Record replacement = makeRecord(7);

    const ull firstRid = table->InsertRecord(const_cast<char*>(first.data()));
    const ull secondRid = table->InsertRecord(const_cast<char*>(second.data()));
    table->EraseRecord(firstRid);
    const ull replacementRid =
        table->InsertRecord(const_cast<char*>(replacement.data()));

    require(secondRid == RID(1, 1), "second record returned the wrong RID");
    require(replacementRid == firstRid, "the erased RID was not reused");
    expectStoredRecord(table, 1, 0, replacement);
    expectStoredRecord(table, 1, 1, second);
    std::remove(path.c_str());
}

void testBitmapByteBoundary() {
    const std::string path = testPath("bitmap_boundary");
    RecordManager* table = createTable(path);

    for (int i = 0; i < 9; ++i) {
        Record record = makeRecord(100 + i);
        table->InsertRecord(record.data());
    }

    int index = -1;
    BufType page = table->bpm->getPage(table->fid, 1, index);
    const unsigned char* bitmap =
        reinterpret_cast<const unsigned char*>(page) + DPOFFSET;
    require(bitmap[0] == 0xff, "the first eight slots should be occupied");
    require((bitmap[1] & 0x01) != 0,
            "slot 8 should use the first bit of the second bitmap byte");
    expectStoredRecord(table, 1, 8, makeRecord(108));
    std::remove(path.c_str());
}

void testPageRollover() {
    const std::string path = testPath("page_rollover");
    RecordManager* table = createTable(path);
    require(table->slotcapa > 8,
            "test requires enough slots to cross a bitmap byte boundary");

    for (int i = 0; i <= table->slotcapa; ++i) {
        Record record = makeRecord(1000 + i);
        table->InsertRecord(record.data());
    }

    require(table->tbmeta.page == 2,
            "inserting slot capacity plus one records should allocate page 2");
    expectStoredRecord(table, 1, table->slotcapa - 1,
                       makeRecord(1000 + table->slotcapa - 1));
    expectStoredRecord(table, 2, 0, makeRecord(1000 + table->slotcapa));
    std::remove(path.c_str());
}

void testFullPageReturnsToFreeList() {
    const std::string path = testPath("full_page_reuse");
    RecordManager* table = createTable(path);

    for (int i = 0; i < table->slotcapa; ++i) {
        Record record = makeRecord(2000 + i);
        table->InsertRecord(record.data());
    }
    require(table->tbmeta.head == 0,
            "a full page should be removed from the free-page list");

    const int erasedSlot = table->slotcapa / 2;
    table->EraseRecord(RID(1, erasedSlot));
    require(table->tbmeta.head == 1,
            "a formerly full page should re-enter the free-page list");

    const Record replacement = makeRecord(3000);
    table->InsertRecord(const_cast<char*>(replacement.data()));
    require(table->tbmeta.head == 0,
            "the page should leave the free-page list after becoming full again");
    expectStoredRecord(table, 1, erasedSlot, replacement);
    std::remove(path.c_str());
}

void testPersistence() {
    const std::string path = testPath("persistence");
    RecordManager* table = createTable(path);
    const Record record = makeRecord(8);
    table->InsertRecord(const_cast<char*>(record.data()));
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    require(reopened->tbmeta.schema.recordsize == kRecordSize,
            "record size was not restored after reopening");
    require(reopened->tbmeta.schema.col == 1,
            "column count was not restored after reopening");
    expectStoredRecord(reopened, 1, 0, record);
    expectRecord(reopened, RID(1, 0), record);
    std::remove(path.c_str());
}

void testSchemaPersistence() {
    const std::string path = testPath("schema_persistence");
    std::remove(path.c_str());
    Schema expected = makeMultiColumnSchema();
    RecordManager* table =
        new RecordManager(const_cast<char*>(path.c_str()), expected);
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    expectSchema(reopened->tbmeta.schema, expected);
    std::remove(path.c_str());
}

void testPageMetadataPersistence() {
    const std::string path = testPath("page_metadata_persistence");
    RecordManager* table = createTable(path);
    Record record = makeRecord(10);
    table->InsertRecord(record.data());
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    require(reopened->tbmeta.page == 1,
            "data page count was not restored after reopening");
    require(reopened->tbmeta.head == 1,
            "free-page list head was not restored after reopening");
    std::remove(path.c_str());
}

void testSchemaSurvivesPageCreation() {
    const std::string path = testPath("schema_after_page");
    std::remove(path.c_str());
    Schema expected = makeMultiColumnSchema();
    RecordManager* table =
        new RecordManager(const_cast<char*>(path.c_str()), expected);
    std::array<char, 30> record{};
    table->InsertRecord(record.data());
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    expectSchema(reopened->tbmeta.schema, expected);
    std::remove(path.c_str());
}

void testMaximumVarcharDefaultPersistence() {
    const std::string path = testPath("maximum_varchar_default");
    std::remove(path.c_str());
    Schema expected = makeSchema();
    expected.desc[0].vcharlen = 2048;
    expected.recordsize = expected.nullen + expected.desc[0].vcharlen;
    expected.desc[0].defalutable = 1;
    expected.desc[0].default_offset = 0;
    expected.desc[0].default_len = sizeof(expected.defval);
    for (int i = 0; i < static_cast<int>(sizeof(expected.defval)); ++i) {
        expected.defval[i] = static_cast<char>('a' + i % 26);
    }

    RecordManager* table =
        new RecordManager(const_cast<char*>(path.c_str()), expected);
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    expectSchema(reopened->tbmeta.schema, expected);
    std::remove(path.c_str());
}

void testDefaultValueMetadata() {
    const Schema schema = makeMultiColumnSchema();
    int defaultId = 0;
    std::memcpy(&defaultId,
                schema.defval + schema.desc[0].default_offset,
                schema.desc[0].default_len);
    require(defaultId == 42, "integer default value was stored incorrectly");
    require(schema.desc[1].defalutable == 1 &&
                schema.desc[1].default_offset == -1 &&
                schema.desc[1].default_len == 0,
            "DEFAULT NULL metadata cannot be distinguished");
    require(std::string(schema.defval + schema.desc[2].default_offset,
                        schema.desc[2].default_len) == "unknown",
            "VARCHAR default value was stored incorrectly");
}

void testDefaultStatesPersistence() {
    const std::string path = testPath("default_states");
    std::remove(path.c_str());
    Schema expected = makeMultiColumnSchema();

    expected.desc[0].defalutable = 0;
    expected.desc[0].default_offset = 0;
    expected.desc[0].default_len = 0;

    expected.desc[1].defalutable = 1;
    expected.desc[1].default_offset = -1;
    expected.desc[1].default_len = 0;

    expected.desc[2].defalutable = 1;
    expected.desc[2].default_offset = 0;
    expected.desc[2].default_len = 0;

    RecordManager* table =
        new RecordManager(const_cast<char*>(path.c_str()), expected);
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    expectSchema(reopened->tbmeta.schema, expected);
    require(reopened->tbmeta.schema.desc[0].defalutable == 0,
            "missing default was not preserved");
    require(reopened->tbmeta.schema.desc[1].defalutable == 1 &&
                reopened->tbmeta.schema.desc[1].default_offset == -1,
            "DEFAULT NULL was not preserved");
    require(reopened->tbmeta.schema.desc[2].defalutable == 1 &&
                reopened->tbmeta.schema.desc[2].default_offset >= 0 &&
                reopened->tbmeta.schema.desc[2].default_len == 0,
            "empty VARCHAR default was not preserved");
    std::remove(path.c_str());
}

void testMetadataFitsOnePage() {
    require(sizeof(TableMeta) <= PAGE_SIZE,
            "TableMeta no longer fits in metadata page 0");
}

void testRecordTraversal() {
    const std::string path = testPath("record_traversal");
    RecordManager* table = createTable(path);
    require(table->FirstRecord() == RID(0, 0),
            "an empty table should have no first record");

    const ull first = table->InsertRecord(makeRecord(7000).data());
    const ull second = table->InsertRecord(makeRecord(7001).data());
    const ull third = table->InsertRecord(makeRecord(7002).data());
    require(table->FirstRecord() == first,
            "FirstRecord did not return the first occupied slot");
    require(table->NextRecord(first) == second,
            "NextRecord did not return the next occupied slot");
    require(table->NextRecord(second) == third,
            "NextRecord skipped an occupied slot");
    require(table->NextRecord(third) == RID(0, 0),
            "NextRecord did not return the end sentinel");

    table->EraseRecord(second);
    require(table->FirstRecord() == first,
            "FirstRecord changed after deleting a later record");
    require(table->NextRecord(first) == third,
            "NextRecord did not skip an erased slot");
    require(table->NextRecord(third) == RID(0, 0),
            "scan did not end after the final occupied slot");

    table->EraseRecord(first);
    require(table->FirstRecord() == third,
            "FirstRecord did not skip empty slots before an occupied slot");
    std::remove(path.c_str());
}

void testFirstRecordAfterGap() {
    const std::string path = testPath("first_record_after_gap");
    RecordManager* table = createTable(path);
    const ull first = table->InsertRecord(makeRecord(7100).data());
    const ull second = table->InsertRecord(makeRecord(7101).data());
    const ull third = table->InsertRecord(makeRecord(7102).data());
    table->EraseRecord(first);
    table->EraseRecord(second);
    require(table->FirstRecord() == third,
            "FirstRecord did not find an occupied nonzero slot");
    std::remove(path.c_str());
}

void testTraversalAcrossPages() {
    const std::string path = testPath("traversal_across_pages");
    RecordManager* table = createTable(path);
    const int capacity = table->slotcapa;
    for (int i = 0; i <= capacity; ++i) {
        Record record = makeRecord(8000 + i);
        const ull rid = table->InsertRecord(record.data());
        const ull expected = i < capacity ? RID(1, i) : RID(2, 0);
        require(rid == expected, "insertion returned the wrong cross-page RID");
    }

    ull current = table->FirstRecord();
    for (int i = 0; i <= capacity; ++i) {
        const ull expected = i < capacity ? RID(1, i) : RID(2, 0);
        require(current == expected,
                "table scan returned an incorrect cross-page RID");
        current = table->NextRecord(current);
    }
    require(current == RID(0, 0),
            "cross-page scan did not return the end sentinel");
    std::remove(path.c_str());
}

void testMaximumColumnNamePersistence() {
    const std::string path = testPath("maximum_column_name");
    std::remove(path.c_str());
    Schema expected = makeSchema();
    for (int i = 0; i < 32; ++i) {
        expected.desc[0].name[i] = static_cast<char>('a' + i % 26);
    }
    RecordManager* table =
        new RecordManager(const_cast<char*>(path.c_str()), expected);
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    require(std::memcmp(reopened->tbmeta.schema.desc[0].name,
                        expected.desc[0].name, 32) == 0,
            "a 32-byte fixed-width column name was not restored");
    std::remove(path.c_str());
}

void testTwoPagePersistence() {
    const std::string path = testPath("two_page_persistence");
    RecordManager* table = createTable(path);
    const int capacity = table->slotcapa;
    for (int i = 0; i <= capacity; ++i) {
        Record record = makeRecord(4000 + i);
        table->InsertRecord(record.data());
    }
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    require(reopened->tbmeta.page == 2,
            "two allocated data pages were not restored");
    require(reopened->tbmeta.head == 2,
            "the partially filled second page should head the free-page list");
    expectRecord(reopened, RID(1, capacity - 1),
                 makeRecord(4000 + capacity - 1));
    expectRecord(reopened, RID(2, 0), makeRecord(4000 + capacity));
    std::remove(path.c_str());
}

void testFreeListPersistence() {
    const std::string path = testPath("free_list_persistence");
    RecordManager* table = createTable(path);
    const int capacity = table->slotcapa;
    for (int i = 0; i <= capacity; ++i) {
        Record record = makeRecord(5000 + i);
        table->InsertRecord(record.data());
    }

    table->EraseRecord(RID(1, 0));
    require(table->tbmeta.head == 1,
            "page 1 should head the free list after deleting from a full page");
    flushTable(table);

    RecordManager* reopened = new RecordManager(const_cast<char*>(path.c_str()));
    require(reopened->tbmeta.head == 1,
            "free-list head was not restored after reopening");
    const Record replacement = makeRecord(6000);
    reopened->InsertRecord(const_cast<char*>(replacement.data()));
    require(reopened->tbmeta.head == 2,
            "filling page 1 should advance the free-list head to page 2");
    expectRecord(reopened, RID(1, 0), replacement);
    expectRecord(reopened, RID(2, 0), makeRecord(5000 + capacity));
    std::remove(path.c_str());
}

void testDestructorFlushesSafely() {
    const std::string path = testPath("destructor");
    RecordManager* table = createTable(path);
    Record record = makeRecord(9);
    table->InsertRecord(record.data());
    delete table;

    struct stat fileInfo {};
    require(stat(path.c_str(), &fileInfo) == 0,
            "table file was not available after destruction");
    require(fileInfo.st_size >= 2 * PAGE_SIZE,
            "destructor did not flush metadata and data pages before closing the file");
    std::remove(path.c_str());
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
        {"schema metadata", testSchemaMetadata},
        {"RID encoding", testRidEncoding},
        {"insert and load", testInsertAndLoad},
        {"update", testUpdate},
        {"erase and reuse slot", testEraseAndReuseSlot},
        {"bitmap byte boundary", testBitmapByteBoundary},
        {"page rollover", testPageRollover},
        {"full page returns to free list", testFullPageReturnsToFreeList},
        {"persistence", testPersistence},
        {"schema persistence", testSchemaPersistence},
        {"page metadata persistence", testPageMetadataPersistence},
        {"schema survives page creation", testSchemaSurvivesPageCreation},
        {"maximum VARCHAR default persistence",
         testMaximumVarcharDefaultPersistence},
        {"default-value metadata", testDefaultValueMetadata},
        {"default states persistence", testDefaultStatesPersistence},
        {"metadata fits one page", testMetadataFitsOnePage},
        {"record traversal", testRecordTraversal},
        {"first record after gap", testFirstRecordAfterGap},
        {"traversal across pages", testTraversalAcrossPages},
        {"maximum column name persistence", testMaximumColumnNamePersistence},
        {"two-page persistence", testTwoPagePersistence},
        {"free-list persistence", testFreeListPersistence},
        {"destructor flush", testDestructorFlushesSafely},
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        if (!runIsolated(test)) {
            ++failures;
        }
    }

    std::cout << (sizeof(tests) / sizeof(tests[0]) - failures) << '/'
              << sizeof(tests) / sizeof(tests[0]) << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
