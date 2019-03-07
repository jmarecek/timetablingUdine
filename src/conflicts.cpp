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

#include <cstdio>
#include <cassert>
#include <vector>
#include <set>
#include <algorithm>

#include "conflicts.h"

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

void* cliquerHelperWrapperHack;

void Graph::generateConflictGraph(TimetablingInstance &instance) {
  vs.clear();
  es.clear();
  cliques.clear();

  int i;
  Vertex fresh;
  for (i = 0; i < instance.getCourseCount(); i++) 
    vs.push_back(fresh);

  cliquerRepresentation = graph_new(vs.size()); 

  for (i = 0; i != instance.getCurriculumCount(); i++) {
    CourseIds::const_iterator it1 = instance.getCurriculum(i).courseIds.begin();
    for (; it1 != instance.getCurriculum(i).courseIds.end(); it1++) {
      CourseIds::const_iterator it2 = it1;
      std::advance(it2, 1);
      for (; it2 != instance.getCurriculum(i).courseIds.end(); it2++) {
        Edge e(*it1, *it2);
        es.push_back(e);
        vs.at(*it1).adj.insert(*it2);
        vs.at(*it2).adj.insert(*it1);
        GRAPH_ADD_EDGE(cliquerRepresentation, *it1, *it2);
      }
    }
  }

  std::cout << "Graphs: Generated a conflict graph with " << vs.size();
  std::cout << " vertices and " << es.size() << " edges" << std::endl;  
}  // END of Graph::generateConflictGraph


typedef std::pair< int, std::vector<int> > Candidate;
struct CandidateLess { 
  bool operator()(const Candidate &x, const Candidate &y ) {
    return (x.second.size() > y.second.size());
  }
};


boolean Graph::generateAllCliquesHelper(set_t s, graph_t *g, clique_options *opts) {
  std::vector<int> clique;
  std::cout << "Clique: ";
  for (int i=0; i<SET_MAX_SIZE(s); i++)
    if (SET_CONTAINS(s,i)) {
      clique.push_back(i);
      std::cout << i << " ";
    }
    cliques.push_back(clique);
    std::cout << std::endl;
    return TRUE;
};


boolean Graph::generateAllCliquesHelperWrapper(set_t s, graph_t *g, clique_options *opts) {
  return ((Graph*)cliquerHelperWrapperHack)->generateAllCliquesHelper(s, g, opts);
};


void Graph::generateAllCliques() {
  std::cout << "Graphs: Generating all maximal cliques ..." << std::endl;  
  cliques.clear();

  // the cliquer interface
  clique_options *opts; 
  opts = (clique_options *)malloc(sizeof(clique_options)); 
  cliquerHelperWrapperHack = (void*)this;
  opts->user_function = generateAllCliquesHelperWrapper;
  opts->time_function = NULL;
  opts->output = stderr;
  opts->reorder_function = NULL;
  opts->reorder_map = NULL;
  opts->user_data = NULL;
  opts->clique_list = NULL;
  opts->clique_list_length = 0; 
  int num = clique_unweighted_find_all(cliquerRepresentation, 0, 0, true, opts);

  std::cout << "Graphs: Clique pool initialised with " << cliques.size() << " clique(s)" << std::endl;  
} // END Graph::generateAllCliques


void Graph::generateSomeCliques() {
  std::cout << "Graphs: Generating some cliques ..." << std::endl;  
  cliques.clear();

  // the size of the largest cli que not to pre-generate
  int minLimit = 3;

  // for all vertices
  int u = 0;
  for (u = 0; u < vs.size(); u++) {

    if (vs[u].adj.size() < minLimit) continue;

    std::vector<Candidate> cands;
    for (std::set<int>::iterator vi = vs[u].adj.begin(); vi != vs[u].adj.end(); vi++) {
      std::vector<int> intersection;
      std::set_intersection(vs[u].adj.begin(), vs[u].adj.end(),
        vs[*vi].adj.begin(), vs[*vi].adj.end(), 
        std::back_inserter(intersection));
      if (u > (*vi) && intersection.size() > minLimit) {
        Candidate cand(*vi, intersection);
        cands.push_back(cand);
      }
    }

    if (cands.size() < minLimit) continue;

    CandidateLess CandidateLessInstance;
    std::sort(cands.begin(), cands.end(), CandidateLessInstance);

    std::vector<int> clique;
    for(int candi = 0; candi < cands.size(); candi++)
      if (std::includes(cands[candi].second.begin(), cands[candi].second.end(), clique.begin(), clique.end()))
        clique.push_back(cands[candi].first);

    if (clique.size() < minLimit) continue;

    clique.push_back(u);
    cliques.push_back(clique);
  }

  std::cout << "Graphs: Clique pool initialised with " << cliques.size() << " clique(s)" << std::endl;  
} // END Graph::generateSomeCliques

void Graph::exportDimacs(const char *filename, const char *comment, bool binary) {
  // the cliquer interface
  // boolean graph_write_dimacs_ascii_file(graph_t *g,char *comment, char *file);
  // boolean graph_write_dimacs_binary_file(graph_t *g, char *comment, char *file);

  std::cout << "Graphs: Exporting conflict graph to  " << filename << std::endl;  
  if (binary) graph_write_dimacs_binary_file(cliquerRepresentation, comment, filename);
  else graph_write_dimacs_ascii_file(cliquerRepresentation, comment, filename);
}
