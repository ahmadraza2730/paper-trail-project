# PaperTrail: Work Split

**Team:** Hassaan, Ahmad, Huzaima
**Deadline:** ~7 December

The work is split evenly by effort (amount and difficulty together). Each person has one hard core part plus a share of the easier parts.

## Day 1 (all three)
- **Set up the project together and agree on how the pieces will connect:** so all three can start working at the same time.

---

## Hassaan (~33%, Nov 21–30)
**Why Hassaan:** the basic building blocks everyone needs, so they come first, plus the search index as his hard part. All of it can be finished without waiting on anyone.

| Part | What it is | Why it's his |
|---|---|---|
| Basic building blocks | The simple data structures (lists, queues, tables, etc.) everyone uses | Everyone needs these first |
| Memory tracker | Checks that the program doesn't leak memory | Needed by everyone from the start |
| Console | Reads the commands you type and prints errors | Everything runs through it |
| Folders and files | Creating folders, moving around, listing files | Needed early so documents have a place to live |
| Recycle bin | Deleting and restoring files | Part of folders and files |
| Bloom filter + experiment | A quick "is this word here?" checker, plus the test for the report | Small and separate; report part can be done early |
| Blocked words | Warning when someone types a banned word | Uses his Bloom filter |
| Search index (B+ tree) | Stores every word so search is fast | His hard part; completely separate, needs no one else's code |

---

## Ahmad (~33%)
**Why Ahmad:** search, links between documents and history all involve walking through connected data, so they fit together. He also sets up the testing tools, since he'll be there for the final testing.

| Part | What it is | Why it's his |
|---|---|---|
| Search | Finding words across all documents | Uses Hassaan's search index |
| Links | Which documents point to which | His hard part; connected-data work |
| Embeds | One document showing inside another, without loops | Same idea as links |
| History | Viewing old versions and finding where two versions split | Same kind of work as links |
| Similarity check | Finding documents that copy each other | Fits with search; uses the shared building blocks |
| Test scripts | Turning the PDF examples into tests | Used in the final testing he'll be part of |
| Testing tools setup | The base for the error checker and random tester | Same reason |
| His tests | Checks for his own parts | He knows them best |
| Final joining (half) | Putting everything together, fixing bugs | Shared at the end |

---

## Huzaima (~33%)
**Why Huzaima:** document text, edits, saving and merging are one connected chain, so one person keeps it consistent.

| Part | What it is | Why it's his |
|---|---|---|
| Document text | How a document's lines are stored | Big, separate, can start on day 1 |
| Edits | Adding, changing and deleting lines | Works directly on document text |
| Save and undo | Saving changes as a version, or throwing them away | Built from his edits |
| Version history | Keeping track of all saved versions | Saving creates versions |
| Fork | Copying a document into a separate branch | Copies his document text |
| Merge | Combining two branches and handling clashes | His hard part; needs all his pieces |
| His tests | Checks for his own parts | He knows them best |
| Final joining (half) | Putting everything together, fixing bugs | Shared at the end |

---

## Rules
- Everyone starts on day 1.
- Upload each piece as soon as it works.
- Someone else checks each piece before it's added.
- Anything Hassaan doesn't finish by 30 Nov goes back to the team.
