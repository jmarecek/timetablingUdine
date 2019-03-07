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

#ifndef UDINE_CUT_CUT_MANAGER
#define UDINE_CUT_CUT_MANAGER

#include <map>
#include <vector>
#include <utility>

#include <boost/shared_ptr.hpp>
#include <ilcplex/ilocplex.h>

#include "solver.h"


// For clique cuts
typedef std::pair<int, int> CliqueCutIdentifier;  // period, clique
typedef std::map<CliqueCutIdentifier, bool> CliquePool;

// For relaxation processing, access using (*vals)[c][p]
typedef boost::shared_ptr< std::vector< std::vector<float> > > RelaxationSummary;

// Handles cut management
class CutManagerI : public IloCplex::LazyConstraintCallbackI {
protected:
  bool active;
  int cutLevel;
  int cutUp;
  int totalCalls, frequency;
  int totalCutsAdded;
  int integerLB;
  TimetablingSolver& solver;
  IloCplex& cplex;
  CliquePool cliquePool;
  RelaxationSummary getRelaxationSummedOverRooms();
public:
  ILOCOMMONCALLBACKSTUFF(CutManager) 
    CutManagerI(IloEnv env, IloCplex &c, TimetablingSolver& s, int limit, int level) 
    : IloCplex::LazyConstraintCallbackI(env), cplex(c), solver(s), cutUp(limit), cutLevel(level) {
      active = true;
      totalCalls = 0; 
      frequency = 1; 
      totalCutsAdded = 0;
      integerLB = 0;
      std::cout << "Mycuts: Instantiating the cut manager ..." << std::endl;
  } 
  void main();
  void logProgress();
  bool genCutsFromObjIntegrality(RelaxationSummary vals);
  bool genCutsFromCliquePool(RelaxationSummary vals);
  bool genCutsFromTriangles(RelaxationSummary vals);
  bool genCutsFromPatterns(RelaxationSummary vals);
  bool genCutsFromMindaysChecks(RelaxationSummary vals);
  bool genCutsFromCurriculumChecks(RelaxationSummary vals);
};

IloCplex::Callback CutManager(IloEnv env, IloCplex &c, TimetablingSolver& s, int cutUp, int level);

// Should there be an implementation of:
// IloCplex::CallbackI* duplicateCallback() const { return (new (getEnv()) CutManagerI(*this)); } 

#endif // UDINE_CUT MANAGER
