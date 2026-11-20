// "similar <k> [percent]": shingles, Bloom estimate vs exact, top-k with a min-heap (Section 5.10).
// Owner: Ahmad
#pragma once

#include "engine/Engine.h"

void runSimilar(Engine& engine, int k, int thresholdPercent);
