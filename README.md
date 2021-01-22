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
- _arc benefits:_ An arc is the undirected, weighted edge between a person and object in the bipartite graph. The edge weight is the arc benefit. We'll be solving the maximization case, so the more weight, the better, hence "benefits". Of course, for minimization, you can use negative benefits, or subtract from a large number. Note also, I'm just implementing the dense case, where there is an edge between each person-object; the source code could be modified for the sparse case fairly easily if desired.
- _artificial objects:_ We can allow persons to be unmatched in the final solution by introducing artificial objects. Perhaps in your application, the real-world object match for a person might not have been recorded by the sensors. An artificial object is just a virtual distinction indicating a person can be assigned to nothing.
- _slack benefit_: The slack benefit is the arc benefit for a person being unassigned (or rather, assigned to a virtual artificial object). If you want to force every person to be assigned, this just needs to be lower than all other arc benefits (a small float, but not >= `std::numeric_limits<float>::lowest()` would serve). For simplicty, there is just one global `slack_benefit` for all persons, rather than having to specify the slack arc benefit for each person. If you wish to allow varying slack benefits for each person, you will need to modify the code a little bit.

Example usage is provided in the file ``demo.cpp``. To build and run:

```console
user@hostname:~$ ./build_demo.sh
user@hostname:~$ ./demo -h
```

This runs a demo assigning persons to objects with random arc benefits.

## Debugging
There are numerous assertions throughout the code to ensure the algorithm is performing correctly. I believe from all my testing that the algorithm is working perfectly, but I've left them in just in case. You can enable debugging mode by changing the `V` and `NDEBUG` variables at the top of `Matching.hpp`. There are a couple methods like `validate`, `print_state`, and `is_optimal`, to ensure it is giving the correct results. I've also left all of the variables and methods public for convenience in modifying the code.

## Future enhancements?
Currently artificial objects are hard-coded. It would be nice to have a third constructor option for the number of artificial objects, so problems that don't need them don't suffer the extra runtime.