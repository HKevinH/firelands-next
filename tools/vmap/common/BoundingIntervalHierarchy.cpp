#include "BoundingIntervalHierarchy.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace Firelands::VMap {

// ─── InitEmpty ────────────────────────────────────────────────────────────────

void BIH::InitEmpty() {
    _tree.clear();
    _objects.clear();
    _tree.push_back(3u << 30u); // dummy leaf
    _tree.push_back(0);
    _tree.push_back(0);
}

// ─── CreateNode ───────────────────────────────────────────────────────────────

void BIH::CreateNode(std::vector<uint32_t>& tree, int idx,
                     uint32_t lo, uint32_t hi) const
{
    tree[idx + 0] = (3u << 30u) | lo;
    tree[idx + 1] = hi - lo + 1;
}

// ─── BuildHierarchy ───────────────────────────────────────────────────────────

void BIH::BuildHierarchy(std::vector<uint32_t>& tree,
                         BuildData& dat, BuildStats& stats)
{
    tree.push_back(3u << 30u);
    tree.push_back(0);
    tree.push_back(0);

    AABound gridBox = {_bounds.lo, _bounds.hi};
    AABound nodeBox = gridBox;
    Subdivide(0, static_cast<int>(dat.numPrims) - 1,
              tree, dat, gridBox, nodeBox, 0, 1, stats);
}

// ─── Subdivide ────────────────────────────────────────────────────────────────
// Direct port of BIH::subdivide from the reference.

void BIH::Subdivide(int left, int right,
                    std::vector<uint32_t>& tree, BuildData& dat,
                    AABound& gridBox, AABound& nodeBox,
                    int nodeIndex, int depth, BuildStats& stats)
{
    if ((right - left + 1) <= dat.maxPrims || depth >= FL_BIH_MAX_STACK) {
        stats.UpdateLeaf(depth, right - left + 1);
        CreateNode(tree, nodeIndex, static_cast<uint32_t>(left),
                   static_cast<uint32_t>(right));
        return;
    }

    int    axis = -1, prevAxis = -1, rightOrig = right;
    float  clipL = FNan(), clipR = FNan(), prevClip = FNan();
    float  split = FNan(), prevSplit = FNan();
    bool   wasLeft = true;

    while (true) {
        prevAxis  = axis;
        prevSplit = split;

        Vec3 d = {gridBox.hi.x - gridBox.lo.x,
                  gridBox.hi.y - gridBox.lo.y,
                  gridBox.hi.z - gridBox.lo.z};
        if (d.x < 0 || d.y < 0 || d.z < 0)
            throw std::logic_error("BIH: negative node extents");
        for (int i = 0; i < 3; ++i) {
            if (nodeBox.hi[i] < gridBox.lo[i] || nodeBox.lo[i] > gridBox.hi[i])
                throw std::logic_error("BIH: invalid node overlap");
        }

        axis  = d.primaryAxis();
        split = 0.5f * (gridBox.lo[axis] + gridBox.hi[axis]);

        clipL    = -FInf();
        clipR    =  FInf();
        rightOrig = right;
        float nodeL =  FInf();
        float nodeR = -FInf();

        for (int i = left; i <= right; ) {
            int   obj  = static_cast<int>(dat.indices[i]);
            float minb = dat.primBound[obj].lo[axis];
            float maxb = dat.primBound[obj].hi[axis];
            float ctr  = (minb + maxb) * 0.5f;
            if (ctr <= split) {
                ++i;
                if (clipL < maxb) clipL = maxb;
            } else {
                std::swap(dat.indices[i], dat.indices[right]);
                --right;
                if (clipR > minb) clipR = minb;
            }
            nodeL = std::min(nodeL, minb);
            nodeR = std::max(nodeR, maxb);
        }

        // check for empty space
        if (nodeL > nodeBox.lo[axis] && nodeR < nodeBox.hi[axis]) {
            float nodeBoxW = nodeBox.hi[axis] - nodeBox.lo[axis];
            float nodeNewW = nodeR - nodeL;
            if (1.3f * nodeNewW < nodeBoxW) {
                stats.UpdateBVH2();
                int nextIdx = static_cast<int>(tree.size());
                tree.push_back(0); tree.push_back(0); tree.push_back(0);
                stats.UpdateInner();
                tree[nodeIndex + 0] = (static_cast<uint32_t>(axis) << 30u) |
                                      (1u << 29u) |
                                      static_cast<uint32_t>(nextIdx);
                tree[nodeIndex + 1] = FloatToRawBits(nodeL);
                tree[nodeIndex + 2] = FloatToRawBits(nodeR);
                nodeBox.lo[axis] = nodeL;
                nodeBox.hi[axis] = nodeR;
                Subdivide(left, rightOrig, tree, dat, gridBox, nodeBox,
                          nextIdx, depth + 1, stats);
                return;
            }
        }

        if (right == rightOrig) {
            // all left
            if (prevAxis == axis && Vec3::fuzzyEq(prevSplit, split)) {
                stats.UpdateLeaf(depth, right - left + 1);
                CreateNode(tree, nodeIndex,
                           static_cast<uint32_t>(left), static_cast<uint32_t>(right));
                return;
            }
            if (clipL <= split) {
                gridBox.hi[axis] = split;
                prevClip = clipL;
                wasLeft  = true;
                continue;
            }
            gridBox.hi[axis] = split;
            prevClip = FNan();
        } else if (left > right) {
            // all right
            right = rightOrig;
            if (prevAxis == axis && Vec3::fuzzyEq(prevSplit, split)) {
                stats.UpdateLeaf(depth, right - left + 1);
                CreateNode(tree, nodeIndex,
                           static_cast<uint32_t>(left), static_cast<uint32_t>(right));
                return;
            }
            if (clipR >= split) {
                gridBox.lo[axis] = split;
                prevClip = clipR;
                wasLeft  = false;
                continue;
            }
            gridBox.lo[axis] = split;
            prevClip = FNan();
        } else {
            // actually splitting
            if (prevAxis != -1 && !std::isnan(prevClip)) {
                int nextIdx = static_cast<int>(tree.size());
                tree.push_back(0); tree.push_back(0); tree.push_back(0);
                if (wasLeft) {
                    stats.UpdateInner();
                    tree[nodeIndex + 0] = (static_cast<uint32_t>(prevAxis) << 30u) |
                                          static_cast<uint32_t>(nextIdx);
                    tree[nodeIndex + 1] = FloatToRawBits(prevClip);
                    tree[nodeIndex + 2] = FloatToRawBits(FInf());
                } else {
                    stats.UpdateInner();
                    tree[nodeIndex + 0] = (static_cast<uint32_t>(prevAxis) << 30u) |
                                          static_cast<uint32_t>(nextIdx - 3);
                    tree[nodeIndex + 1] = FloatToRawBits(-FInf());
                    tree[nodeIndex + 2] = FloatToRawBits(prevClip);
                }
                ++depth;
                stats.UpdateLeaf(depth, 0);
                nodeIndex = nextIdx;
            }
            break;
        }
    }

    int nextIdx = static_cast<int>(tree.size());
    int nl = right - left + 1;
    int nr = rightOrig - (right + 1) + 1;
    if (nl > 0) {
        tree.push_back(0); tree.push_back(0); tree.push_back(0);
    } else {
        nextIdx -= 3;
    }
    if (nr > 0) {
        tree.push_back(0); tree.push_back(0); tree.push_back(0);
    }
    stats.UpdateInner();
    tree[nodeIndex + 0] = (static_cast<uint32_t>(axis) << 30u) |
                          static_cast<uint32_t>(nextIdx);
    tree[nodeIndex + 1] = FloatToRawBits(clipL);
    tree[nodeIndex + 2] = FloatToRawBits(clipR);

    AABound gridBoxL(gridBox), gridBoxR(gridBox);
    AABound nodeBoxL(nodeBox), nodeBoxR(nodeBox);
    gridBoxL.hi[axis] = gridBoxR.lo[axis] = split;
    nodeBoxL.hi[axis] = clipL;
    nodeBoxR.lo[axis] = clipR;

    if (nl > 0)
        Subdivide(left, right, tree, dat, gridBoxL, nodeBoxL,
                  nextIdx, depth + 1, stats);
    else
        stats.UpdateLeaf(depth + 1, 0);

    if (nr > 0)
        Subdivide(right + 1, rightOrig, tree, dat, gridBoxR, nodeBoxR,
                  nextIdx + 3, depth + 1, stats);
    else
        stats.UpdateLeaf(depth + 1, 0);
}

// ─── WriteToFile ──────────────────────────────────────────────────────────────
// Binary layout matches the reference exactly:
//   float[3]  bounds.lo
//   float[3]  bounds.hi
//   uint32    treeSize
//   uint32[treeSize]
//   uint32    objectCount
//   uint32[objectCount]

bool BIH::WriteToFile(FILE* wf) const {
    uint32_t treeSize = static_cast<uint32_t>(_tree.size());
    uint32_t check = 0;
    check += static_cast<uint32_t>(std::fwrite(&_bounds.lo.x, sizeof(float), 3, wf));
    check += static_cast<uint32_t>(std::fwrite(&_bounds.hi.x, sizeof(float), 3, wf));
    check += static_cast<uint32_t>(std::fwrite(&treeSize, sizeof(uint32_t), 1, wf));
    if (!_tree.empty())
        check += static_cast<uint32_t>(std::fwrite(_tree.data(), sizeof(uint32_t), treeSize, wf));
    uint32_t count = static_cast<uint32_t>(_objects.size());
    check += static_cast<uint32_t>(std::fwrite(&count, sizeof(uint32_t), 1, wf));
    if (!_objects.empty())
        check += static_cast<uint32_t>(std::fwrite(_objects.data(), sizeof(uint32_t), count, wf));
    return check == (3 + 3 + 2 + treeSize + count);
}

// ─── ReadFromFile ─────────────────────────────────────────────────────────────

bool BIH::ReadFromFile(FILE* rf) {
    uint32_t check = 0;
    check += static_cast<uint32_t>(std::fread(&_bounds.lo.x, sizeof(float), 3, rf));
    check += static_cast<uint32_t>(std::fread(&_bounds.hi.x, sizeof(float), 3, rf));

    uint32_t treeSize = 0;
    check += static_cast<uint32_t>(std::fread(&treeSize, sizeof(uint32_t), 1, rf));
    _tree.resize(treeSize);
    if (treeSize)
        check += static_cast<uint32_t>(std::fread(_tree.data(), sizeof(uint32_t), treeSize, rf));

    uint32_t count = 0;
    check += static_cast<uint32_t>(std::fread(&count, sizeof(uint32_t), 1, rf));
    _objects.resize(count);
    if (count)
        check += static_cast<uint32_t>(std::fread(_objects.data(), sizeof(uint32_t), count, rf));

    return static_cast<uint64_t>(check) ==
           static_cast<uint64_t>(3 + 3 + 1 + 1) +
           static_cast<uint64_t>(treeSize) +
           static_cast<uint64_t>(count);
}

// ─── BuildStats ───────────────────────────────────────────────────────────────

void BIH::BuildStats::UpdateLeaf(int depth, int n) {
    ++numLeaves;
    minDepth  = std::min(depth, minDepth);
    maxDepth  = std::max(depth, maxDepth);
    sumDepth += depth;
    minObjects = std::min(n, minObjects);
    maxObjects = std::max(n, maxObjects);
    sumObjects += n;
    numLeavesN[std::min(n, 5)]++;
}

void BIH::BuildStats::Print() const {
    std::printf("BIH stats:\n");
    std::printf("  Nodes:  %d\n", numNodes);
    std::printf("  Leaves: %d\n", numLeaves);
    if (numLeaves > 0) {
        std::printf("  Objects: min=%d avg=%.2f max=%d\n",
                    minObjects, (float)sumObjects / numLeaves, maxObjects);
        std::printf("  Depth:   min=%d avg=%.2f max=%d\n",
                    minDepth, (float)sumDepth / numLeaves, maxDepth);
        std::printf("  BVH2 nodes: %d\n", numBVH2);
    }
}

} // namespace Firelands::VMap
