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

#pragma warning(disable : 4018) 

#include <iostream>
#include <string>

#include <ilcplex/ilocplex.h>

#include "loader.h"
#include "solver.h"
#include "cut_manager.h"
#include "saver.h"

ILOSTLBEGIN

int main (int argc, char **argv) {

  if (argc < 2) { 
    std::cerr << "Usage: " << argv[0] << " <data> [cutUp]" << std::endl;
    std::cerr << "where: <data> is a path to an instance of Udine Timetabling and [cutUp] is an optional value of a known solution" << std::endl;
    exit(-1); 
  }
  string filename(argv[1]);

  IloEnv env;

  try {

    IloModel model(env);
    TimetablingInstance instance;
    instance.load(filename.c_str());

    bool isSubMIP = false;
    bool useCoursePeriods = false;
    TimetablingSolver solver(model, instance, isSubMIP, useCoursePeriods);

    solver.exportConfictGraph(filename.append(".dimacs").c_str());

    IloCplex cplex(model);
    filename = argv[1];
    cplex.exportModel(filename.append(".lp").c_str());

    // filename = argv[1];
    // solver.importSolution(cplex, instance, filename.append(".sol").c_str());

    filename = argv[1];
    ofstream file;
    file.open(filename.append(".log").c_str(), std::ofstream::out);
    file << ""; file.close();

    int cutUp = -1;
    int cutLevel = 5;
    if (argc >= 3) {
      istringstream convert(argv[2]);
      if (!(convert >> cutUp)) cutUp = -1;
      if (argc >= 4) {
        istringstream convert2(argv[3]);
        if (!(convert2 >> cutLevel)) cutLevel = 5;
      }
    }

    // 3 CPX_MIPEMPHASIS_BESTBOUND  Emphasize moving best bound  
    cplex.setParam(IloCplex::MIPEmphasis, 3);

    // 6 CPX_ALG_CONCURRENT  Concurrent (Dual, Barrier, and Primal) 
    // 4 CPX_ALG_BARRIER  Barrier 
    cplex.setParam(IloCplex::RootAlg, 6);
    // cplex.setParam(IloCplex::RootAlg, 4);

    // When NodeLim is set to 1, nodes are created but not solved.
    cplex.setParam(IloCplex::NodeLim, 0);
    cplex.setParam(IloCplex::TiLim, 7200);
    // CPLEX discards any solutions that are greater than the upper cutoff value.
    if (cutUp > 0) cplex.setParam(IloCplex::CutUp, cutUp);

    // Minor params
    cplex.setParam(IloCplex::RepeatPresolve, 3);
    cplex.setParam(IloCplex::Symmetry, 3);
    cplex.setParam(IloCplex::MIPInterval, 1);
    cplex.setParam(IloCplex::WriteLevel, 1);
    cplex.setParam(IloCplex::MIPDisplay, 4);
    cplex.setParam(IloCplex::PreDual, 1);

    // Other params worth experimenting with
    // cplex.setParam(IloCplex::PreDual, -1);
    // cplex.setParam(IloCplex::BrDir, 1);
    // cplex.setParam(IloCplex::VarSel, 3);
    // cplex.setParam(IloCplex::ZeroHalfCuts, 2);
    // cplex.setParam(IloCplex::PreslvNd, 2);

    // The in-built heuristics need to be disabled, as they are unaware of Type 1 cuts!
    cplex.setParam(IloCplex::HeurFreq, -1);
    cplex.setParam(IloCplex::RINSHeur, -1);
    cplex.setParam(IloCplex::FPHeur, -1);

    cplex.use(CutManager(env, cplex, solver, cutUp, cutLevel));
    cplex.use(IncumbentSaver(env, solver, argv[1]));

    env.out() << std::endl << "Solver: Running ..." << std::endl;
    cplex.solve();

    if (cplex.getSolnPoolNsolns() >= 1) {
      filename = argv[1];
      cplex.writeMIPStart(filename.append(".opt").c_str());
    }

    filename = argv[1];
    file.open(filename.append(".log").c_str(), std::ofstream::app);
    IloNum LB = cplex.getBestObjValue();
    if (LB < 0.001) LB = 0;
    file << env.getTime() << " " << cplex.getNnodes() << " " << LB << std::endl;
    file.close();  

    env.out() << std::endl << "Solver: ";
    if (cplex.getStatus() == IloAlgorithm::Feasible || 
      cplex.getStatus() == IloAlgorithm::Optimal) {
        env.out() << cplex.getStatus() << " ";
    } else {
      env.out() << "No ";
    }
    env.out() << "timetable found" << std::endl << std::endl;

  }
  catch (IloException& e) {
    std::cerr << "Solver: Concert exception caught: " << e << std::endl;
  }
  catch (...) {
    std::cerr << "Solver: Unknown exception caught" << std::endl;
  }

  env.end();

  return 0;
}
