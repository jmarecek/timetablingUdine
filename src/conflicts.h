/* A Branch-and-cut Procedure for the Udine Course Timetabling Problem.
* 
* Copyright (C) 2007-2010 Jakub Marecek and The University of Nottingham.
* Licensed under the GNU GPL. Please read LICENSE file for details.
* 
* Please cite:
* A Branch-and-cut Procedure for the Udine Course Timetabling Problem
* by Edmund K. Burke, Jakub Marecek, Andrew J. Parkes, and Hana Rudova
* available from http://cs.nott.ac.uk/~jxm/timetabling/
*/

#ifndef UDINE_CONFLICT_GRAPH
#define UDINE_CONFLICT_GRAPH

#include <vector>
#include <utility>

#include "loader.h"

#ifdef _WIN32
extern "C" {
#include "cliquer.h"
#include "graph.h"
#include "reorder.h"
}
#else
#include "cliquer.h"
#include "graph.h"
#include "reorder.h"
#endif

class Vertex {  
public:      
  std::set<int> adj;        // ajacent vertices 
  int c;                       // color
  Vertex() { c = -1; }         // constructor
};

typedef std::pair<int, int> Edge;


class Graph {
protected:
  graph_t *cliquerRepresentation;
  boolean generateAllCliquesHelper(set_t s, graph_t *g, clique_options *opts);
  static boolean generateAllCliquesHelperWrapper(set_t s, graph_t *g, clique_options *opts);
public:
  std::vector<Vertex> vs;
  std::vector<Edge> es;
  std::vector< std::vector<int> > cliques;
public:
  virtual void generateConflictGraph(TimetablingInstance &i);  
  virtual void generateAllCliques(); 
  virtual void generateSomeCliques(); 
  virtual void exportDimacs(const char *filename, const char *comment = "", bool binary = false);
};

#endif // UDINE_CONFLICT_GRAPH
