// Hook implementations. Each owner fills in their section.
#include "engine/Engine.h"

void onDocumentCreated(Engine& e, Document* doc) {
    (void)e; (void)doc;
    // TODO(Ahmad): resolve unresolved links/embeds that now point at doc.
}

void onDocumentRemoved(Engine& e, Document* doc) {
    (void)e; (void)doc;
    // TODO(Ahmad): remove doc from link and embed graphs.
}

void onDocumentRestored(Engine& e, Document* doc) {
    (void)e; (void)doc;
    // TODO(Ahmad): re-scan saved text, re-add edges.
}

void onDocumentFreed(Engine& e, Document* doc) {
    (void)e; (void)doc;
    // TODO(Ahmad): purge index postings.
    // TODO(Huzaima): drop head reference, cascade-free commits, free AVL tree.
}

void onOpsSaved(Engine& e, Document* doc, const Op* ops, int count) {
    (void)e; (void)doc; (void)ops; (void)count;
    // TODO(Ahmad): update index, link graph, embed graph from the saved ops.
}
