#include "Matching.hpp"

Matching::Matching(uint max_m, uint max_n){
	if (max_n < max_m)
		throw std::invalid_argument("max_n must be >= max_m");
	// Adjusted max, once artificial objects are added for feasibility
	uint max_adj = 2*max_m+max_n;
	arcs = new float[max_m*max_n];
	value = new float[max_adj];
	match = new uint[max_adj];
	sort = new uint[max_adj];
	sort_rev = new uint[max_adj];
	buffer = new uint[max_m+max_n];
}
Matching::~Matching(){
	delete[] arcs;
	delete[] value;
	delete[] match;
	delete[] sort;
	delete[] sort_rev;
	delete[] buffer;
}

void Matching::solve(uint m, uint n, float max_arc){
	if (n < m)
		throw std::invalid_argument("n must be >= m");
	assert(n >= m && m > 0 && n > 0);
	assert(max_arc >= slack_benefit && "Warning slack_benefit > max_arc!\nDisable this assertion to enable this behavior");
	// Initialize items
	const uint size = 2*m+n;
	for (uint i=0; i<size; i++){
		value[i] = i < m ? max_arc : 1; // TODO
		sort[i] = i;
		sort_rev[i] = i;
	}

	/* sort index:
		[0, unassigned): unassigned persons
		[unassigned, m): assigned persons
		[m, below_lambda): unassigned objects w/ price < lambda
		[below_lambda, equal_lambda): unassigned objects w/ price = lambda
		[equal_lambda, above_lambda): unassigned objects w/ price > lambda
		[above_lambda, size): assigned objects (price >= lambda)
	*/
	uint unassigned = m,
		below_lambda = m,
		equal_lambda = m,
		above_lambda = size;

	///static_cast<float>(n)
	float slack = 10, lambda = 0;
	//print_state(false, m, n, unassigned, below_lambda, equal_lambda, above_lambda, lambda);
	do{
		// This uses Guass-Siedel bidding (e.g. only one person/object bids at a time)
		// Forward iteration
		while (true){
			if (V) validate(m, n, unassigned, below_lambda, equal_lambda, above_lambda, lambda);
			// Pick an unassigned person
			uint pidx = sort[unassigned-1], oidx = pidx+m+n;

			// Find the first/second best objects
			// Initially, the best is the artificial slack object
			float arc = slack_benefit, val1 = arc-value[oidx], val2 = MIN_FLT;
			// Regular objects
			for (uint i=m, a=pidx*n; i<m+n; i++, a++){
				float cur_arc = arcs[a],
					val = cur_arc-value[i];
				if (val > val2){
					if (val > val1){
						oidx = i;
						arc = cur_arc;
						val2 = val1;
						val1 = val;
					}
					else val2 = val;
				}
			}
			uint soidx = sort_rev[oidx];

			// Update profit
			float profit = val2-slack;
			value[pidx] = profit;

			// Update object price
			float price = arc-profit;
			if (price >= lambda){
				value[oidx] = price;
				// If object was already assigned, swap assignment
				// This makes pidx assigned and pidx_old unassigned
				if (soidx >= above_lambda){
					uint pidx_old = match[oidx];
					assert(pidx_old != pidx);
					swap(pidx, unassigned-1, pidx_old, sort_rev[pidx_old]);
					// Update matching
					match[pidx] = oidx;
					match[oidx] = pidx;
					// Don't switch to reverse cycle, since unassigned cardinality hasn't changed
				}
				// Otherwise, set the new assignment
				else{
					// This makes pidx assigned
					unassigned--;
					// Make object assigned, by repeatedly swapping with below/equal/above_lambda
					if (soidx >= equal_lambda) goto FAL;
					if (soidx >= below_lambda) goto FEL;
					move_single(&soidx, --below_lambda);
					FEL:
						move_single(&soidx, --equal_lambda);
					FAL:
						uint soidx_swap = --above_lambda;
						// May be redundant; but since we've been doing move_singles, we may still need to set the final oidx sort values
						if (soidx == soidx_swap){
							sort[soidx] = oidx;
							sort_rev[oidx] = soidx;
						}
						else swap(oidx, soidx, sort[soidx_swap], soidx_swap);
						// Update matching
						match[pidx] = oidx;
						match[oidx] = pidx;
						// Switch to reverse cycle, once a person has been assigned
						break;
				}
			}
			else{
				value[oidx] = lambda;
				// price must increase, meaning old price < lambda
				// since all assigned objects >= lambda, it must have been unassigned
				assert(soidx < below_lambda);
				// we don't actually assign the object in this situation; but it is now = lambda
				move(oidx, soidx, --below_lambda);
			}
		}

		// Reverse iteration
		while (equal_lambda != above_lambda){
			if (V) validate(m, n, unassigned, below_lambda, equal_lambda, above_lambda, lambda);
			// Pick an unassigned object, with price > lambda
			uint soidx = above_lambda-1, oidx = sort[soidx], pidx;

			// Find the first/second best persons
			float arc, val1, val2;
			// Artificial object
			if (oidx >= m+n){
				pidx = oidx-m-n;
				arc = slack_benefit;
				val1 = arc-value[pidx];
				// Paper indicates this should be +infinity, but his code uses -infinity
				val2 = MIN_FLT;
			}
			// Regular object
			else{
				val1 = MIN_FLT;
				val2 = MIN_FLT;
				for (uint i=0, a=oidx-m; i<m; i++, a+=n){
					float cur_arc = arcs[a],
						val = cur_arc-value[i];
					if (val > val2){
						if (val > val1){
							pidx = i;
							arc = cur_arc;
							val2 = val1;
							val1 = val;
						}
						else val2 = val;
					}
				}
			}

			// Update prices/profits
			float price = val1-slack;
			if (price >= lambda){
				price = std::max(lambda, val2-slack);
				value[pidx] = arc-price;
				value[oidx] = price;
				// If person is already assigned, replace object assignment
				uint spidx = sort_rev[pidx];
				if (spidx >= unassigned){
					uint oidx_old = match[pidx],
						soidx_old = sort_rev[oidx_old];
					assert(value[oidx_old] >= lambda);
					assert(soidx_old >= above_lambda);
					assert(oidx_old != oidx);
					// Old object can be equal to lambda
					bool eq = value[oidx_old] == lambda;
					// Need to swap twice to put in correct category
					if (eq && soidx != equal_lambda){
						sort[soidx_old] = oidx;
						sort_rev[oidx] = soidx_old;
						move(oidx_old, soidx, equal_lambda, true);
					}
					// Otherwise, we can swap directly
					else swap(oidx, soidx, oidx_old, soidx_old);
					if (eq) equal_lambda++;
					// Update matching
					match[pidx] = oidx;
					match[oidx] = pidx;
					// Don't switch to forward cycle, since unassigned cardinality hasn't changed
				}
				// Otherwise, set new assignments
				else{
					move(pidx, spidx, --unassigned);
					// This makes oidx assigned
					above_lambda--;
					// Update matching
					match[pidx] = oidx;
					match[oidx] = pidx;
					// Switch to forward cycle, once an object is assigned
					// If there are no persons to bid in the forward cycle, stay in reverse loop
					// until condition (pj <= lambda for all unassigned objects) is met
					if (unassigned)
						break;
				}
			}
			else{
				value[oidx] = price;
				// object used to be unassigned, above_lambda
				// we don't assign the person to object in this situation, but it is now < lambda
				set_sort(soidx, equal_lambda);
				set_sort(equal_lambda, below_lambda);
				sort[below_lambda] = oidx;
				sort_rev[oidx] = below_lambda;
				below_lambda++;
				equal_lambda++;
				// Lower lambda, so more objects can be assigned
				if (below_lambda > m+n){
					// Find min price of objects below lambda
					uint eq;
					float min_price = MAX_FLT;
					for (uint i=m; i<below_lambda; i++){
						uint oidx_bl = sort[i];
						float val = value[oidx_bl];
						if (val < min_price){
							eq = 1;
							min_price = val;
							buffer[0] = oidx_bl;
						}
						else if (val == min_price)
							buffer[eq++] = oidx_bl;
					}
					// Move all objects = lambda
					for (uint i=0; i<eq; i++){
						uint oidx_eq = buffer[i];
						move(oidx_eq, sort_rev[oidx_eq], m+i);
					}
					// Update lambda
					below_lambda = m;
					equal_lambda = m+eq;
					lambda = min_price;
				}
			}
		}
	} while (unassigned);

	if (V){
		validate(m, n, unassigned, below_lambda, equal_lambda, above_lambda, lambda);
		std::cout << "ensuring optimal...\n";
		is_optimal(m, n, slack);
	}
}

// Data structure helpers
inline void Matching::swap(uint i, uint si, uint j, uint sj){
	// Never check for si/sj equality
	sort_rev[i] = sj;
	sort_rev[j] = si;
	sort[si] = j;
	sort[sj] = i;
}
inline void Matching::move(uint i, uint si, uint sj, bool force){
	// Always check for si/sj equality
	if (force || si != sj)
		swap(i, si, sort[sj], sj);
}
// Moves value at sj to position si, storing old sj in si
inline void Matching::move_single(uint *si, uint sj){
	// Always check for si/sj equality
	if (*si != sj){
		uint j = sort[sj];
		sort[*si] = j;
		sort_rev[j] = *si;
		*si = sj;
	}
}
// Same as move_single, but doesn't keep track of the previous sort value
inline void Matching::set_sort(uint si, uint sj){
	// Always check for si/sj equality
	if (si != sj){
		uint j = sort[sj];
		sort[si] = j;
		sort_rev[j] = si;
	}
}

// Debugging stuff
void Matching::validate(uint m, uint n, uint unassigned, uint below_lambda, uint equal_lambda, uint above_lambda, float lambda){
	uint size = 2*m+n;
	assert(unassigned >= 0);
	assert(m >= unassigned);
	assert(below_lambda >= m);
	assert(equal_lambda >= below_lambda);
	assert(above_lambda >= equal_lambda);
	assert(size >= above_lambda);

	for (uint j=0; j<size; j++){
		uint s = sort_rev[j];
		assert(s >= 0 && s < size);
		assert(sort[sort_rev[j]] == j);
	}

	// Same number of assigned items
	assert(m-unassigned == size-above_lambda);

	// Person structure
	std::cout << "\tλ: " << lambda << std::endl;
	std::cout << "\tp: ";
	uint i=0;
	if (i == unassigned) std::cout << "- ";
	else{
		for (; i<unassigned; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= 0 && id < m);
		}
	}
	std::cout << "| ";
	if (i == m) std::cout << "- ";
	else{
		for (; i<m; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= 0 && id < m);
			uint z = match[id];
			assert(sort_rev[z] >= above_lambda);
			assert(match[z] == id);
		}
	}
	std::cout << std::endl;

	// Object structure
	std::cout << "\to: ";
	if (i == below_lambda) std::cout << "- ";
	else{
		for (; i<below_lambda; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= m && id < size);
			assert(value[id] < lambda);
		}
	}
	std::cout << "| ";
	if (i == equal_lambda) std::cout << "- ";
	else{
		for (; i<equal_lambda; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= m && id < size);
			assert(value[id] == lambda);
		}
	}
	std::cout << "| ";
	if (i == above_lambda) std::cout << "- ";
	else{
		for (; i<above_lambda; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= m && id < size);
			assert(value[id] > lambda);
		}
	}
	std::cout << "| ";
	if (i == size) std::cout << "- ";
	else{
		for (; i<size; i++){
			uint id = sort[i];
			std::cout << id << " ";
			assert(id >= m && id < size);
			assert(value[id] >= lambda);
		}
	}
	std::cout << std::endl;
}
void Matching::print_state(bool person, uint m, uint n, uint unassigned, uint below_lambda, uint equal_lambda, uint above_lambda, uint lambda){
	uint size = 2*m+n;

	// Person structure
	if (person){
		std::cout << "\tp: ";
		uint i=0;
		if (i == unassigned) std::cout << "- ";
		else{
			for (; i<unassigned; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << "| ";
		if (i == m) std::cout << "- ";
		else{
			for (; i<m; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << std::endl;
	}
	// Object structure
	else{
		uint i=m;
		std::cout << "\to: ";
		if (i == below_lambda) std::cout << "- ";
		else{
			for (; i<below_lambda; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << "| ";
		if (i == equal_lambda) std::cout << "- ";
		else{
			for (; i<equal_lambda; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << "| ";
		if (i == above_lambda) std::cout << "- ";
		else{
			for (; i<above_lambda; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << "| ";
		if (i == size) std::cout << "- ";
		else{
			for (; i<size; i++)
				std::cout << sort[i] << " ";
		}
		std::cout << std::endl;
	}
}
void Matching::is_optimal(uint m, uint n, float slack){
	/* To be optimal:
		profit + price >= arc-slack, for all arcs
		profit + price = arc, for all matched arcs
		price of all unassigned objects <= min(price of all assigned objects)
	*/
	float fuzz = .1;
	std::cout.precision(12);
	for (int i=0; i<m; i++){
		for (int j=0; j<n; j++){
			float a = value[i]+value[m+j], b = arcs[i*n+j]-slack;
			if (a < b)
				std::cout << a << " >= " << b << std::endl;
			assert(a-b >= -fuzz);
		}
		float a = value[i]+value[m+n+i], b = slack_benefit-slack;
		if (a < b)
			std::cout << a << " >= " << b << std::endl;
		assert(a-b >= -fuzz);
		uint pair = match[i];
		if (pair >= m+n){
			float a = value[i]+value[pair];
			if (a != b)
				std::cout << a << " == " << slack_benefit << std::endl;
			assert(std::abs(a-slack_benefit) < fuzz);
		}
		else{
			float arc1 = arcs[i*n+(pair-m)], dual = value[i]+value[pair];
			if (arc1 != dual)
				std::cout << arc1 << " == " << dual << std::endl;
			assert(std::abs(arc1-dual) < fuzz);
		}
	}
	float max_p = MIN_FLT;
	uint size = 2*m+n;
	for (int i=m; i<size; i++){
		float p = value[sort[i]];
		if (i >= size-m){
			if (p < max_p)
				std::cout << p << " >= " << max_p << std::endl;
			assert(p-max_p >= -fuzz);
		}
		else if (p > max_p)
			max_p = p;
	}
}