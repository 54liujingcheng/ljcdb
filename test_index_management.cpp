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
    meta.offset[0] = 0;
    meta.offset[1] = static_cast<int>(sizeof(int));
    return meta;
}

void writeKey(int value, char* key) {
    std::memcpy(key, &value, sizeof(value));
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

Entry readEntry(const char* entry, int keylen) {
    int key = 0;
    ull rid = 0;
    std::memcpy(&key, entry, sizeof(key));
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
        result.entries.push_back(readEntry(data + sid * stride, tree.keylen));
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
    return rootSummary.depth;
}

void checkExactEntries(BPT& tree, const std::vector<Entry>& entries) {
    char key[sizeof(int)];
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
    char key[sizeof(int)];
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
    char lowerKey[sizeof(int)];
    char upperKey[sizeof(int)];
    writeKey(lower, lowerKey);
    writeKey(upper, upperKey);
    const bool combinations[][2] = {
        {true, true}, {true, false}, {false, true}, {false, false}};
    for (const auto& combination : combinations) {
        SearchIter iter = tree.SearchRange(lowerKey, combination[0]);
        std::vector<ull> actual;
        for (std::size_t count = 0; count <= model.size(); ++count) {
            ull rid = 0;
            if (!tree.Next(iter, upperKey, rid, combination[1])) break;
            actual.push_back(rid);
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

void checkFullScan(BPT& tree, const std::vector<Entry>& inserted) {
    std::vector<Entry> expected = inserted;
    std::sort(expected.begin(), expected.end());

    char lower[sizeof(int)];
    writeKey(std::numeric_limits<int>::min(), lower);
    SearchIter iter = tree.SearchRange(lower, true);
    std::vector<Entry> actual;
    actual.reserve(expected.size());

    char* noUpper = nullptr;
    for (std::size_t n = 0; n <= expected.size(); ++n) {
        ull rid = 0;
        if (!tree.Next(iter, noUpper, rid)) {
            break;
        }
        auto found = std::find_if(
            inserted.begin(), inserted.end(),
            [rid](const Entry& entry) { return entry.second == rid; });
        require(found != inserted.end(), "scan returned an unknown RID");
        actual.push_back(*found);
    }

    require(actual == expected,
            "full leaf scan did not return entries in key/RID order");

    ull extraRid = 0;
    require(!tree.Next(iter, noUpper, extraRid),
            "full leaf scan returned more entries than were inserted");
}

void testEmptyAndSingleEntry() {
    const std::string path = testPath("empty_single");
    BPT* tree = createTree(path);

    char key[sizeof(int)];
    writeKey(7, key);
    SearchIter empty = tree->SearchRange(key, true);
    ull rid = 0;
    require(!tree->Next(empty, nullptr, rid),
            "empty tree produced a range result");

    const ull insertedRid = RID(9, 1);
    require(tree->InsertEntry(key, insertedRid) == 0,
            "first insertion failed");
    require(tree->InsertEntry(key, insertedRid) == -1,
            "exact duplicate was accepted");

    SearchIter iter = tree->SearchRange(key, true);
    require(tree->Next(iter, key, rid), "single entry was not found");
    require(rid == insertedRid, "single entry returned the wrong RID");
    require(!tree->Next(iter, key, rid),
            "single-entry range returned an extra result");

    delete tree;
    std::remove(path.c_str());
}

void testDuplicateAndExactDuplicate() {
    const std::string path = testPath("duplicates");
    BPT* tree = createTree(path);
    std::vector<Entry> inserted;
    char key[sizeof(int)];

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
    char key[sizeof(int)];

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
    char first[sizeof(int)];
    char second[sizeof(int)];
    writeKey(42, first);
    writeKey(42, second);

    require(tree->InsertEntry(first, RID(12, 1)) == 0,
            "unique index first insertion failed");
    require(tree->InsertEntry(second, RID(12, 2)) == -2,
            "unique index accepted a duplicate key");

    delete tree;
    std::remove(path.c_str());
}

void testMultiLevelSplitsAndPersistence() {
    const std::string path = testPath("multilevel");
    std::vector<Entry> inserted;
    {
        BPT* tree = createTree(path);
        char key[sizeof(int)];

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

std::vector<ull> scanRange(BPT& tree, int lower, bool lowerInclusive,
                           int upper, bool upperInclusive) {
    char lowerKey[sizeof(int)];
    char upperKey[sizeof(int)];
    writeKey(lower, lowerKey);
    writeKey(upper, upperKey);
    SearchIter iter = tree.SearchRange(lowerKey, lowerInclusive);
    std::vector<ull> result;
    for (;;) {
        ull rid = 0;
        if (!tree.Next(iter, upperKey, rid, upperInclusive)) {
            break;
        }
        result.push_back(rid);
        require(result.size() <= 100,
                "range scan did not make progress or did not terminate");
    }
    return result;
}

void testRangeBoundaries() {
    const std::string path = testPath("ranges");
    BPT* tree = createTree(path);
    char key[sizeof(int)];
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
    char key[sizeof(int)];
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
    char key[sizeof(int)];

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
    char lower[sizeof(int)];
    char upper[sizeof(int)];
    writeKey(42, lower);
    writeKey(42, upper);
    SearchIter iter = tree->SearchRange(lower, true);
    std::vector<ull> actual;
    for (;;) {
        ull rid = 0;
        if (!tree->Next(iter, upper, rid, true)) break;
        actual.push_back(rid);
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
    char key[sizeof(int)];

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
    char key[sizeof(int)];
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
    char key[sizeof(int)];
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
        char key[sizeof(int)];
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
    char key[sizeof(int)];
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
        {"empty and single entry", testEmptyAndSingleEntry},
        {"duplicate and exact duplicate", testDuplicateAndExactDuplicate},
        {"exact duplicate at search boundaries", testExactDuplicateAtSearchBoundaries},
        {"unique index", testUniqueIndex},
        {"multi-level splits and persistence", testMultiLevelSplitsAndPersistence},
        {"range boundaries", testRangeBoundaries},
        {"duplicate range boundaries", testDuplicateRangeBoundaries},
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
