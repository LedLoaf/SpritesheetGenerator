/** @file Rect.h
	@author Jukka Jylänki

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <cassert>
#include <cstdlib>
#include <vector>

#ifdef _DEBUG
/// debug_assert is an assert that also requires debug mode to be defined.
#define debug_assert(x) assert(x)
#else
#define debug_assert(x)
#endif

//using namespace std;

namespace rbp {

struct RectSize
{
	int width{};
	int height{};
};

struct Rect
{
	int x{};
	int y{};
	int width{};
	int height{};
};

/// Performs a lexicographic compare on (rect short side, rect long side).
/// @return -1 if the smaller side of a is shorter than the smaller side of b, 1 if the other way around.
///   If they are equal, the larger side length is used as a tie-breaker.
///   If the rectangles are of same size, returns 0.
int compareRectShortSide(const Rect &a, const Rect &b);

/// Performs a lexicographic compare on (x, y, width, height).
int nodeSortCmp(const Rect &a, const Rect &b);

/// Returns true if a is contained in b.
bool isContainedIn(const Rect &a, const Rect &b);

class DisjointRectCollection
{
public:
	std::vector<Rect> rects;

	bool add(const Rect &r)
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0)
			return true;

		if (!disjoint(r))
			return false;
		rects.push_back(r);
		return true;
	}

	void clear()
	{
		rects.clear();
	}

	bool disjoint(const Rect &r) const
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0)
			return true;

		for (auto rect : rects) {
			if (!disjoint(rect, r))
				return false;
		}
		return true;
	}

	static bool disjoint(const Rect &a, const Rect &b)
	{
		if (a.x + a.width <= b.x ||
			b.x + b.width <= a.x ||
			a.y + a.height <= b.y ||
			b.y + b.height <= a.y)
			return true;
		return false;
	}
};

}
