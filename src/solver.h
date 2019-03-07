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

#ifndef UDINE_SOLVER
#define UDINE_SOLVER

#include <vector>
#include <set>
#include <map>
#include <utility>

#include <ilcplex/ilocplex.h>

#include "loader.h"
#include "conflicts.h"


ILOSTLBEGIN

struct TimetablingVariables {
  // The main array of decision variables
  IloArray< IloArray<IloNumVarArray> > x; // ... indexed first with periods, then rooms, then courses
  // Plus some auxiliary decision variables
  IloNumVarArray courseMinDayViolations; // ... indexed with courses 
  IloArray<IloNumVarArray> courseDays; // ... indexed with courses first, days second
  IloArray<IloNumVarArray> coursePeriods; // ... indexed with courses first, periods second
  IloArray<IloNumVarArray> courseRooms; // ... indexed with courses first, rooms second
  IloArray< IloArray<IloNumVarArray> > singletonChecks; // ... indexed with curricula first, days second, index last
  IloNumVarArray all; // a helper with all variables in a flat array
  TimetablingVariables(IloEnv env, TimetablingInstance &i, bool subMIP = false, bool useCoursePeriods = false);
};


class TimetablingSolver {

protected:
  TimetablingInstance &instance;
  IloEnv env;
  IloModel model;
  TimetablingVariables vars;
  IloRangeArray constraints;
  Graph conflictGraph;

  bool useCoursePeriods;

  virtual void generateConstraints(TimetablingInstance &i);
  virtual void generateCutsStatically(TimetablingInstance &i);
  virtual void generateObjective(TimetablingInstance &i);

public:
  TimetablingSolver(IloModel &modelToGenerate, TimetablingInstance &i, bool subMIP = false, bool coursePeriods = false) 
    : vars(modelToGenerate.getEnv(), i, subMIP, coursePeriods), 
    constraints(modelToGenerate.getEnv()), instance(i),
    useCoursePeriods(coursePeriods) {
      model = modelToGenerate;
      env = model.getEnv();

      generateConstraints(instance);
      generateObjective(instance);
      if (!subMIP) {
        conflictGraph.generateConflictGraph(instance);
        conflictGraph.generateAllCliques();
        generateCutsStatically(instance);
      }
  }

  // tries to import solution with the given filename, returns "success"
  virtual bool importSolution(IloCplex &cplex, TimetablingInstance &i, const char *filename);

  virtual void exportConfictGraph(const char *filename, const char *comment = "", bool binary = false) {
    conflictGraph.exportDimacs(filename, comment, binary);
  }

  virtual const TimetablingVariables &getVariables() { return vars; }

  friend class CutManagerI;
  friend class IncumbentSaverI;
};


#endif // UDINE_SOLVER
