# PaperTrail

CS2001 Data Structures, Fall 2026 final project. A terminal document explorer with version control, written in C++ without the STL.

Team: **Hassaan**, **Ahmad**, **Huzaima**. See [docs/PLAN.md](docs/PLAN.md) for the work split and the reasons behind it.

## Build and run

```
make                     # builds ./papertrail (warnings are errors)
./papertrail             # interactive
./papertrail script.txt  # run a script, then exit with the allocation report
make clean
```

## Who works where

Each person works inside their own folders. Files marked **shared** need a heads-up to the team before changing.

| Folder / file | Owner | What's in it |
|---|---|---|
| `src/foundation/` | Hassaan | Dynamic array, ring queue, stack, hash map, min-heap, sort, hashes, tokenizer, allocation tracker |
| `src/console/` | Hassaan | Command reading, parsing, scripts, error messages |
| `src/explorer/` | Hassaan | Folders, files, `ls`, stars, `props`, recycle bin |
| `src/filters/` | Hassaan | Bloom filters, `fptest`, blocklist, `similar` |
| `src/testing/` | Hassaan (framework) + everyone (own checks) | Invariant checker, fuzzer |
| `src/main.cpp` | Hassaan | Entry point |
| `src/document/` | Huzaima | AVL tree, document, edits, staging, save, discard |
| `src/history/CommitDag.*` | Huzaima | Commits, DAG, reference counting |
| `src/history/Merge.*` | Huzaima | `fork`, `merge` |
| `src/history/Queries.*` | Ahmad | `log`, `view`, `common` (LCA) |
| `src/search/` | Ahmad | B+ tree index, `search` |
| `src/graph/` | Ahmad | Link and embed graphs |
| `src/common/Types.h` | **shared** | IDs and the `Op` struct |
| `src/engine/Engine.*` | **shared** | Global state and the hooks between parts |
| `tests/` | everyone | Test scripts and expected output |

The headers contain **proposed** function signatures. Agree on them together on day 1, then change them freely inside your own folders.

## How we work

1. Pull before you start: `git pull`
2. Work on your own branch: `git checkout -b hassaan/hashmap` (use `yourname/what`)
3. Make sure `make` passes with no warnings before pushing.
4. Push and open a pull request. Someone else reviews it before it's merged into `main`.
5. Never push broken code to `main`.

## Rules from the spec

- No STL containers, algorithms or smart pointers. Only `std::string` and `iostream`.
- Free everything you allocate; the allocation tracker reports leaks.
- Output must match the PDF transcripts and error wording exactly.
