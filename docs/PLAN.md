# PaperTrail: Work Split

**Team:** Hassaan, Ahmad, Huzaima
**Deadline:** ~7 December

## Day 1 (all three)
- **Set up the project together and agree on how the pieces will connect:** so all three can start working at the same time.

---

## Hassaan (~35%, Nov 21–30)
**Why Hassaan:** these are the basic building blocks everyone else needs, so they have to be done first. He finishes early, and nobody is left waiting on him.

| Part | What it is | Why it's his |
|---|---|---|
| Basic building blocks | The simple data structures (lists, queues, tables, etc.) everyone uses | Everyone needs these first |
| Memory tracker | Checks that the program doesn't leak memory | Needed by everyone from the start |
| Console | Reads the commands you type and prints errors | Everything runs through it |
| Folders and files | Creating folders, moving around, listing files | Needed early so documents have a place to live |
| Recycle bin | Deleting and restoring files | Part of folders and files |
| Bloom filter + experiment | A quick "is this word here?" checker, plus the test for the report | Small and separate; report part can be done early |
| Blocked words | Warning when someone types a banned word | Uses his Bloom filter |
| Similarity check | Finding documents that copy each other | Uses only his own pieces |
| Test scripts | Turning the PDF examples into tests | Gives the team ready-made tests for the end |
| Testing tools setup | The base for the error checker and random tester | Others just add their own checks |

---

## Ahmad (~32%)
**Why Ahmad:** search, links between documents and history all involve walking through connected data, so they fit together.

| Part | What it is | Why it's his |
|---|---|---|
| Search index | Stores every word so search is fast | Big, separate, can start on day 1 |
| Search | Finding words across all documents | Built on his index |
| Links | Which documents point to which | Connected-data work |
| Embeds | One document showing inside another, without loops | Same idea as links |
| History | Viewing old versions and finding where two versions split | Same kind of work as links |
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
| Merge | Combining two branches and handling clashes | The hardest part; needs all his pieces |
| His tests | Checks for his own parts | He knows them best |
| Final joining (half) | Putting everything together, fixing bugs | Shared at the end |

---

## Rules
- Everyone starts on day 1.
- Upload each piece as soon as it works.
- Someone else checks each piece before it's added.
- Anything Hassaan doesn't finish by 30 Nov goes back to the team.
