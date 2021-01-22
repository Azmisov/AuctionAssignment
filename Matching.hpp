#pragma once
// Validation flags
#define V false
// Disables assertions
#define NDEBUG

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>

static const float MIN_FLT = std::numeric_limits<float>::lowest();
static const float MAX_FLT = std::numeric_limits<float>::max();

class Matching{
public:
	// Arc benefit for artificial objects
	float slack_benefit = -1e-10;
	// Arc benefits, in the form midx*n + nidx (does not include artificial object arcs)
	// Since n > m, cache locality benefits the worst case of traversing n arcs
	float *arcs;
	// m persons + n objects + m artificial objects; see docs in solve for "sort" structure
	// Split into separate arrays, for better caching
	float *value;		// profit/price, depending on if person/object
	uint *match;		// index of current match (if assigned)
	uint *sort;			// identifies which items are (un)assigned (index into match/vals arrays)
	uint *sort_rev;		// reverse index for sort; identifies sort position index in match/vals
	uint *buffer;		// buffer for restructuring "sort"

	Matching(uint max_m, uint max_n);
	~Matching();

	void solve(uint m, uint n, float max_arc);

	// Methods for manipulating person/object position in "sort" structure
	inline void swap(uint i, uint si, uint j, uint sj);
	inline void move(uint i, uint si, uint sj, bool force = false);
	inline void move_single(uint *si, uint sj);
	inline void set_sort(uint si, uint sj);

	// Debugging stuff
	void validate(uint m, uint n, uint unassigned, uint below_lambda, uint equal_lambda, uint above_lambda, float lambda);
	void print_state(bool person, uint m, uint n, uint unassigned, uint below_lambda, uint equal_lambda, uint above_lambda, uint lambda);
	void is_optimal(uint m, uint n, float slack, float fuzz = 0.1f);
};