/** @file MaxRectsBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the MAXRECTS data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <iostream>
#include <limits>
#include <utility>

#include <cassert>
#include <cmath>
#include <cstring>

#include "MaxRectsBinPack.h"

namespace rbp
{
	using namespace std;

	MaxRectsBinPack::MaxRectsBinPack()
		: binWidth(0),
		  binHeight(0) {}

	MaxRectsBinPack::MaxRectsBinPack(const int width, const int height)
	{
		init(width, height);
	}

	void MaxRectsBinPack::init(const int width, const int height)
	{
		binWidth  = width;
		binHeight = height;

		Rect n{};
		n.x      = 0;
		n.y      = 0;
		n.width  = width;
		n.height = height;

		usedRectangles.clear();

		freeRectangles.clear();
		freeRectangles.push_back(n);
	}

	Rect MaxRectsBinPack::insert(const int width, const int height, const FreeRectChoiceHeuristic method)
	{
		Rect newNode = {};
		// Unused in this function. We don't need to know the score after finding the position.
		int score1 = std::numeric_limits<int>::max();
		int score2 = std::numeric_limits<int>::max();
		switch (method) {
			case RectBestShortSideFit: newNode = findPositionForNewNodeBestShortSideFit(width, height, score1, score2);
				break;
			case RectBottomLeftRule: newNode = findPositionForNewNodeBottomLeft(width, height, score1, score2);
				break;
			case RectContactPointRule: newNode = findPositionForNewNodeContactPoint(width, height, score1);
				break;
			case RectBestLongSideFit: newNode = findPositionForNewNodeBestLongSideFit(width, height, score2, score1);
				break;
			case RectBestAreaFit: newNode = findPositionForNewNodeBestAreaFit(width, height, score1, score2);
				break;
		}

		if (newNode.height == 0) return newNode;

		size_t numRectanglesToProcess = freeRectangles.size();
		for (size_t i = 0; i < numRectanglesToProcess; ++i) {
			if (splitFreeNode(freeRectangles[i], newNode)) {
				freeRectangles.erase(freeRectangles.begin() + static_cast<int>(i));
				--i;
				--numRectanglesToProcess;
			}
		}

		pruneFreeList();

		usedRectangles.push_back(newNode);
		return newNode;
	}

	void MaxRectsBinPack::insert(std::vector<RectSize>& rects, std::vector<Rect>& dst, const FreeRectChoiceHeuristic method)
	{
		dst.clear();

		while (!rects.empty()) {
			int  bestScore1    = std::numeric_limits<int>::max();
			int  bestScore2    = std::numeric_limits<int>::max();
			int  bestRectIndex = -1;
			Rect bestNode{};

			for (size_t i = 0; i < rects.size(); ++i) {
				int        score1;
				int        score2;
				const Rect newNode = scoreRect(rects[i].width, rects[i].height, method, score1, score2);

				if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2)) {
					bestScore1    = score1;
					bestScore2    = score2;
					bestNode      = newNode;
					bestRectIndex = static_cast<int>(i);
				}
			}

			if (bestRectIndex == -1) return;

			placeRect(bestNode);
			rects.erase(rects.begin() + bestRectIndex);
		}
	}

	void MaxRectsBinPack::placeRect(const Rect& node)
	{
		size_t numRectanglesToProcess = freeRectangles.size();
		for (size_t i = 0; i < numRectanglesToProcess; ++i) {
			if (splitFreeNode(freeRectangles[i], node)) {
				freeRectangles.erase(freeRectangles.begin() + static_cast<int>(i));
				--i;
				--numRectanglesToProcess;
			}
		}

		pruneFreeList();

		usedRectangles.push_back(node);
		//		dst.push_back(bestNode); ///\todo Refactor so that this compiles.
	}

	Rect MaxRectsBinPack::scoreRect(const int width, const int height, const FreeRectChoiceHeuristic method, int& score1, int& score2) const
	{
		Rect newNode = {};
		score1       = std::numeric_limits<int>::max();
		score2       = std::numeric_limits<int>::max();
		switch (method) {
			case RectBestShortSideFit: newNode = findPositionForNewNodeBestShortSideFit(width, height, score1, score2);
				break;
			case RectBottomLeftRule: newNode = findPositionForNewNodeBottomLeft(width, height, score1, score2);
				break;
			case RectContactPointRule: newNode = findPositionForNewNodeContactPoint(width, height, score1);
				score1 = -score1; // Reverse since we are minimizing, but for contact point score bigger is better.
				break;
			case RectBestLongSideFit: newNode = findPositionForNewNodeBestLongSideFit(width, height, score2, score1);
				break;
			case RectBestAreaFit: newNode = findPositionForNewNodeBestAreaFit(width, height, score1, score2);
				break;
		}

		// Cannot fit the current rectangle.
		if (newNode.height == 0) {
			score1 = std::numeric_limits<int>::max();
			score2 = std::numeric_limits<int>::max();
		}

		return newNode;
	}

	/// Computes the ratio of used surface area.
	float MaxRectsBinPack::occupancy() const
	{
		unsigned long usedSurfaceArea = 0;
		for (const auto usedRect : usedRectangles) usedSurfaceArea += usedRect.width * usedRect.height;

		return static_cast<float>(usedSurfaceArea) / static_cast<float>(binWidth * binHeight);
	}

	Rect MaxRectsBinPack::findPositionForNewNodeBottomLeft(const int width, const int height, int& bestY, int& bestX) const
	{
		Rect bestNode = {};

		bestY = std::numeric_limits<int>::max();
		bestX = std::numeric_limits<int>::max();

		for (const auto freeRect : freeRectangles) {
			// Try to place the rectangle in upright (non-flipped) orientation.
			if (freeRect.width >= width && freeRect.height >= height) {
				const int topSideY = freeRect.y + height;
				if (topSideY < bestY || (topSideY == bestY && freeRect.x < bestX)) {
					bestNode.x      = freeRect.x;
					bestNode.y      = freeRect.y;
					bestNode.width  = width;
					bestNode.height = height;
					bestY           = topSideY;
					bestX           = freeRect.x;
				}
			}
			if (freeRect.width >= height && freeRect.height >= width) {
				const int topSideY = freeRect.y + width;
				if (topSideY < bestY || (topSideY == bestY && freeRect.x < bestX)) {
					bestNode.x      = freeRect.x;
					bestNode.y      = freeRect.y;
					bestNode.width  = height;
					bestNode.height = width;
					bestY           = topSideY;
					bestX           = freeRect.x;
				}
			}
		}
		return bestNode;
	}

	Rect MaxRectsBinPack::findPositionForNewNodeBestShortSideFit(const int width,
																 const int height,
																 int&      bestShortSideFit,
																 int&      bestLongSideFit) const
	{
		Rect bestNode = {};

		bestShortSideFit = std::numeric_limits<int>::max();
		bestLongSideFit  = std::numeric_limits<int>::max();

		for (const auto freeRect : freeRectangles) {
			// Try to place the rectangle in upright (non-flipped) orientation.
			if (freeRect.width >= width && freeRect.height >= height) {
				int       leftoverHoriz = abs(freeRect.width - width);
				int       leftoverVert  = abs(freeRect.height - height);
				const int shortSideFit  = min(leftoverHoriz, leftoverVert);
				const int longSideFit   = max(leftoverHoriz, leftoverVert);

				if (shortSideFit < bestShortSideFit || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = width;
					bestNode.height  = height;
					bestShortSideFit = shortSideFit;
					bestLongSideFit  = longSideFit;
				}
			}

			if (freeRect.width >= height && freeRect.height >= width) {
				int       flippedLeftoverHoriz = abs(freeRect.width - height);
				int       flippedLeftoverVert  = abs(freeRect.height - width);
				const int flippedShortSideFit  = min(flippedLeftoverHoriz, flippedLeftoverVert);
				const int flippedLongSideFit   = max(flippedLeftoverHoriz, flippedLeftoverVert);

				if (flippedShortSideFit < bestShortSideFit || (flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = height;
					bestNode.height  = width;
					bestShortSideFit = flippedShortSideFit;
					bestLongSideFit  = flippedLongSideFit;
				}
			}
		}
		return bestNode;
	}

	Rect MaxRectsBinPack::findPositionForNewNodeBestLongSideFit(const int width,
																const int height,
																int&      bestShortSideFit,
																int&      bestLongSideFit) const
	{
		Rect bestNode{};

		bestShortSideFit = std::numeric_limits<int>::max();
		bestLongSideFit  = std::numeric_limits<int>::max();

		for (const auto freeRect : freeRectangles) {
			// Try to place the rectangle in upright (non-flipped) orientation.
			if (freeRect.width >= width && freeRect.height >= height) {
				int       leftoverHoriz = abs(freeRect.width - width);
				int       leftoverVert  = abs(freeRect.height - height);
				const int shortSideFit  = min(leftoverHoriz, leftoverVert);
				const int longSideFit   = max(leftoverHoriz, leftoverVert);

				if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = width;
					bestNode.height  = height;
					bestShortSideFit = shortSideFit;
					bestLongSideFit  = longSideFit;
				}
			}

			if (freeRect.width >= height && freeRect.height >= width) {
				int       leftoverHoriz = abs(freeRect.width - height);
				int       leftoverVert  = abs(freeRect.height - width);
				const int shortSideFit  = min(leftoverHoriz, leftoverVert);
				const int longSideFit   = max(leftoverHoriz, leftoverVert);

				if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = height;
					bestNode.height  = width;
					bestShortSideFit = shortSideFit;
					bestLongSideFit  = longSideFit;
				}
			}
		}
		return bestNode;
	}

	Rect MaxRectsBinPack::findPositionForNewNodeBestAreaFit(const int width,
															const int height,
															int&      bestAreaFit,
															int&      bestShortSideFit) const
	{
		Rect bestNode{};

		bestAreaFit      = std::numeric_limits<int>::max();
		bestShortSideFit = std::numeric_limits<int>::max();

		for (const auto freeRect : freeRectangles) {
			const int areaFit = freeRect.width * freeRect.height - width * height;

			// Try to place the rectangle in upright (non-flipped) orientation.
			if (freeRect.width >= width && freeRect.height >= height) {
				int       leftoverHoriz = abs(freeRect.width - width);
				int       leftoverVert  = abs(freeRect.height - height);
				const int shortSideFit  = min(leftoverHoriz, leftoverVert);

				if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = width;
					bestNode.height  = height;
					bestShortSideFit = shortSideFit;
					bestAreaFit      = areaFit;
				}
			}

			if (freeRect.width >= height && freeRect.height >= width) {
				int       leftoverHoriz = abs(freeRect.width - height);
				int       leftoverVert  = abs(freeRect.height - width);
				const int shortSideFit  = min(leftoverHoriz, leftoverVert);

				if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit)) {
					bestNode.x       = freeRect.x;
					bestNode.y       = freeRect.y;
					bestNode.width   = height;
					bestNode.height  = width;
					bestShortSideFit = shortSideFit;
					bestAreaFit      = areaFit;
				}
			}
		}
		return bestNode;
	}

	/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
	int commonIntervalLength(const int i1Start, const int i1End, const int i2Start, const int i2End)
	{
		if (i1End < i2Start || i2End < i1Start) return 0;
		return min(i1End, i2End) - max(i1Start, i2Start);
	}

	int MaxRectsBinPack::contactPointScoreNode(const int x, const int y, const int width, const int height) const
	{
		int score = 0;

		if (x == 0 || x + width == binWidth) score += height;
		if (y == 0 || y + height == binHeight) score += width;

		for (const auto usedRect : usedRectangles) {
			if (usedRect.x == x + width || usedRect.x + usedRect.width == x) score += commonIntervalLength(usedRect.y, usedRect.y + usedRect.height, y, y + height);
			if (usedRect.y == y + height || usedRect.y + usedRect.height == y) score += commonIntervalLength(usedRect.x, usedRect.x + usedRect.width, x, x + width);
		}
		return score;
	}

	Rect MaxRectsBinPack::findPositionForNewNodeContactPoint(const int width, const int height, int& contactScore) const
	{
		Rect bestNode{};

		contactScore = -1;

		for (const auto freeRect : freeRectangles) {
			// Try to place the rectangle in upright (non-flipped) orientation.
			if (freeRect.width >= width && freeRect.height >= height) {
				const int score = contactPointScoreNode(freeRect.x, freeRect.y, width, height);
				if (score > contactScore) {
					bestNode.x      = freeRect.x;
					bestNode.y      = freeRect.y;
					bestNode.width  = width;
					bestNode.height = height;
					contactScore    = score;
				}
			}
			if (freeRect.width >= height && freeRect.height >= width) {
				const int score = contactPointScoreNode(freeRect.x, freeRect.y, height, width);
				if (score > contactScore) {
					bestNode.x      = freeRect.x;
					bestNode.y      = freeRect.y;
					bestNode.width  = height;
					bestNode.height = width;
					contactScore    = score;
				}
			}
		}
		return bestNode;
	}

	bool MaxRectsBinPack::splitFreeNode(const Rect freeNode, const Rect& usedNode)
	{
		// Test with SAT if the rectangles even intersect.
		if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
			usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
			return false;

		if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x) {
			// New node at the top side of the used node.
			if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height) {
				Rect newNode   = freeNode;
				newNode.height = usedNode.y - newNode.y;
				freeRectangles.push_back(newNode);
			}

			// New node at the bottom side of the used node.
			if (usedNode.y + usedNode.height < freeNode.y + freeNode.height) {
				Rect newNode   = freeNode;
				newNode.y      = usedNode.y + usedNode.height;
				newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
				freeRectangles.push_back(newNode);
			}
		}

		if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y) {
			// New node at the left side of the used node.
			if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width) {
				Rect newNode  = freeNode;
				newNode.width = usedNode.x - newNode.x;
				freeRectangles.push_back(newNode);
			}

			// New node at the right side of the used node.
			if (usedNode.x + usedNode.width < freeNode.x + freeNode.width) {
				Rect newNode  = freeNode;
				newNode.x     = usedNode.x + usedNode.width;
				newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
				freeRectangles.push_back(newNode);
			}
		}

		return true;
	}

	void MaxRectsBinPack::pruneFreeList()
	{
		/* 
		///  Would be nice to do something like this, to avoid a Theta(n^2) loop through each pair.
		///  But unfortunately it doesn't quite cut it, since we also want to detect containment. 
		///  Perhaps there's another way to do this faster than Theta(n^2).
	
		if (freeRectangles.size() > 0)
			clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), NodeSortCmp);
	
		for(size_t i = 0; i < freeRectangles.size()-1; ++i)
			if (freeRectangles[i].x == freeRectangles[i+1].x &&
				freeRectangles[i].y == freeRectangles[i+1].y &&
				freeRectangles[i].width == freeRectangles[i+1].width &&
				freeRectangles[i].height == freeRectangles[i+1].height)
			{
				freeRectangles.erase(freeRectangles.begin() + i);
				--i;
			}
		*/

		/// Go through each pair and remove any rectangle that is redundant.
		for (size_t i = 0; i < freeRectangles.size(); ++i)
			for (size_t j = i + 1; j < freeRectangles.size(); ++j) {
				if (isContainedIn(freeRectangles[i], freeRectangles[j])) {
					freeRectangles.erase(freeRectangles.begin() + i);
					--i;
					break;
				}
				if (isContainedIn(freeRectangles[j], freeRectangles[i])) {
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
			}
	}
}
