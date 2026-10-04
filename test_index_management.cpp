#ifndef TEST
#define TEST
#endif
#ifndef TEST_L
#define TEST_L TEST_T
#endif
#ifndef TEST_I
#define TEST_I 4
#endif
#ifndef TEST_T
#define TEST_T 4
#endif

#include "index_management.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using Entry = std::pair<int, ull>;

constexpr int kNullBytes = 1;
constexpr int kKeySize = kNullBytes + static_cast<int>(sizeof(int));

static_assert(sizeof(BPTMeta) == BPTMETA,
              "BPTMeta no longer matches the on-disk metadata layout");
static_assert(sizeof(PageMeta) == PAGEMETA,
              "PageMeta no longer matches the on-disk page layout");

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string testPath(const char* name) {
    return "/tmp/ljcdb_" + std::to_string(getpid()) + "_" + name + ".idx";
}

BPTMeta makeMeta(bool unique) {
    BPTMeta meta{};
    meta.root = 1;
    meta.lhead = 1;
    meta.ltail = 1;
    meta.freehead = 0;
    meta.page = 0;
    meta.unique = unique ? 1 : 0;
    meta.col = 1;
    meta.type[0] = 0;
    meta.offset[0] = kNullBytes;
    meta.offset[1] = kKeySize;
    return meta;
}

void writeKey(int value, char* key) {
    std::memset(key, 0, kKeySize);
    std::memcpy(key + kNullBytes, &value, sizeof(value));
}

void writeNullKey(char* key) {
    std::memset(key, 0, kKeySize);
    SetKeyColumnNull(key, 0);
}

struct NodeSummary {
    bool hasMax = false;
    Entry max{};
    int depth = 0;
    std::vector<int> leaves;
};

struct NodeSnapshot {
    int type = -1;
    int count = 0;
    int prev = 0;
    int succ = 0;
    std::vector<Entry> entries;
    std::vector<int> children;
};

Entry readEntry(const char* entry, int keyOffset, int keylen) {
    int key = 0;
    ull rid = 0;
    std::memcpy(&key, entry + keyOffset, sizeof(key));
    std::memcpy(&rid, entry + keylen, sizeof(rid));
    return {key, rid};
}

NodeSnapshot snapshot(BPT& tree, int pid) {
    require(pid > 0 && pid <= tree.meta.page,
            "tree points to a page outside the allocated page range");
    int index = -1;
    BufType page = tree.bpm->getPage(tree.fid, pid, index);
    NodeSnapshot result;
    result.type = static_cast<int>(page[0]);
    result.count = static_cast<int>(page[1]);
    result.prev = static_cast<int>(page[2]);
    result.succ = static_cast<int>(page[3]);
    require(result.type == 0 || result.type == 1,
            "reachable page is neither a leaf nor an internal page");
    require(result.count >= 0, "page has a negative entry count");

    const char* data = reinterpret_cast<const char*>(page) + PAGEMETA;
    const int stride = result.type == 0 ? tree.offset_leaf : tree.offset_inter;
    for (int sid = 0; sid < result.count; ++sid) {
        result.entries.push_back(
            readEntry(data + sid * stride, tree.meta.offset[0], tree.keylen));
        if (result.type == 1) {
            int child = 0;
            std::memcpy(&child, data + sid * stride + tree.keylen + 8,
                        sizeof(child));
            result.children.push_back(child);
        }
    }
    return result;
}

NodeSummary validateNode(BPT& tree, int pid, bool isRoot,
                         std::set<int>& visited) {
    require(visited.insert(pid).second, "cycle or duplicate child page found");
    const NodeSnapshot node = snapshot(tree, pid);
    const int capacity = node.type == 0 ? tree.L : tree.I;
    require(node.count <= capacity, "page exceeds its configured capacity");
    if (!isRoot) {
        const int minimum = node.type == 0 ? tree.Lmin : tree.Imin;
        require(node.count >= minimum, "non-root page is below minimum occupancy");
    }

    NodeSummary summary;
    if (node.type == 0) {
        require(node.count == static_cast<int>(node.entries.size()),
                "leaf entry count was not decoded consistently");
        for (std::size_t i = 1; i < node.entries.size(); ++i) {
            require(node.entries[i - 1] < node.entries[i],
                    "leaf entries are not strictly sorted by key/RID");
        }
        summary.depth = 1;
        summary.leaves.push_back(pid);
        if (!node.entries.empty()) {
            summary.hasMax = true;
            summary.max = node.entries.back();
        }
        return summary;
    }

    if (isRoot) {
        require(node.count >= 2,
                "internal root with fewer than two children was not contracted");
    }
    require(node.count > 0, "internal page has no child entries");
    require(node.children.size() == node.entries.size(),
            "internal child count does not match entry count");
    int childDepth = 0;
    for (int sid = 0; sid < node.count; ++sid) {
        NodeSummary child = validateNode(tree, node.children[sid], false, visited);
        require(child.hasMax, "internal page points to an empty subtree");
        require(node.entries[sid] == child.max,
                "internal separator is not the child subtree maximum");
        if (sid > 0) {
            require(node.entries[sid - 1] < node.entries[sid],
                    "internal entries are not strictly sorted");
            require(childDepth == child.depth,
                    "tree has leaves at different depths");
        } else {
            childDepth = child.depth;
        }
        summary.leaves.insert(summary.leaves.end(), child.leaves.begin(),
                              child.leaves.end());
    }
    summary.depth = childDepth + 1;
    summary.hasMax = true;
    summary.max = node.entries.back();
    return summary;
}

int validateTree(BPT& tree) {
    int index = -1;
    BufType rawMeta = tree.bpm->getPage(tree.fid, 0, index);
    const int root = static_cast<int>(rawMeta[0]);
    const int head = static_cast<int>(rawMeta[1]);
    const int tail = static_cast<int>(rawMeta[2]);
    require(root == tree.meta.root, "in-memory root differs from page metadata");
    require(head == tree.meta.lhead, "in-memory leaf head differs from metadata");
    require(tail == tree.meta.ltail, "in-memory leaf tail differs from metadata");
    require(static_cast<int>(rawMeta[3]) == tree.meta.freehead,
            "in-memory free-list head differs from page metadata");
    require(static_cast<int>(rawMeta[4]) == tree.meta.page,
            "in-memory page count differs from metadata");

    std::set<int> visited;
    const NodeSummary rootSummary = validateNode(tree, root, true, visited);
    require(!rootSummary.leaves.empty(), "tree has no reachable leaf page");

    std::vector<int> chain;
    std::set<int> chainVisited;
    int pid = head;
    int previous = 0;
    while (pid != 0) {
        require(chainVisited.insert(pid).second, "leaf sibling chain contains a cycle");
        const NodeSnapshot leaf = snapshot(tree, pid);
        require(leaf.type == 0, "leaf sibling chain points to an internal page");
        require(leaf.prev == previous, "leaf prev pointer is inconsistent");
        if (previous != 0) {
            const NodeSnapshot previousLeaf = snapshot(tree, previous);
            require(previousLeaf.succ == pid,
                    "leaf successor pointer is inconsistent");
            require(!previousLeaf.entries.empty() && !leaf.entries.empty(),
                    "leaf chain contains an empty non-root leaf");
            require(previousLeaf.entries.back() < leaf.entries.front(),
                    "adjacent leaves overlap or are out of order");
        }
        chain.push_back(pid);
        previous = pid;
        pid = leaf.succ;
    }
    require(previous == tail, "leaf sibling chain tail differs from metadata");
    require(chain == rootSummary.leaves,
            "leaf sibling chain differs from the tree's recursive leaf order");
    require(chainVisited == std::set<int>(rootSummary.leaves.begin(),
                                          rootSummary.leaves.end()),
            "some reachable leaf is missing from the sibling chain");

    std::set<int> freePages;
    pid = tree.meta.freehead;
    while (pid != 0) {
        require(pid > 0 && pid <= tree.meta.page,
                "free list points outside the allocated page range");
        require(freePages.insert(pid).second,
                "free page list contains a cycle");
        require(visited.count(pid) == 0,
                "an active tree page also appears in the free list");
        int index = -1;
        BufType page = tree.bpm->getPage(tree.fid, pid, index);
        require(static_cast<int>(page[0]) == 2,
                "free list contains a page not marked free");
        pid = static_cast<int>(page[3]);
    }
    require(visited.size() + freePages.size() ==
                static_cast<std::size_t>(tree.meta.page),
            "allocated page is neither reachable nor on the free list");
    return rootSummary.depth;
}

void checkExactEntries(BPT& tree, const std::vector<Entry>& entries) {
    char key[kKeySize];
    for (const Entry& entry : entries) {
        writeKey(entry.first, key);
        const int level = tree.Search(key, entry.second);
        require(tree._precise, "Search failed to find an inserted entry");
        const int pid = tree._path[level][0];
        const int sid = tree._path[level][1];
        const NodeSnapshot leaf = snapshot(tree, pid);
        require(leaf.type == 0 && sid >= 0 && sid < leaf.count,
                "Search returned an invalid leaf position");
        require(leaf.entries[sid] == entry,
                "Search returned a different entry at the exact position");
    }
}

std::vector<ull> expectedRange(const std::vector<Entry>& entries, int lower,
                               bool lowerInclusive, int upper,
                               bool upperInclusive) {
    std::vector<Entry> sorted = entries;
    std::sort(sorted.begin(), sorted.end());
    std::vector<ull> result;
    for (const Entry& entry : sorted) {
        const bool aboveLower = entry.first > lower ||
                                (lowerInclusive && entry.first == lower);
        const bool belowUpper = entry.first < upper ||
                                (upperInclusive && entry.first == upper);
        if (aboveLower && belowUpper) result.push_back(entry.second);
    }
    return result;
}

void checkSearchInsertionPoint(BPT& tree, const Entry& target) {
    char key[kKeySize];
    writeKey(target.first, key);
    const int level = tree.Search(key, target.second);
    const int pid = tree._path[level][0];
    const int sid = tree._path[level][1];
    const NodeSnapshot leaf = snapshot(tree, pid);
    require(leaf.type == 0, "Search did not finish at a leaf");
    require(sid >= 0 && sid <= leaf.count,
            "Search returned an out-of-range insertion slot");
    if (sid > 0) {
        require(leaf.entries[sid - 1] < target,
                "entry before Search insertion point is not smaller");
    }
    if (sid < leaf.count) {
        require(target <= leaf.entries[sid],
                "entry at Search insertion point is smaller than target");
    } else if (leaf.succ != 0) {
        const NodeSnapshot next = snapshot(tree, leaf.succ);
        require(!next.entries.empty() && target <= next.entries.front(),
                "entry in the next leaf is smaller than Search target");
    }
}

void checkRanges(BPT& tree, const std::vector<Entry>& model,
                 int lower, int upper) {
    char lowerKey[kKeySize];
    char upperKey[kKeySize];
    writeKey(lower, lowerKey);
    writeKey(upper, upperKey);
    const bool combinations[][2] = {
        {true, true}, {true, false}, {false, true}, {false, false}};
    for (const auto& combination : combinations) {
        SearchIter iter =
            tree.SearchRange(lowerKey, upperKey, combination[0]);
        // SearchRange currently has one inclusivity argument. It is used for
        // the lower bound during positioning; set the iterator's upper-bound
        // policy separately so all four boundary combinations are exercised.
        iter.inclusive = combination[1];
        std::vector<ull> actual;
        for (std::size_t count = 0;
             count <= model.size() && tree.Valid(iter); ++count) {
            actual.push_back(iter.rec_rid);
            tree.Next(iter);
        }
        require(actual == expectedRange(model, lower, combination[0], upper,
                                       combination[1]),
                "range boundary result differs from the reference model");
    }
}

BPT* createTree(const std::string& path, bool unique = false) {
    std::remove(path.c_str());
    BPTMeta meta = makeMeta(unique);
    BPT* tree = new BPT(const_cast<char*>(path.c_str()), meta);
    // The test macros force frequent leaf and internal splits.
    tree->L = TEST_T;
    tree->I = TEST_I;
    tree->Lmin = (tree->L + 1) / 2;
    tree->Imin = (tree->I + 1) / 2;
    return tree;
}

BPT* createTreeWithMeta(const std::string& path, BPTMeta meta) {
    std::remove(path.c_str());
    BPT* tree = new BPT(const_cast<char*>(path.c_str()), meta);
    tree->L = TEST_T;
    tree->I = TEST_I;
    tree->Lmin = (tree->L + 1) / 2;
    tree->Imin = (tree->I + 1) / 2;
    return tree;
}

void checkFullScan(BPT& tree, const std::vector<Entry>& inserted) {
    std::vector<Entry> expected = inserted;
    std::sort(expected.begin(), expected.end());

    char lower[kKeySize];
    writeKey(std::numeric_limits<int>::min(), lower);
    SearchIter iter = tree.SearchRange(lower, nullptr, true);
    std::vector<Entry> actual;
    actual.reserve(expected.size());

    for (std::size_t n = 0;
         n <= expected.size() && tree.Valid(iter); ++n) {
        const ull rid = iter.rec_rid;
        auto found = std::find_if(
            inserted.begin(), inserted.end(),
            [rid](const Entry& entry) { return entry.second == rid; });
        require(found != inserted.end(), "scan returned an unknown RID");
        actual.push_back(*found);
        tree.Next(iter);
    }

    require(actual == expected,
            "full leaf scan did not return entries in key/RID order");

    require(!tree.Valid(iter) && !tree.Next(iter),
            "full leaf scan returned more entries than were inserted");
}

void testEmptyAndSingleEntry() {
    const std::string path = testPath("empty_single");
    BPT* tree = createTree(path);

    char key[kKeySize];
    writeKey(7, key);
    SearchIter empty = tree->SearchRange(key, nullptr, true);
    require(!tree->Valid(empty) && !tree->Next(empty),
            "empty tree produced a range result");

    const ull insertedRid = RID(9, 1);
    require(tree->InsertEntry(key, insertedRid) == 0,
            "first insertion failed");
    require(tree->InsertEntry(key, insertedRid) == -1,
            "exact duplicate was accepted");

    SearchIter iter = tree->SearchRange(key, key, true);
    require(tree->Valid(iter), "single entry was not found");
    require(iter.rec_rid == insertedRid,
            "single entry returned the wrong RID");
    require(!tree->Next(iter) && !tree->Valid(iter),
            "single-entry range returned an extra result");

    delete tree;
    std::remove(path.c_str());
}

void testDuplicateAndExactDuplicate() {
    const std::string path = testPath("duplicates");
    BPT* tree = createTree(path);
    std::vector<Entry> inserted;
    char key[kKeySize];

    for (int i = 0; i < 9; ++i) {
        writeKey(i == 4 ? 5 : i, key);
        const ull rid = RID(11, static_cast<unsigned>(i + 1));
        require(tree->InsertEntry(key, rid) == 0,
                "non-unique insertion failed");
        inserted.emplace_back(i == 4 ? 5 : i, rid);
    }

    writeKey(5, key);
    require(tree->InsertEntry(key, RID(11, 5)) == -1,
            "duplicate at an interior position was accepted");
    require(tree->InsertEntry(key, RID(11, 6)) == -1,
            "duplicate at a same-key boundary was accepted");
    writeKey(0, key);
    require(tree->InsertEntry(key, RID(11, 1)) == -1,
            "duplicate at the first leaf position was accepted");
    writeKey(8, key);
    require(tree->InsertEntry(key, RID(11, 9)) == -1,
            "duplicate at the last leaf position was accepted");

    checkFullScan(*tree, inserted);
    delete tree;
    std::remove(path.c_str());
}

void testExactDuplicateAtSearchBoundaries() {
    const std::string path = testPath("exact_search_boundaries");
    BPT* tree = createTree(path);
    char key[kKeySize];

    // With two leaf entries, the second entry is the lower-bound result but
    // is not necessarily compared inside Search's binary-search loop.
    for (int value : {10, 20}) {
        writeKey(value, key);
        require(tree->InsertEntry(key, RID(19, static_cast<unsigned>(value))) == 0,
                "exact-boundary fixture insertion failed");
    }
    for (int value : {10, 20}) {
        writeKey(value, key);
        require(tree->InsertEntry(key, RID(19, static_cast<unsigned>(value))) == -1,
                "exact duplicate at a leaf lower-bound position was accepted");
    }

    delete tree;
    std::remove(path.c_str());
}

void testUniqueIndex() {
    const std::string path = testPath("unique");
    BPT* tree = createTree(path, true);
    char first[kKeySize];
    char second[kKeySize];
    writeKey(42, first);
    writeKey(42, second);

    require(tree->InsertEntry(first, RID(12, 1)) == 0,
            "unique index first insertion failed");
    require(tree->InsertEntry(second, RID(12, 2)) == -2,
            "unique index accepted a duplicate key");

    delete tree;

    BPT reopened(const_cast<char*>(path.c_str()));
    reopened.L = TEST_T;
    reopened.I = TEST_I;
    reopened.Lmin = (reopened.L + 1) / 2;
    reopened.Imin = (reopened.I + 1) / 2;
    require(reopened.meta.unique == 1,
            "unique flag was not restored from index metadata");
    require(reopened.InsertEntry(second, RID(12, 2)) == -2,
            "reopened unique index accepted a duplicate key");
    writeKey(43, second);
    require(reopened.InsertEntry(second, RID(12, 3)) == 0,
            "reopened unique index rejected a distinct key");
    std::remove(path.c_str());
}

void testNullKeysAcrossLeavesAndPersistence() {
    const std::string path = testPath("null_keys");
    std::vector<Entry> expected;
    char nullKey[kKeySize];
    char valueKey[kKeySize];
    writeNullKey(nullKey);

    {
        BPT* tree = createTree(path);

        // Insert in reverse RID order so the test also verifies that NULL keys
        // are ordered by RID and survive leaf/internal splits.
        for (unsigned ridNumber = 9; ridNumber >= 1; --ridNumber) {
            const ull rid = RID(24, ridNumber);
            require(tree->InsertEntry(nullKey, rid) == 0,
                    "NULL-key insertion failed");
            expected.emplace_back(0, rid);
        }
        for (int value = 1; value <= 8; ++value) {
            const ull rid = RID(25, static_cast<unsigned>(value));
            writeKey(value, valueKey);
            require(tree->InsertEntry(valueKey, rid) == 0,
                    "non-NULL fixture insertion failed");
            expected.emplace_back(value, rid);
        }

        require(validateTree(*tree) >= 3,
                "NULL-key fixture did not split into a multi-level tree");

        SearchIter all = tree->SearchRange(nullKey, nullptr, true);
        std::vector<Entry> actual;
        while (tree->Valid(all)) {
            int value = 0;
            if (!KeyColumnIsNull(all.key, 0)) {
                std::memcpy(&value, all.key + kNullBytes, sizeof(value));
            }
            actual.emplace_back(value, all.rec_rid);
            tree->Next(all);
        }
        std::sort(expected.begin(), expected.end());
        require(actual == expected,
                "full scan did not order NULL keys before non-NULL keys");

        SearchIter onlyNull = tree->SearchRange(nullKey, nullKey, true);
        std::vector<ull> nullRids;
        while (tree->Valid(onlyNull)) {
            require(KeyColumnIsNull(onlyNull.key, 0),
                    "NULL equality range returned a non-NULL key");
            nullRids.push_back(onlyNull.rec_rid);
            tree->Next(onlyNull);
        }
        std::vector<ull> expectedNullRids;
        for (unsigned ridNumber = 1; ridNumber <= 9; ++ridNumber) {
            expectedNullRids.push_back(RID(24, ridNumber));
        }
        require(nullRids == expectedNullRids,
                "NULL equality range lost or reordered entries");

        require(tree->DeleteEntry(nullKey, RID(24, 5)) == 0,
                "deleting a NULL-key entry failed");
        require(tree->DeleteEntry(nullKey, RID(24, 5)) == -1,
                "deleting the same NULL-key entry twice succeeded");
        validateTree(*tree);
        delete tree;
    }

    BPT reopened(const_cast<char*>(path.c_str()));
    reopened.L = TEST_T;
    reopened.I = TEST_I;
    reopened.Lmin = (reopened.L + 1) / 2;
    reopened.Imin = (reopened.I + 1) / 2;
    SearchIter onlyNull = reopened.SearchRange(nullKey, nullKey, true);
    std::vector<ull> reopenedRids;
    while (reopened.Valid(onlyNull)) {
        reopenedRids.push_back(onlyNull.rec_rid);
        reopened.Next(onlyNull);
    }
    require(std::find(reopenedRids.begin(), reopenedRids.end(), RID(24, 5)) ==
                reopenedRids.end() &&
                reopenedRids.size() == 8,
            "NULL-key deletion was not persisted");

    // Remove all remaining NULL entries. This forces NULL-containing leaf
    // separators through borrowing, merging, and root contraction paths.
    for (unsigned ridNumber = 1; ridNumber <= 9; ++ridNumber) {
        if (ridNumber == 5) continue;
        require(reopened.DeleteEntry(nullKey, RID(24, ridNumber)) == 0,
                "NULL-key deletion failed during rebalance");
        validateTree(reopened);
    }
    SearchIter noNulls = reopened.SearchRange(nullKey, nullKey, true);
    require(!reopened.Valid(noNulls),
            "NULL equality range remained nonempty after deleting all NULLs");
    char firstValue[kKeySize];
    writeKey(1, firstValue);
    SearchIter remaining = reopened.SearchRange(firstValue, nullptr, true);
    int remainingCount = 0;
    while (reopened.Valid(remaining)) {
        ++remainingCount;
        reopened.Next(remaining);
    }
    require(remainingCount == 8,
            "NULL-key rebalance damaged neighboring non-NULL entries");
    std::remove(path.c_str());
}

void testUniqueAllowsNullKeys() {
    const std::string path = testPath("unique_null");
    BPT* tree = createTree(path, true);
    char nullKey[kKeySize];
    char valueKey[kKeySize];
    writeNullKey(nullKey);
    writeKey(42, valueKey);

    require(tree->InsertEntry(nullKey, RID(26, 1)) == 0,
            "unique index rejected its first NULL key");
    require(tree->InsertEntry(nullKey, RID(26, 2)) == 0,
            "unique index rejected a second NULL key");
    require(tree->InsertEntry(nullKey, RID(26, 1)) == -1,
            "unique index accepted an exact duplicate NULL key/RID");
    require(tree->InsertEntry(valueKey, RID(26, 3)) == 0,
            "unique index rejected a non-NULL key");
    require(tree->InsertEntry(valueKey, RID(26, 4)) == -2,
            "unique index accepted a duplicate non-NULL key");

    delete tree;
    std::remove(path.c_str());
}

void writeCompositeKey(int first, bool firstNull, int second,
                       bool secondNull, char* key) {
    constexpr int compositeKeySize = 1 + 2 * static_cast<int>(sizeof(int));
    std::memset(key, 0, compositeKeySize);
    if (firstNull) {
        SetKeyColumnNull(key, 0);
    } else {
        std::memcpy(key + 1, &first, sizeof(first));
    }
    if (secondNull) {
        SetKeyColumnNull(key, 1);
    } else {
        std::memcpy(key + 1 + sizeof(int), &second, sizeof(second));
    }
}

void testCompositeUniqueNullSemantics() {
    const std::string path = testPath("composite_unique_null");
    std::remove(path.c_str());
    BPTMeta meta{};
    meta.root = meta.lhead = meta.ltail = 1;
    meta.unique = 1;
    meta.col = 2;
    meta.type[0] = meta.type[1] = 0;
    meta.offset[0] = 1;
    meta.offset[1] = 1 + static_cast<int>(sizeof(int));
    meta.offset[2] = 1 + 2 * static_cast<int>(sizeof(int));
    BPT* tree = new BPT(const_cast<char*>(path.c_str()), meta);
    tree->L = TEST_T;
    tree->I = TEST_I;
    tree->Lmin = (tree->L + 1) / 2;
    tree->Imin = (tree->I + 1) / 2;

    char partialNull[1 + 2 * sizeof(int)];
    char leadingNull[1 + 2 * sizeof(int)];
    char nonNull[1 + 2 * sizeof(int)];
    char allNull[1 + 2 * sizeof(int)];
    writeCompositeKey(7, false, 0, true, partialNull);
    writeCompositeKey(0, true, 9, false, leadingNull);
    writeCompositeKey(7, false, 9, false, nonNull);
    writeCompositeKey(0, true, 0, true, allNull);

    require(KeyHasNull(partialNull, 2) && KeyHasNull(leadingNull, 2) &&
                !KeyHasNull(nonNull, 2),
            "composite NULL bitmap was decoded incorrectly");
    require(tree->InsertEntry(partialNull, RID(27, 1)) == 0 &&
                tree->InsertEntry(partialNull, RID(27, 2)) == 0,
            "composite unique index rejected duplicate partial-NULL keys");
    require(tree->InsertEntry(leadingNull, RID(27, 3)) == 0 &&
                tree->InsertEntry(leadingNull, RID(27, 4)) == 0,
            "composite unique index rejected duplicate leading-NULL keys");
    require(tree->InsertEntry(nonNull, RID(27, 5)) == 0,
            "composite unique index rejected a non-NULL key");
    require(tree->InsertEntry(nonNull, RID(27, 6)) == -2,
            "composite unique index accepted a duplicate non-NULL key");

    SearchIter scan = tree->SearchRange(allNull, nullptr, true);
    std::vector<ull> actualOrder;
    while (tree->Valid(scan)) {
        actualOrder.push_back(scan.rec_rid);
        tree->Next(scan);
    }
    require(actualOrder ==
                std::vector<ull>({RID(27, 3), RID(27, 4), RID(27, 1),
                                  RID(27, 2), RID(27, 5)}),
            "composite NULL keys were not ordered lexicographically");

    delete tree;
    std::remove(path.c_str());
}

void testNullBitmapAcrossByteBoundary() {
    constexpr int columnCount = 9;
    constexpr int nullBytes = 2;
    constexpr int wideKeySize =
        nullBytes + columnCount * static_cast<int>(sizeof(int));
    const std::string path = testPath("wide_null_bitmap");

    BPTMeta meta{};
    meta.root = meta.lhead = meta.ltail = 1;
    meta.unique = 1;
    meta.col = columnCount;
    meta.offset[0] = nullBytes;
    for (int i = 0; i < columnCount; ++i) {
        meta.type[i] = 0;
        meta.offset[i + 1] = meta.offset[i] + sizeof(int);
    }
    BPT* tree = createTreeWithMeta(path, meta);

    auto writeWideKey = [&](char* key, int nullColumn) {
        std::memset(key, 0, wideKeySize);
        for (int i = 0; i < columnCount; ++i) {
            const int value = 100 + i;
            std::memcpy(key + meta.offset[i], &value, sizeof(value));
        }
        if (nullColumn >= 0) SetKeyColumnNull(key, nullColumn);
    };

    char nullAtSeven[wideKeySize];
    char nullAtEight[wideKeySize];
    char nonNull[wideKeySize];
    char cleared[wideKeySize];
    char allNull[wideKeySize];
    writeWideKey(nullAtSeven, 7);
    writeWideKey(nullAtEight, 8);
    writeWideKey(nonNull, -1);
    std::memcpy(cleared, nullAtEight, wideKeySize);
    ClearKeyColumnNull(cleared, 8);
    writeWideKey(allNull, -1);
    for (int i = 0; i < columnCount; ++i) SetKeyColumnNull(allNull, i);

    require(KeyColumnIsNull(nullAtSeven, 7) &&
                !KeyColumnIsNull(nullAtSeven, 8) &&
                KeyColumnIsNull(nullAtEight, 8),
            "NULL bitmap failed across the byte-7/byte-8 boundary");
    require(!KeyHasNull(cleared, columnCount) &&
                tree->cmp(cleared, nonNull, 0, 0) == 0,
            "clearing a NULL bit in the second bitmap byte failed");
    require(tree->cmp(nullAtSeven, nullAtEight, 0, 0) < 0 &&
                tree->cmp(nullAtEight, nonNull, 0, 0) < 0,
            "multi-byte NULL bitmap did not participate in lexicographic order");

    require(tree->InsertEntry(nullAtSeven, RID(28, 1)) == 0 &&
                tree->InsertEntry(nullAtEight, RID(28, 2)) == 0 &&
                tree->InsertEntry(nullAtEight, RID(28, 3)) == 0 &&
                tree->InsertEntry(nonNull, RID(28, 4)) == 0 &&
                tree->InsertEntry(allNull, RID(28, 5)) == 0,
            "wide NULL-bitmap insertion failed");
    require(tree->InsertEntry(nonNull, RID(28, 6)) == -2,
            "wide unique index accepted a duplicate non-NULL key");
    require(tree->meta.root != 1,
            "wide NULL-bitmap fixture did not exercise a leaf split");

    SearchIter scan = tree->SearchRange(allNull, nullptr, true);
    int count = 0;
    while (tree->Valid(scan)) {
        ++count;
        tree->Next(scan);
    }
    require(count == 5, "wide NULL-bitmap scan lost entries");

    delete tree;
    std::remove(path.c_str());
}

void testFloatKeyOrderingAndRanges() {
    constexpr int floatKeySize = 1 + static_cast<int>(sizeof(double));
    const std::string path = testPath("float_keys");
    BPTMeta meta{};
    meta.root = meta.lhead = meta.ltail = 1;
    meta.unique = 0;
    meta.col = 1;
    meta.type[0] = 1;
    meta.offset[0] = 1;
    meta.offset[1] = floatKeySize;
    BPT* tree = createTreeWithMeta(path, meta);

    auto writeFloatKey = [](double value, char* key) {
        std::memset(key, 0, floatKeySize);
        std::memcpy(key + 1, &value, sizeof(value));
    };
    using FloatEntry = std::pair<double, ull>;
    const std::vector<double> values = {
        3.5, -2.25, 0.0, 17.0, -100.5, 3.5, 1.25,
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), 8.75,
    };
    std::vector<FloatEntry> expected;
    char key[floatKeySize];
    unsigned ridNumber = 1;
    for (double value : values) {
        const ull rid = RID(29, ridNumber++);
        writeFloatKey(value, key);
        require(tree->InsertEntry(key, rid) == 0,
                "FLOAT-key insertion failed");
        expected.emplace_back(value, rid);
    }
    require(tree->meta.root != 1,
            "FLOAT-key fixture did not exercise a leaf split");

    char lower[floatKeySize];
    writeFloatKey(-std::numeric_limits<double>::infinity(), lower);
    SearchIter scan = tree->SearchRange(lower, nullptr, true);
    std::vector<FloatEntry> actual;
    while (tree->Valid(scan)) {
        double value = 0;
        std::memcpy(&value, scan.key + 1, sizeof(value));
        actual.emplace_back(value, scan.rec_rid);
        tree->Next(scan);
    }
    std::sort(expected.begin(), expected.end());
    require(actual == expected, "FLOAT keys were not scanned in numeric order");

    char rangeLower[floatKeySize];
    char rangeUpper[floatKeySize];
    writeFloatKey(-2.25, rangeLower);
    writeFloatKey(3.5, rangeUpper);
    SearchIter range = tree->SearchRange(rangeLower, rangeUpper, true);
    std::vector<FloatEntry> ranged;
    while (tree->Valid(range)) {
        double value = 0;
        std::memcpy(&value, range.key + 1, sizeof(value));
        ranged.emplace_back(value, range.rec_rid);
        tree->Next(range);
    }
    std::vector<FloatEntry> expectedRangeEntries;
    for (const FloatEntry& entry : expected) {
        if (entry.first >= -2.25 && entry.first <= 3.5) {
            expectedRangeEntries.push_back(entry);
        }
    }
    require(ranged == expectedRangeEntries,
            "FLOAT closed range returned the wrong entries");

    delete tree;
    std::remove(path.c_str());
}

void testVarcharKeyOrderingAndRanges() {
    constexpr int varcharLength = 8;
    constexpr int varcharKeySize = 1 + varcharLength;
    const std::string path = testPath("varchar_keys");
    BPTMeta meta{};
    meta.root = meta.lhead = meta.ltail = 1;
    meta.unique = 0;
    meta.col = 1;
    meta.type[0] = 2;
    meta.offset[0] = 1;
    meta.offset[1] = varcharKeySize;
    BPT* tree = createTreeWithMeta(path, meta);

    auto writeVarcharKey = [](const std::string& value, char* key) {
        std::memset(key, 0, varcharKeySize);
        std::memcpy(key + 1, value.data(),
                    std::min(value.size(), static_cast<std::size_t>(varcharLength)));
    };
    auto readVarcharKey = [](const char* key) {
        std::size_t length = 0;
        while (length < varcharLength && key[1 + length] != '\0') ++length;
        return std::string(key + 1, length);
    };

    using StringEntry = std::pair<std::string, ull>;
    const std::vector<std::string> values = {
        "b", "", "aa", "a", "z", "ab", "alpha", "alp", "aa", "ba",
    };
    std::vector<StringEntry> expected;
    char key[varcharKeySize];
    unsigned ridNumber = 1;
    for (const std::string& value : values) {
        const ull rid = RID(30, ridNumber++);
        writeVarcharKey(value, key);
        require(tree->InsertEntry(key, rid) == 0,
                "VARCHAR-key insertion failed");
        expected.emplace_back(value, rid);
    }
    require(tree->meta.root != 1,
            "VARCHAR-key fixture did not exercise a leaf split");

    char lower[varcharKeySize];
    writeVarcharKey("", lower);
    SearchIter scan = tree->SearchRange(lower, nullptr, true);
    std::vector<StringEntry> actual;
    while (tree->Valid(scan)) {
        actual.emplace_back(readVarcharKey(scan.key), scan.rec_rid);
        tree->Next(scan);
    }
    std::sort(expected.begin(), expected.end());
    require(actual == expected,
            "VARCHAR keys were not scanned in lexicographic order");

    char rangeLower[varcharKeySize];
    char rangeUpper[varcharKeySize];
    writeVarcharKey("aa", rangeLower);
    writeVarcharKey("b", rangeUpper);
    SearchIter range = tree->SearchRange(rangeLower, rangeUpper, true);
    range.inclusive = false;
    std::vector<StringEntry> ranged;
    while (tree->Valid(range)) {
        ranged.emplace_back(readVarcharKey(range.key), range.rec_rid);
        tree->Next(range);
    }
    std::vector<StringEntry> expectedRangeEntries;
    for (const StringEntry& entry : expected) {
        if (entry.first >= "aa" && entry.first < "b") {
            expectedRangeEntries.push_back(entry);
        }
    }
    require(ranged == expectedRangeEntries,
            "VARCHAR half-open range returned the wrong entries");

    delete tree;
    std::remove(path.c_str());
}

void runNodeCapacityCase(int leafCapacity, int internalCapacity,
                         const char* caseName) {
    const std::string path = testPath(caseName);
    BPT* tree = createTree(path);
    tree->L = leafCapacity;
    tree->I = internalCapacity;
    tree->Lmin = (tree->L + 1) / 2;
    tree->Imin = (tree->I + 1) / 2;

    std::vector<Entry> model;
    char key[kKeySize];
    for (int value = 0; value < 120; ++value) {
        const int shuffled = (value * 53) % 127;
        const ull rid = RID(31 + static_cast<unsigned>(leafCapacity * 10 +
                                                       internalCapacity),
                            static_cast<unsigned>(value + 1));
        writeKey(shuffled, key);
        require(tree->InsertEntry(key, rid) == 0,
                "capacity-boundary insertion failed");
        model.emplace_back(shuffled, rid);
    }
    require(validateTree(*tree) >= 4,
            "capacity-boundary fixture did not create a deep tree");
    checkFullScan(*tree, model);

    std::vector<Entry> deletionOrder = model;
    std::sort(deletionOrder.begin(), deletionOrder.end(),
              [](const Entry& left, const Entry& right) {
                  const ull leftGroup = left.second % 7;
                  const ull rightGroup = right.second % 7;
                  return leftGroup != rightGroup
                             ? leftGroup < rightGroup
                             : left.second < right.second;
              });
    for (std::size_t i = 0; i < deletionOrder.size(); ++i) {
        const Entry entry = deletionOrder[i];
        writeKey(entry.first, key);
        require(tree->DeleteEntry(key, entry.second) == 0,
                "capacity-boundary deletion failed");
        model.erase(std::remove(model.begin(), model.end(), entry), model.end());
        if (i % 11 == 0) {
            validateTree(*tree);
            checkFullScan(*tree, model);
        }
    }
    require(validateTree(*tree) == 1,
            "capacity-boundary tree did not contract to a leaf root");
    checkFullScan(*tree, model);
    delete tree;
    std::remove(path.c_str());
}

void testOddNodeCapacities() {
    const std::vector<std::pair<int, int>> capacities = {
        {3, 3}, {3, 5}, {5, 3},
    };
    int caseNumber = 0;
    for (const auto& capacity : capacities) {
        const std::string name =
            "odd_capacity_" + std::to_string(caseNumber++);
        runNodeCapacityCase(capacity.first, capacity.second, name.c_str());
    }
}

void testMinimumLeafCapacity() {
    runNodeCapacityCase(2, 3, "minimum_leaf_capacity");
}

void testMinimumInternalCapacity() {
    runNodeCapacityCase(3, 2, "minimum_internal_capacity");
}

void testMinimumLeafAndInternalCapacity() {
    runNodeCapacityCase(2, 2, "minimum_leaf_internal_capacity");
}

void testMultiLevelSplitsAndPersistence() {
    const std::string path = testPath("multilevel");
    std::vector<Entry> inserted;
    {
        BPT* tree = createTree(path);
        char key[kKeySize];

        for (int i = 0; i < 80; ++i) {
            const int value = (i * 37) % 53;
            const ull rid = RID(13, static_cast<unsigned>(i + 1));
            writeKey(value, key);
            require(tree->InsertEntry(key, rid) == 0,
                    "insertion failed during multi-level split test");
            inserted.emplace_back(value, rid);
        }

        require(tree->meta.root != 1,
                "root never changed despite many forced splits");
        require(tree->meta.page > 10,
                "multi-level test did not allocate enough pages");
        checkFullScan(*tree, inserted);
        delete tree;
    }

    BPT reopened(const_cast<char*>(path.c_str()));
    reopened.L = TEST_T;
    reopened.I = TEST_I;
    reopened.Lmin = (reopened.L + 1) / 2;
    reopened.Imin = (reopened.I + 1) / 2;
    checkFullScan(reopened, inserted);
    std::remove(path.c_str());
}

void testFreeListPersistenceAndReuse() {
    const std::string path = testPath("free_list_persistence");
    constexpr int kEntryCount = 192;
    int allocatedPages = 0;
    int persistedFreeHead = 0;

    {
        BPT* tree = createTree(path);
        std::vector<Entry> entries;
        char key[kKeySize];
        for (int i = 0; i < kEntryCount; ++i) {
            const int value = (i * 73) % 211;
            const ull rid = RID(34, static_cast<unsigned>(i + 1));
            writeKey(value, key);
            require(tree->InsertEntry(key, rid) == 0,
                    "free-list fixture insertion failed");
            entries.emplace_back(value, rid);
        }
        require(validateTree(*tree) >= 4,
                "free-list fixture did not create a deep tree");
        allocatedPages = tree->meta.page;

        std::sort(entries.begin(), entries.end(),
                  [](const Entry& left, const Entry& right) {
                      const ull leftGroup = left.second % 13;
                      const ull rightGroup = right.second % 13;
                      return leftGroup != rightGroup
                                 ? leftGroup < rightGroup
                                 : left.second < right.second;
                  });
        for (std::size_t i = 0; i < entries.size(); ++i) {
            writeKey(entries[i].first, key);
            require(tree->DeleteEntry(key, entries[i].second) == 0,
                    "free-list fixture deletion failed");
            if (i % 17 == 0) {
                validateTree(*tree);
            }
        }

        require(validateTree(*tree) == 1,
                "empty tree did not contract to a leaf before reopen");
        require(tree->meta.page == allocatedPages,
                "deleting entries unexpectedly changed the high-water page count");
        require(tree->meta.freehead != 0,
                "emptying a deep tree did not create a free-page list");
        persistedFreeHead = tree->meta.freehead;
        delete tree;
    }

    {
        BPT reopened(const_cast<char*>(path.c_str()));
        reopened.L = TEST_T;
        reopened.I = TEST_I;
        reopened.Lmin = (reopened.L + 1) / 2;
        reopened.Imin = (reopened.I + 1) / 2;
        require(reopened.meta.page == allocatedPages,
                "page high-water mark was not restored after reopen");
        require(reopened.meta.freehead == persistedFreeHead,
                "free-list head was not restored after reopen");
        require(validateTree(reopened) == 1,
                "reopened empty tree is structurally invalid");

        std::vector<Entry> rebuilt;
        char key[kKeySize];
        for (int i = 0; i < kEntryCount; ++i) {
            const int value = (i * 73) % 211;
            const ull rid = RID(35, static_cast<unsigned>(i + 1));
            writeKey(value, key);
            require(reopened.InsertEntry(key, rid) == 0,
                    "insertion after restoring the free list failed");
            rebuilt.emplace_back(value, rid);
            if (i == TEST_T) {
                require(reopened.meta.page == allocatedPages,
                        "first split after reopen grew the file despite free pages");
                require(reopened.meta.freehead != persistedFreeHead,
                        "first split after reopen did not consume the free list");
            }
        }
        require(validateTree(reopened) >= 4,
                "tree rebuilt from persisted free pages is not deep enough");
        require(reopened.meta.page <= allocatedPages,
                "reopened tree allocated new pages instead of reusing free pages");
        checkFullScan(reopened, rebuilt);
    }

    std::remove(path.c_str());
}

std::vector<ull> scanRange(BPT& tree, int lower, bool lowerInclusive,
                           int upper, bool upperInclusive) {
    char lowerKey[kKeySize];
    char upperKey[kKeySize];
    writeKey(lower, lowerKey);
    writeKey(upper, upperKey);
    SearchIter iter = tree.SearchRange(lowerKey, upperKey, lowerInclusive);
    iter.inclusive = upperInclusive;
    std::vector<ull> result;
    while (tree.Valid(iter)) {
        result.push_back(iter.rec_rid);
        require(result.size() <= 100,
                "range scan did not make progress or did not terminate");
        tree.Next(iter);
    }
    return result;
}

void testRangeBoundaries() {
    const std::string path = testPath("ranges");
    BPT* tree = createTree(path);
    char key[kKeySize];
    std::map<ull, int> values;

    for (int value = 1; value <= 9; ++value) {
        writeKey(value, key);
        const ull rid = RID(14, static_cast<unsigned>(value));
        require(tree->InsertEntry(key, rid) == 0,
                "range fixture insertion failed");
        values[rid] = value;
    }

    const std::vector<ull> closed =
        scanRange(*tree, 3, true, 6, true);
    const std::vector<ull> openUpper =
        scanRange(*tree, 3, true, 6, false);
    const std::vector<ull> openLower =
        scanRange(*tree, 3, false, 6, true);
    const std::vector<ull> openBoth =
        scanRange(*tree, 3, false, 6, false);

    auto expected = [&values](const std::vector<int>& keys) {
        std::vector<ull> result;
        for (const auto& item : values) {
            if (std::find(keys.begin(), keys.end(), item.second) != keys.end()) {
                result.push_back(item.first);
            }
        }
        return result;
    };

    require(closed == expected({3, 4, 5, 6}),
            "closed range returned the wrong entries");
    require(openUpper == expected({3, 4, 5}),
            "open upper range returned the wrong entries");
    require(openLower == expected({4, 5, 6}),
            "open lower range returned the wrong entries");
    require(openBoth == expected({4, 5}),
            "open range returned the wrong entries");

    require(scanRange(*tree, 8, true, 3, true).empty(),
            "range with lower bound above upper bound was not empty");
    require(scanRange(*tree, -100, true, -50, true).empty(),
            "range entirely below the minimum key was not empty");
    require(scanRange(*tree, 50, true, 100, true).empty(),
            "range entirely above the maximum key was not empty");
    require(scanRange(*tree, 5, true, 5, true) == expected({5}),
            "closed singleton range returned the wrong entries");
    require(scanRange(*tree, 5, false, 5, true).empty() &&
                scanRange(*tree, 5, true, 5, false).empty() &&
                scanRange(*tree, 5, false, 5, false).empty(),
            "open singleton range was not empty");

    // Exercise Next's own upper-bound guard even when the caller does not
    // invoke Valid before advancing an already out-of-range cursor.
    char low[kKeySize];
    char high[kKeySize];
    writeKey(8, low);
    writeKey(3, high);
    SearchIter invalid = tree->SearchRange(low, high, true);
    require(!tree->Next(invalid) && !tree->Valid(invalid),
            "Next did not stop an already out-of-range cursor");

    delete tree;
    std::remove(path.c_str());
}

void testDuplicateRangeBoundaries() {
    const std::string path = testPath("duplicate_ranges");
    BPT* tree = createTree(path);
    const std::vector<Entry> inserted = {
        {3, RID(15, 8)}, {3, RID(15, 2)}, {3, RID(15, 5)},
        {4, RID(15, 7)}, {4, RID(15, 1)}, {5, RID(15, 4)},
    };
    char key[kKeySize];
    for (const Entry& entry : inserted) {
        writeKey(entry.first, key);
        require(tree->InsertEntry(key, entry.second) == 0,
                "duplicate-range fixture insertion failed");
    }

    const std::vector<ull> closed = scanRange(*tree, 3, true, 4, true);
    const std::vector<ull> openLower = scanRange(*tree, 3, false, 4, true);
    const std::vector<ull> openUpper = scanRange(*tree, 3, true, 4, false);

    require(closed == std::vector<ull>({RID(15, 2), RID(15, 5), RID(15, 8),
                                        RID(15, 1), RID(15, 7)}),
            "closed duplicate-key range returned the wrong entries");
    require(openLower == std::vector<ull>({RID(15, 1), RID(15, 7)}),
            "exclusive lower duplicate-key range returned the wrong entries");
    require(openUpper == std::vector<ull>({RID(15, 2), RID(15, 5), RID(15, 8)}),
            "exclusive upper duplicate-key range returned the wrong entries");

    delete tree;
    std::remove(path.c_str());
}

void testManyDuplicatesAcrossLeaves() {
    const std::string path = testPath("many_duplicates");
    BPT* tree = createTree(path);
    std::vector<Entry> model;
    char key[kKeySize];

    for (unsigned ridNumber = 1; ridNumber <= 37; ++ridNumber) {
        const Entry entry{42, RID(20, ridNumber)};
        writeKey(entry.first, key);
        require(tree->InsertEntry(key, entry.second) == 0,
                "many-duplicate insertion failed");
        model.push_back(entry);
    }
    for (int value = 0; value < 20; ++value) {
        const Entry entry{value, RID(21, static_cast<unsigned>(value + 1))};
        writeKey(entry.first, key);
        require(tree->InsertEntry(key, entry.second) == 0,
                "duplicate boundary fixture insertion failed");
        model.push_back(entry);
    }

    require(validateTree(*tree) >= 4,
            "many-duplicate fixture did not create a deep tree");
    checkFullScan(*tree, model);
    checkExactEntries(*tree, model);
    checkRanges(*tree, model, 41, 43);

    const std::vector<ull> expected = [&model]() {
        std::vector<Entry> sorted = model;
        std::sort(sorted.begin(), sorted.end());
        std::vector<ull> result;
        for (const Entry& entry : sorted) {
            if (entry.first == 42) result.push_back(entry.second);
        }
        return result;
    }();
    char lower[kKeySize];
    char upper[kKeySize];
    writeKey(42, lower);
    writeKey(42, upper);
    SearchIter iter = tree->SearchRange(lower, upper, true);
    std::vector<ull> actual;
    while (tree->Valid(iter)) {
        actual.push_back(iter.rec_rid);
        tree->Next(iter);
    }
    require(actual == expected,
            "range scan lost or duplicated entries spanning duplicate-key leaves");

    delete tree;
    std::remove(path.c_str());
}

void testDeleteAndRebalance() {
    const std::string path = testPath("delete_rebalance");
    BPT* tree = createTree(path);
    std::vector<Entry> remaining;
    char key[kKeySize];

    for (int value = 0; value < 48; ++value) {
        const Entry entry{value, RID(16, static_cast<unsigned>(value + 1))};
        writeKey(entry.first, key);
        require(tree->InsertEntry(key, entry.second) == 0,
                "delete fixture insertion failed");
        remaining.push_back(entry);
    }
    checkFullScan(*tree, remaining);

    const ull missingRid = RID(16, 999);
    writeKey(24, key);
    require(tree->DeleteEntry(key, missingRid) == -1,
            "deleting a nonexistent RID succeeded");
    checkFullScan(*tree, remaining);

    std::vector<int> order;
    for (int value = 0; value < 48; value += 2) order.push_back(value);
    for (int value = 47; value >= 1; value -= 2) order.push_back(value);

    for (int value : order) {
        const ull rid = RID(16, static_cast<unsigned>(value + 1));
        writeKey(value, key);
        require(tree->DeleteEntry(key, rid) == 0,
                "deletion failed during rebalance test");
        remaining.erase(
            std::remove(remaining.begin(), remaining.end(), Entry{value, rid}),
            remaining.end());
        checkFullScan(*tree, remaining);
    }

    writeKey(0, key);
    require(tree->DeleteEntry(key, RID(16, 1)) == -1,
            "deleting from an empty tree succeeded");
    delete tree;
    std::remove(path.c_str());
}

void testDeterministicMixedOperations() {
    const std::string path = testPath("mixed_operations");
    BPT* tree = createTree(path);
    std::vector<Entry> model;
    char key[kKeySize];
    unsigned state = 0x13579bdfU;
    unsigned nextRid = 1;

    auto nextRandom = [&state]() {
        state = state * 1664525U + 1013904223U;
        return state;
    };

    for (int step = 0; step < 240; ++step) {
        const bool doInsert = model.empty() || nextRandom() % 100 < 58;
        if (doInsert) {
            const int value = static_cast<int>(nextRandom() % 41) - 20;
            const ull rid = RID(17, nextRid++);
            writeKey(value, key);
            require(tree->InsertEntry(key, rid) == 0,
                    "mixed-operation insertion failed");
            model.emplace_back(value, rid);
        } else {
            const std::size_t selected = nextRandom() % model.size();
            const Entry entry = model[selected];
            writeKey(entry.first, key);
            require(tree->DeleteEntry(key, entry.second) == 0,
                    "mixed-operation deletion failed");
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(selected));
        }

        checkFullScan(*tree, model);

        if (step % 17 == 0) {
            const int missingKey = static_cast<int>(nextRandom() % 41) - 20;
            const ull missingRid = RID(17, 0x70000000U + step);
            writeKey(missingKey, key);
            require(tree->DeleteEntry(key, missingRid) == -1,
                    "mixed-operation deletion accepted a missing RID");
            checkFullScan(*tree, model);
        }
    }

    delete tree;
    std::remove(path.c_str());
}

void testFourLevelBoundaryOperations() {
    const std::string path = testPath("four_level_boundaries");
    BPT* tree = createTree(path);
    std::vector<Entry> model;
    char key[kKeySize];
    unsigned ridNumber = 1;

    auto insertValue = [&](int value) {
        const Entry entry{value, RID(18, ridNumber++)};
        writeKey(value, key);
        require(tree->InsertEntry(key, entry.second) == 0,
                "four-level insertion failed");
        model.push_back(entry);
    };

    // Each phase stresses a different edge of the tree. With TEST_T=4 and
    // TEST_I=4 this creates substantially more than four levels of pages.
    for (int value = 0; value < 256; value += 2) insertValue(value);
    for (int value = 511; value >= 1; value -= 2) insertValue(value);
    for (int value = 512; value < 768; ++value) insertValue(value);
    for (int value = -256; value < 0; ++value) insertValue(value);

    const int depth = validateTree(*tree);
    require(depth >= 4, "boundary fixture did not create a four-level tree");
    checkFullScan(*tree, model);
    checkExactEntries(*tree, model);
    checkRanges(*tree, model, -256, 768);

    const std::vector<Entry> probes = {
        {-257, RID(18, 0)}, {-256, RID(18, 1)}, {-1, RID(18, 1)},
        {0, RID(18, 1)}, {1, RID(18, 1)}, {127, RID(18, 1)},
        {128, RID(18, 1)}, {255, RID(18, 1)}, {256, RID(18, 1)},
        {511, RID(18, 1)}, {512, RID(18, 1)}, {767, RID(18, 1)},
        {768, RID(18, 1)}, {1000, RID(18, 1)},
    };
    for (const Entry& probe : probes) checkSearchInsertionPoint(*tree, probe);

    // Delete the two global boundaries, then delete a patterned set that
    // repeatedly reaches leaf edges and forces internal rebalancing.
    const std::vector<int> firstDeletes = {-256, 767, -255, 766, 0, 1,
                                           2, 3, 764, 765, 4, 5};
    for (int value : firstDeletes) {
        auto found = std::find_if(model.begin(), model.end(),
                                  [value](const Entry& entry) {
                                      return entry.first == value;
                                  });
        require(found != model.end(), "boundary delete fixture entry missing");
        writeKey(found->first, key);
        require(tree->DeleteEntry(key, found->second) == 0,
                "boundary deletion failed");
        model.erase(found);
        validateTree(*tree);
        checkFullScan(*tree, model);
    }

    for (int value = -254; value < 768; value += 3) {
        auto found = std::find_if(model.begin(), model.end(),
                                  [value](const Entry& entry) {
                                      return entry.first == value;
                                  });
        if (found == model.end()) continue;
        const Entry entry = *found;
        writeKey(entry.first, key);
        require(tree->DeleteEntry(key, entry.second) == 0,
                "patterned boundary deletion failed");
        model.erase(found);
        if (value % 15 == 1) {
            validateTree(*tree);
            checkFullScan(*tree, model);
        }
    }

    validateTree(*tree);
    checkFullScan(*tree, model);

    // Empty the deep tree to exercise repeated root contraction and the free
    // page list, then allocate pages again from the resulting free list.
    std::vector<Entry> deletionOrder = model;
    std::sort(deletionOrder.begin(), deletionOrder.end(),
              [](const Entry& left, const Entry& right) {
                  return (left.second % 11) < (right.second % 11);
              });
    int deletionCount = 0;
    for (const Entry& entry : deletionOrder) {
        writeKey(entry.first, key);
        require(tree->DeleteEntry(key, entry.second) == 0,
                "deep tree deletion to empty failed");
        if (++deletionCount % 29 == 0) {
            require(validateTree(*tree) >= 1,
                    "tree became structurally invalid while emptying");
        }
    }
    model.clear();
    require(validateTree(*tree) == 1,
            "empty tree did not contract back to one leaf root");
    checkFullScan(*tree, model);

    for (int value : {std::numeric_limits<int>::min(), -1, 0, 1,
                      std::numeric_limits<int>::max()}) {
        insertValue(value);
    }
    require(validateTree(*tree) >= 1,
            "reusing free pages corrupted the rebuilt tree");
    checkFullScan(*tree, model);
    delete tree;
    std::remove(path.c_str());
}

void testSplitInsertionOrders() {
    const int kEntryCount = 384;
    const std::vector<std::string> names = {
        "ascending", "descending", "alternating", "random"};

    for (std::size_t pattern = 0; pattern < names.size(); ++pattern) {
        const std::string path = testPath(("split_" + names[pattern]).c_str());
        BPT* tree = createTree(path);
        std::vector<int> values;
        values.reserve(kEntryCount);
        if (pattern == 0) {
            for (int value = 0; value < kEntryCount; ++value) {
                values.push_back(value);
            }
        } else if (pattern == 1) {
            for (int value = kEntryCount - 1; value >= 0; --value) {
                values.push_back(value);
            }
        } else if (pattern == 2) {
            int low = 0;
            int high = kEntryCount - 1;
            while (low <= high) {
                values.push_back(low++);
                if (low <= high) values.push_back(high--);
            }
        } else {
            unsigned state = 0x2468ace1U;
            for (int count = 0; count < kEntryCount; ++count) {
                state = state * 1103515245U + 12345U;
                values.push_back(static_cast<int>(state % kEntryCount));
            }
            // Keep RIDs unique while retaining repeated keys in this pattern.
        }

        std::vector<Entry> model;
        char key[kKeySize];
        unsigned ridNumber = 1;
        for (int value : values) {
            const Entry entry{value, RID(22 + static_cast<unsigned>(pattern),
                                        ridNumber++)};
            writeKey(value, key);
            require(tree->InsertEntry(key, entry.second) == 0,
                    "ordered split fixture insertion failed");
            model.push_back(entry);
        }

        require(validateTree(*tree) >= 4,
                "ordered split fixture did not reach four levels");
        checkFullScan(*tree, model);
        checkExactEntries(*tree, model);
        checkRanges(*tree, model, -1, kEntryCount);

        // Probe every gap at the leaf and internal boundaries, including the
        // first and last possible insertion positions.
        for (int probe = -1; probe <= kEntryCount; probe += 7) {
            checkSearchInsertionPoint(*tree,
                                      {probe, RID(99, static_cast<unsigned>(probe + 2))});
        }

        delete tree;
        std::remove(path.c_str());
    }
}

void testLongRandomizedRebalance() {
    const std::string path = testPath("long_random_rebalance");
    BPT* tree = createTree(path);
    std::vector<Entry> model;
    char key[kKeySize];
    unsigned state = 0x7f4a7c15U;
    unsigned ridNumber = 1;

    auto randomValue = [&state]() {
        state = state * 1664525U + 1013904223U;
        return static_cast<int>(state % 129) - 64;
    };
    auto randomIndex = [&state](std::size_t size) {
        state = state * 1664525U + 1013904223U;
        return static_cast<std::size_t>(state % size);
    };

    for (int step = 0; step < 1200; ++step) {
        const bool insert = model.empty() || randomValue() % 100 < 55;
        if (insert) {
            const Entry entry{randomValue(), RID(23, ridNumber++)};
            writeKey(entry.first, key);
            require(tree->InsertEntry(key, entry.second) == 0,
                    "long randomized insertion failed");
            model.push_back(entry);
        } else {
            const std::size_t selected = randomIndex(model.size());
            const Entry entry = model[selected];
            writeKey(entry.first, key);
            require(tree->DeleteEntry(key, entry.second) == 0,
                    "long randomized deletion failed");
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(selected));
        }

        const int depth = validateTree(*tree);
        require(depth >= 1, "randomized tree lost its root");
        if (step % 13 == 0) checkFullScan(*tree, model);
    }

    checkFullScan(*tree, model);
    checkExactEntries(*tree, model);
    delete tree;
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
            std::exit(0);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            std::cerr.flush();
            std::exit(1);
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
        {"empty and single entry", testEmptyAndSingleEntry},
        {"duplicate and exact duplicate", testDuplicateAndExactDuplicate},
        {"exact duplicate at search boundaries", testExactDuplicateAtSearchBoundaries},
        {"unique index", testUniqueIndex},
        {"NULL keys across leaves and persistence",
         testNullKeysAcrossLeavesAndPersistence},
        {"unique index allows NULL keys", testUniqueAllowsNullKeys},
        {"composite unique NULL semantics", testCompositeUniqueNullSemantics},
        {"NULL bitmap across byte boundary", testNullBitmapAcrossByteBoundary},
        {"FLOAT key ordering and ranges", testFloatKeyOrderingAndRanges},
        {"VARCHAR key ordering and ranges", testVarcharKeyOrderingAndRanges},
        {"odd node capacities", testOddNodeCapacities},
        {"minimum leaf capacity", testMinimumLeafCapacity},
        {"minimum internal capacity", testMinimumInternalCapacity},
        {"minimum leaf and internal capacity",
         testMinimumLeafAndInternalCapacity},
        {"multi-level splits and persistence", testMultiLevelSplitsAndPersistence},
        {"free-list persistence and reuse", testFreeListPersistenceAndReuse},
        {"range boundaries", testRangeBoundaries},
        {"duplicate range boundaries", testDuplicateRangeBoundaries},
        {"many duplicate keys across leaves", testManyDuplicatesAcrossLeaves},
        {"delete and rebalance", testDeleteAndRebalance},
        {"deterministic mixed operations", testDeterministicMixedOperations},
        {"four-level boundary operations", testFourLevelBoundaryOperations},
        {"split insertion orders", testSplitInsertionOrders},
        {"long randomized rebalance", testLongRandomizedRebalance},
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
