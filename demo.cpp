#include <random>
#include <chrono>
#include <boost/program_options.hpp>
#include "Matching.hpp"

namespace po = boost::program_options;

auto now(){
	return std::chrono::high_resolution_clock::now();
}

// random seed; set this to a fixed value if debugging
long seed = now().time_since_epoch().count();
//long seed = 1611279561310343530L;
std::uniform_real_distribution<float> dist;

int main(int argc, char** argv){
	// Specify command line arguments
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Display this help")
		("verbose,v", po::value<bool>()->default_value(true), "Print arcs and results")
		("persons,p", po::value<uint>()->default_value(2), "How many persons to generate")
		("objects,o", po::value<uint>()->default_value(3), "How many objects to generate, >= persons")
		("slack,e", po::value<float>()->default_value(-1e-10), "The slack benefit")
		("samples,s", po::value<uint>()->default_value(1), "How many random samples to run");

	// Parse command line arguments
	uint max_persons, max_objects, samples;
	float slack;
	bool verbose;
	try{
		po::variables_map args_map;
		po::store(po::parse_command_line(argc, argv, desc), args_map);
		po::notify(args_map);

		if (args_map.count("help")){
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		max_persons = args_map["persons"].as<uint>();
		max_objects = args_map["objects"].as<uint>();
		samples = args_map["samples"].as<uint>();
		slack = args_map["slack"].as<float>();
		verbose = args_map["verbose"].as<bool>();
	} catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	// First, create a new Matching object with the maximum number of persons and objects, respectively;
	// This same object can be reused for multiple problems, so long as the person/object sets stay <= the max 
	Matching matcher(max_persons, max_objects);
	// Specify the benefit for a person being unassigned in the final solution
	matcher.slack_benefit = slack;

	// Here we'll generate
	for (uint sample=0; sample<samples; sample++){
		printf("Sample #%u\n", sample+1);
		// Increment seed, so if multiple samples, we can easily rerun one of them
		printf("Random seed = %lu\n", seed);
		std::default_random_engine gen(seed++);
		
		// Number of persons/objects can be less than max per solve, if desired
		const uint persons = max_persons,
			objects = max_objects;
		
		// Fill matcher.arcs with the benefits for pairing persons and objects
		if (verbose)
			printf("Generating random arc benefits:\n");
		float max_arc = MIN_FLT;
		for (int pi=0; pi<persons; pi++){
			for (int oi=0; oi<objects; oi++){
				// Here we are just using random benefits;
				// this will likely be computed elsewhere in your program from sensor data or what not
				float arc = dist(gen)*100.0;
				if (arc > max_arc)
					max_arc = arc;
				matcher.arcs[pi*objects+oi] = arc;
				if (verbose)
					printf("\t%d -> %d = %f\n", pi, oi, arc);
			}
		}
		if (verbose)
			printf("Max arc = %f\n\n", max_arc);

		// Arcs are all defined, so we can run the solver;
		// Need to pass in the maximum arc from the arc matrix
		printf("Starting solve...\n");
		auto start = now();
		matcher.solve(persons, objects, max_arc);
		auto end = now();
		std::chrono::duration<float, std::milli> runtime = end-start;
		printf("Finished: %fms\n\n", runtime.count());
		

		// Print out the matching results
		if (verbose){
			printf("Assignment results:\n");
			for (int pi=0; pi<persons; pi++){
				// This is the assigned matching (note subtracting m, due to the internal solver's representation)
				int oi = matcher.match[pi]-persons;
				// If greater than the number of objects, it was assigned to an artifical object (e.g. unassigned)
				if (oi >= objects)
					printf("%d -> null\n", oi);
				else printf("%d -> %d\n", pi, oi);
			}
			printf("\n");
		}
	}

	return EXIT_SUCCESS;
}