// #include <ogdf/basic/graph_generators.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/fileformats/GraphIO.h>
#include <filesystem>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarity/MultiEdgeApproxInserter.h>
#include <ogdf/planarity/PlanarizerChordlessCycle.h>
#include <ogdf/planarity/PlanarizerMixedInsertion.h>
#include <ogdf/planarity/PlanarizerStarReinsertion.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/planarity/VariableEmbeddingInserterDyn.h>

using namespace ogdf;


template<typename EdgeInserter>
void setRemoveReinsert(EdgeInserter& edgeInserter, RemoveReinsertType rrType) {
	edgeInserter.removeReinsert(rrType);
}

template<>
void setRemoveReinsert(MultiEdgeApproxInserter& edgeInserter, RemoveReinsertType rrType) {
	edgeInserter.removeReinsertVar(rrType);
	edgeInserter.removeReinsertFix(rrType);
}

int heuristic_min_cross(Graph &G){
    SubgraphPlanarizer heuristic;
    FixedEmbeddingInserter *edgeInserter=new FixedEmbeddingInserter;
    //options:
    	// (new FixedEmbeddingInserter);
		// (new MultiEdgeApproxInserter);
		// (new VariableEmbeddingInserter);
		// (new VariableEmbeddingInserterDyn);
	
    heuristic.setInserter(edgeInserter);
    RemoveReinsertType rrType=RemoveReinsertType::Inserted;
    setRemoveReinsert(*edgeInserter, rrType);
    heuristic.permutations(1);

    using ReturnType = CrossingMinimizationModule::ReturnType;
    PlanRep planRep(G);
	planRep.initCC(0);
	int crossNum = 1; // an arbitrary nonzero number
	ReturnType result = heuristic.call(planRep, 0, crossNum, nullptr);

    return crossNum;
}


int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        std::cerr << "Usage: ./executable N factor (the bandwidth per GPU is factor*1024GBps(16 switches)" << std::endl;
        return 1; // Exit with an error code
    }

    // Extract command-line arguments
    int N = std::stoi(argv[1]);
    int factor = std::stoi(argv[2]); // the bandwidth per GPU is factor*(N-1)*4GBps*16(wavelengths)GBps
    int baseline_T=8; // 512GBps per GPU
    int T=factor*baseline_T;

	Graph G;
    // Create the nodes
    NodeArray<node> N_nodes(G);
    for (int i = 0; i < N; ++i) {
        N_nodes[i] = G.newNode();
    }
    NodeArray<node> T_nodes(G);
    for (int i = 0; i < T; ++i) {
        T_nodes[i] = G.newNode();
    }
    // Create edges
    int num_edges = 0;
    for (int t = 0; t < T; ++t) {
        for (int n = 0; n < N; ++n) {
            G.newEdge(T_nodes[t], N_nodes[n]);
            ++num_edges;
            G.newEdge(T_nodes[t], N_nodes[n]);
            ++num_edges;
        }
    }

    int crossNum = heuristic_min_cross(G);
	// SubgraphPlanarizer SP;
	// SP.setSubgraph(new PlanarSubgraphFast<int>);
	// SP.setInserter(new VariableEmbeddingInserter);

	// int crossNum;
	// PlanRep PR(G);
	// SP.call(PR, 0, crossNum);


	std::cout << crossNum << " crossings" << std::endl;
    int ave_crossing_per_wg = round((crossNum*2)/num_edges);
    std::vector<int> data = {N, factor * 512, ave_crossing_per_wg};
    // Specify the file name
    const std::string csv_file_name = "config_2_planarization.csv";
    std::ofstream file;
    if (std::filesystem::exists(csv_file_name)) {
        // Writing to CSV file
        file.open(csv_file_name, std::ios::app);
        file << data[0] << "," << data[1] << "," << data[2] << "\n";
    } else {
        file.open(csv_file_name);
        file << "N,Theta[GBps],ave_crossing_per_wg\n";
        file << data[0] << "," << data[1] << "," << data[2] << "\n";
    }
    file.close();

    G.clear();
	return 0;
}
