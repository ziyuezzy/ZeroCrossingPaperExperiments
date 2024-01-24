import sys
import csv
import os
# Check if the correct number of arguments is provided
if len(sys.argv) != 3:
    print("Usage: python3.10 config_1_uniform.py N factor (the bandwidth per GPU is factor*960GBps)")
    sys.exit(1)  # Exit with an error code
# Extract command-line arguments
N = int(sys.argv[1])
factor = int(sys.argv[2]) # the bandwidth per GPU is factor*960GBps

from ogdf_python import *
# Tile_length = 100.0
cppinclude("ogdf/fileformats/GraphIO.h")
cppinclude("ogdf/orthogonal/OrthoLayout.h")
cppinclude("ogdf/planarity/PlanarSubgraphFast.h")
cppinclude("ogdf/planarity/PlanarizationLayout.h")
cppinclude("ogdf/planarity/SubgraphPlanarizer.h")
cppinclude("ogdf/planarity/VariableEmbeddingInserter.h")

G = ogdf.Graph()
# Create the nodes
nodes = [G.newNode() for _ in range(N)]
# Create two edges between each pair of nodes
num_edges=0
for i in range(N):
    for j in range(i+1, N):
        for f in range(factor):
            G.newEdge(nodes[i], nodes[j])
            num_edges+=1
            G.newEdge(nodes[j], nodes[i])
            num_edges+=1

GA = ogdf.GraphAttributes(G, ogdf.GraphAttributes.all)
# define graph attributes
GA.directed=False
for n in G.nodes:
    GA.label[n] = "%s" % n.index()
for e in G.edges:
    GA.arrowType[e] =0

# Initialize layout components
pl = ogdf.PlanarizationLayout()
crossMin = ogdf.SubgraphPlanarizer()
ps = ogdf.PlanarSubgraphFast[int]()
ps.runs(100)
ves = ogdf.VariableEmbeddingInserter()
ves.removeReinsert(ogdf.RemoveReinsertType.All)

# Set up PlanarizationLayout components
crossMin.setSubgraph(ps)
crossMin.setInserter(ves)
pl.setCrossMin(crossMin)

ol = ogdf.OrthoLayout()
# ol.separation(10.0)
# ol.cOverhang(10.0)
pl.setPlanarLayouter(ol)
pl.call(GA)

ogdf.GraphIO.drawSVG(GA, f"N_{N}_{factor}_TBps_config_1_uniform.svg")  # for SVG format

num_crossings=pl.numberOfCrossings()
ave_crossing_per_wg=num_crossings/num_edges

# [N, Theta, ave_crossing_per_wg] in the csv file
data = [N, factor*960, ave_crossing_per_wg]
# Specify the file name
csv_file_name = "config_1_uniform_planarization.csv"
if os.path.exists(csv_file_name):
    # Writing to CSV file
    with open(csv_file_name, mode='a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(data)
else:
    with open(csv_file_name, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["N", "Theta[TBps]", "ave_crossing_per_wg"])
        writer.writerow(data)


# del G
# del GA
# del pl
# del ol
# del ves
# del crossMin