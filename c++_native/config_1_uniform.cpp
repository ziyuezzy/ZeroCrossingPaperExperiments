#include <ogdf/basic/Graph.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <filesystem>

using namespace ogdf;

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        std::cerr << "Usage: ./executable N factor (the bandwidth per GPU is factor*(N-1)*4GBps*16(wavelengths)GBps)" << std::endl;
        return 1; // Exit with an error code
    }

    // Extract command-line arguments
    int N = std::stoi(argv[1]);
    int factor = std::stoi(argv[2]); // the bandwidth per GPU is factor*(N-1)*4GBps*16(wavelengths)GBps

    // Create the graph
    Graph G;

    // Create the nodes
    NodeArray<node> nodes(G);
    for (int i = 0; i < N; ++i) {
        nodes[i] = G.newNode();
    }

    // Create edges
    int num_edges = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            for (int f = 0; f < factor; ++f) {
                G.newEdge(nodes[i], nodes[j]);
                ++num_edges;
                G.newEdge(nodes[j], nodes[i]);
                ++num_edges;
            }
        }
    }

    // Set up graph attributes
    GraphAttributes GA(G, GraphAttributes::all);
    // GA.directed = false;
    for (node n : G.nodes) {
        GA.label(n) = std::to_string(n->index());
    }
    for (edge e : G.edges) {
        GA.arrowType(e) = EdgeArrow::None;
    }

    // Initialize layout components
    PlanarizationLayout pl;
    SubgraphPlanarizer *crossMin = new SubgraphPlanarizer;
    PlanarSubgraphFast<int> *ps = new PlanarSubgraphFast<int>;
    ps->runs(100);
	VariableEmbeddingInserter *ves = new VariableEmbeddingInserter;
	ves->removeReinsert(RemoveReinsertType::All);

    // Set up PlanarizationLayout components
    crossMin->setSubgraph(ps);
    crossMin->setInserter(ves);
    pl.setCrossMin(crossMin);

    OrthoLayout *ol = new OrthoLayout;
    // ol->separation(20.0);
	// ol->cOverhang(0.4);
    pl.setPlanarLayouter(ol);
    pl.call(GA);

    // Draw the graph to SVG file
    GraphIO::drawSVG(GA, "svgs/N_" + std::to_string(N) + "_" + std::to_string(factor) +
                                   "_TBps_config_1_uniform.svg");

    int num_crossings = pl.numberOfCrossings();
    int ave_crossing_per_wg = round((num_crossings)/num_edges);

    // [N, Theta, ave_crossing_per_wg] in the csv file
    std::vector<int> data = {N, factor * (N-1)*4*16, ave_crossing_per_wg};

    // Specify the file name
    const std::string csv_file_name = "config_1_uniform_planarization.csv";

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
    // nodes.clear()
    // GA.clearAllBends
    return 0;
}