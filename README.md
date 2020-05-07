# AuctionAssignment
The assignment problem is described in this [wikipedia article](https://en.wikipedia.org/wiki/Assignment_problem). Basically, you have a weighted bipartite graph, and you want to find a combination of pairings with the maximum weight (or perhaps minimal weight, depending on what your specific application is). The [Hungarian](https://en.wikipedia.org/wiki/Hungarian_algorithm) algorithm is a common choice for solving these, but in practice, it has been seen that iterative "auction" algorithms are more efficient. I've included several papers on the topic in the folder `reference_papers`.

I couldn't find any good implementations of auction algorithms, despite their purported advantages, so I'm sharing mine. As far as I know, it is the fastest solver of its type. There are just two files, `Matching.cpp` and `Matching.hpp`. The algorithm implements the Gauss-Siedel asymmetric forward-reverse auction process described originally by Bertsekas, 1993. See the included papers for details, as the algorithm is quite complicated. I've tried to comment the code as much as I could to make it a bit easier to understand. There are a number of extensions to the algorithm you can try, but I have not implemented them:
- It is possible to implement a forward-only or reverse-only modification of the algorithm. The combined forward-reverse should converge faster however, albeit with increased code complexity.
- Bertsekas also describes a Jacobi method of auctioning; the Jacobi method allows all the "persons" to bid for their desired pairing in parallel, thus allowing for better implementation on a GPU for example. For single-threaded approaches (or high overhead parallel architectures), I don't believe the Jacobi method has any advantage over the regular method.
- In line with the Jacobi method, you may also consider the look-back auction method, also useful for parallel bidding. See included papers for details.
- Older incarnations of Bertsekas method included variations like adaptive scaling and epsilons. I believe these are obsolete with the latest that I've implemented. But I'm mentioning them anyways in case I am mistaken and you wish to explore further.

## Usage
I am using the terminology from the auction literature. To understand how to setup the code to solve the matching problem, you'll need to know these terms:
- _persons:_ The first disjoint set in the bipartite matching graph
- _objects:_ The second disjoint set in the bipartite matching graph; the goal is to match persons to objects. Due to how the algorithm is defined, this must be greater than or equal to the number of persons; if your application is the asymmetric case, the smaller set should be used as the persons set.
- _arc benefits:_ An arc is the undirected, weighted edge between a person and object in the bipartite graph. The edge weight is the arc benefit. We'll be solving the maximization case, so the more weight, the better, hence "benefits". Of course, for minimization, you can use negative benefits, or subtract from a large number.
- _artificial objects:_ We can allow persons to be unmatched in the final solution by introducing artificial objects. Perhaps in your application, the real-world object match for a person might not have been recorded by the sensors. An artificial object is just a virtual distinction indicating a person can be assigned to nothing.
- _slack benefit_: The slack benefit is the arc benefit for a person being unassigned (or rather, assigned to a virtual artificial object). If you want to force every person to be assigned, this just needs to be lower than all other arc benefits (the minimum float could function for this purpose: `std::numeric_limits<float>::lowest()`). Note though that internally, the algorithm still will use artificial objects and the slack benefit until it has converged at the optimal matching. For simplicty, there is just one global `slack_benefit` for all persons, rather than having to specify the slack arc benefit for each person. If you wish to allow varying slack benefits for each person, you will need to modify the code a little bit.

Example usage:

```c++
// First, create a new Matching object with the maximum number of persons and objects, respectively
// This same object can be reused for multiple problems, so long as the person/object sets stay <= the max 
Matching matcher(max_persons, max_objects);
// Fill matcher.arcs with the benefits for pairing persons and objects
// Here's an example, using some user defined get_pairing_cost function
for (int m=0; m<persons; m++)
	for (int n=0; n<objects; n++)
    	matcher.arcs[m*objects+n] = get_pairing_cost(m, n);
// Specify the benefit for a person being unassigned in the final solution
matcher.slack_benefit = 47;
/* Arcs are all defined, so we can run the solver
	We'll also pass in the maximum value in m.arcs. The solver could calculate this value itself, but since we're already
    assigning values to m.arcs, it assumes the caller can compute it more efficiently and pass it in.
*/    
matcher.solve(persons, objects, get_max_pairing());
/* The results are in matcher.match; for speed and efficiency, the solver does minimal post-processing on the results;
	We'll need to do some simple calculations to get the matching out
*/
for (int m=0; m<persons; m++){
	// This is the assigned matching (note subtracting m, due to the internal solver's representation)
	int n = matcher.match[m]-m;
    // If greater than the number of objects, it was assigned to an artifical object (e.g. unassigned)
    if (n > objects)
    	printf("person %d unassigned", m);
    else printf("person %d assigned to object %d", m, n);
    // Matching class contains several other member variables, but they are all just for the solver's internal state and irrelevant to final results
}
// No reset is necessary to run the solver again, simply set benefits and call solve again
```

## Debugging
There are numerous assertions throughout the code to ensure the algorithm is performing correctly. I believe from all my testing that the algorithm is working perfectly, but I've left them in just in case. You can enable debugging mode by changing the `V` and `NDEBUG` variables at the top of `Matching.hpp`. There are a couple methods like `validate`, `print_state`, and `is_optimal`, to ensure it is giving the correct results. I've also left all of the variables and methods public for convenience in modifying the code.