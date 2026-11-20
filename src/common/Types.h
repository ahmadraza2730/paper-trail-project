// Shared IDs and the Operation type used by everyone.
// Owner: shared. Change only after telling the team.
#pragma once

#include <cstdint>
#include <string>

typedef uint32_t FileID;    // F1, F2, ...  (0 = none)
typedef uint64_t LineID;    // L1, L2, ...  (0 = none / "top" as an anchor)
typedef uint32_t CommitID;  // c1, c2, ...  (0 = none)
typedef uint64_t Tick;      // value of now()

enum OpType { OP_INSERT, OP_DELETE, OP_EDIT };

// One edit operation (Section 5.3.3). Ops refer to LineIDs, never line numbers.
//   InsertLine(line, anchor, newText, author)
//   DeleteLine(line, anchor = prevLineID, oldText, oldAuthor)
//   EditLine  (line, oldText, newText, oldAuthor, author)
struct Op {
    OpType      type;
    LineID      line;
    LineID      anchor;
    std::string oldText;
    std::string newText;
    std::string oldAuthor;
    std::string author;
};
