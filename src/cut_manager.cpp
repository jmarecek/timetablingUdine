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

#include "cut_manager.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <ilcplex/ilocplex.h>

IloCplex::Callback CutManager(IloEnv env, IloCplex &c, TimetablingSolver& s, int limit, int level) {
  return (IloCplex::Callback(new (env) CutManagerI(env, c, s, limit, level)));
}

void CutManagerI::main() {

  // if (!active && totalCalls % 5 == 0) active = true; 
  // if (active) {

  RelaxationSummary vals = getRelaxationSummedOverRooms();

  bool thisTime = false;  // Did we get anything useful
  if (cutLevel >= 1) thisTime |= genCutsFromPatterns(vals);
  if (cutLevel >= 2) thisTime |= genCutsFromMindaysChecks(vals);
  if (cutLevel >= 2) thisTime |= genCutsFromCurriculumChecks(vals);
  // if (cutLevel > 3) thisTime |= genCutsFromCliquePool(vals);
  // if (cutLevel > 3 && !thisTime) thisTime |= genCutsFromTriangles(vals);
  if (cutLevel >= 3) thisTime |= genCutsFromObjIntegrality(vals);  

  active = thisTime;
  logProgress();
  totalCalls += 1;

  // FIX: CPLEX fails to terminate at the cutUp value, more often than not 
  if (  (cutUp > 0) 
    && (getBestObjValue() - std::floor(getBestObjValue()) > 0.01)
    && (std::ceil(getBestObjValue()) >= cutUp) ) abort();
}

RelaxationSummary CutManagerI::getRelaxationSummedOverRooms() {

  // Precompute sums of values in the current LP relaxation for all course-period combinations  
  std::vector<float> fresh(solver.instance.getPeriodCount(), 0);
  RelaxationSummary vals(new std::vector< std::vector<float> >(solver.instance.getCourseCount(), fresh));

  int c, p, r;  // course, period, room
  for(c = 0; c < solver.instance.getCourseCount(); c++) {
    for(p = 0; p < solver.instance.getPeriodCount(); p++) {
      float value = 0;
      for(r = 0; r < solver.instance.getRoomCount(); r++)
        value += getValue(solver.vars.x[p][r][c]);
      (*vals)[c][p] = value;
    }
  }
  return vals;
}

bool CutManagerI::genCutsFromObjIntegrality(RelaxationSummary vals) {

  int cuts = 0;

  // GLOBAL CUTS FROM THE WORST OBJECTIVE FROM ACTIVE NODES FIRST
  float diff = getBestObjValue() - std::floor(getBestObjValue());
  if (diff > 0.01 && diff < 0.99) {
    add(cplex.getObjective().getExpr() >= std::ceil(getBestObjValue()));
    cplex.setParam(IloCplex::CutLo, std::ceil(getBestObjValue()));
    std::cout << "Mycuts: Based on local LB of " << getObjValue() << " and global LB of " << getBestObjValue() << 
      ", added global cut from objective integrality: obj >= " << std::ceil(getBestObjValue()) << std::endl;
    cuts += 1;
  }

  // LOCAL CUTS FROM THE OBJECTIVE AT THE CURRENT NODE 
  diff = getObjValue() - std::floor(getObjValue());
  if (diff > 0.01 && diff < 0.99 && std::abs(getObjValue()-getBestObjValue()) > 0.01) {
    addLocal(cplex.getObjective().getExpr() >= std::ceil(getObjValue()));
    std::cout << "Mycuts: Based on local LB of " << getObjValue() << " and global LB of " << getBestObjValue() << 
      ", added local cut from objective integrality: obj >= " << std::ceil(getObjValue()) << std::endl;
    cuts += 1;
  }

  totalCutsAdded += cuts;
  return (cuts > 0);
}


bool CutManagerI::genCutsFromCliquePool(RelaxationSummary vals) {

  int cuts = 0;

  int p, clique, ci, r;  // period, clique and index within, room
  std::vector< std::vector<int> > &cs = solver.conflictGraph.cliques;
  std::vector<Vertex> &vs = solver.conflictGraph.vs;
  std::vector<Edge> &es = solver.conflictGraph.es;

  float value = 0;       // sum for the clique in the present LP relaxation
  for(p = 0; p < solver.instance.getPeriodCount(); p++)
    for(clique = 0; clique < cs.size(); clique++) {
      for(ci = 0; ci < cs.at(clique).size(); ci++)
        value += (*vals)[cs[clique][ci]][p];
      // Do you want to add the cut?
      if (value <= 1) continue;
      CliqueCutIdentifier id(p, clique);
      if (cliquePool.find(id) != cliquePool.end()) continue;
      IloExpr sum(solver.env);
      for(r = 0; r < solver.instance.getRoomCount(); r++)
        for(ci = 0; ci < cs.at(clique).size(); ci++)
          sum += solver.vars.x[p][r][cs[clique][ci]];
      IloConstraint cut(sum <= 1);
      add(cut);
      cuts += 1;
      cliquePool[id] = true;
      // std::cout << "Mycuts: Added a new clique cut (" << value << ")" << std::endl;
    }

    if (cuts > 0) { 
      totalCutsAdded += cuts;    
      std::cout << "Mycuts: Added " << cuts << " cut(s) from " << cs.size() << " pre-generated cliques in round " << totalCalls << std::endl;
      return true;
    } else return false;
}

bool CutManagerI::genCutsFromTriangles(RelaxationSummary vals) {

  int cuts = 0;

  int p, r;  // period, room
  std::vector< std::vector<int> > &cs = solver.conflictGraph.cliques;
  std::vector<Vertex> &vs = solver.conflictGraph.vs;
  std::vector<Edge> &es = solver.conflictGraph.es;

  for (int u = 0; u < vs.size(); u++)
    for (std::set<int>::iterator vi = vs[u].adj.begin(); vi != vs[u].adj.end(); vi++)
      if (u < (*vi))
        for (std::set<int>::iterator wi = vs[*vi].adj.begin(); wi != vs[*vi].adj.end(); wi++)
          if ((*vi < (*wi)) && (vs[(*wi)].adj.find(u) != vs[(*wi)].adj.end()))
            for(p = 0; p < solver.instance.getPeriodCount(); p++) {
              float value = (*vals)[u][p] + (*vals)[*vi][p] + (*vals)[*wi][p];
              if (value <= 1.01) continue;  // TODO: improve upon this
              IloExpr sum(solver.env);
              for(r = 0; r < solver.instance.getRoomCount(); r++)
                sum += solver.vars.x[p][r][u] + solver.vars.x[p][r][*vi] + solver.vars.x[p][r][*wi];
              IloConstraint cut(sum <= 1);
              // std::cout << "Mycuts: Added a violated triangle inequality (" << value << ")" << std::endl;
              cuts += 1;
              sum.end();
            }

            if (cuts > 0) { 
              totalCutsAdded += cuts;    
              std::cout << "Mycuts: Added " << cuts << " cut(s) from triangles in round " << totalCalls << std::endl;
              return true; 
            } else return false;
}


/*  Adds (most of the time redundant) cuts of the form:
forall (c in Courses, d in Days)
sum (p in HasPeriods[d], r in Rooms) 
Taught[p][r][c] <= 1 + CourseInfo[c].events + CourseMinDayViolations[c] - CourseInfo[c].minDays; 
*/
bool CutManagerI::genCutsFromMindaysChecks(RelaxationSummary vals) {

  int cuts = 0;

  int c, d, pd, r;  // course, day, period within a day, room
  std::vector< std::vector<int> > &cs = solver.conflictGraph.cliques;
  std::vector<Vertex> &vs = solver.conflictGraph.vs;
  std::vector<Edge> &es = solver.conflictGraph.es;

  for(c = 0; c < solver.instance.getCourseCount(); c++)
    for(d = 0; d < solver.instance.getDayCount(); d++) {
      float lhsValue = 0;
      for (pd = 0; pd < solver.instance.getPeriodsPerDayCount(); pd++) {
        lhsValue += (*vals)[c][d * solver.instance.getPeriodsPerDayCount() + pd];
      }
      IloExpr rhsExpr(solver.env);
      try {
        rhsExpr += 1 + solver.instance.getCourse(c).lectures - solver.instance.getCourse(c).minWorkingDays
          + solver.vars.courseMinDayViolations[c];
        if (lhsValue > getValue(rhsExpr) + 0.001) { 
          IloExpr lhsExpr(solver.env);
          for (pd = 0; pd < solver.instance.getPeriodsPerDayCount(); pd++)
            for (r = 0; r < solver.instance.getRoomCount(); r++)
              lhsExpr += solver.vars.x[d * solver.instance.getPeriodsPerDayCount() + pd][r][c];
          add(lhsExpr <= rhsExpr);
          cuts += 1;
          lhsExpr.end();
        }
      } catch (...) { /* variable pre-processed away */ }
      rhsExpr.end();
    }    

    if (cuts > 0) { 
      totalCutsAdded += cuts;    
      std::cout << "Mycuts: Added " << cuts << " cut(s) from mindays checks in round " << totalCalls << std::endl;
      return true; 
    } else return false;
}


/*  Adds (most of the time unnecessary) cuts of the form:
*    forall (cu in Curricula)
*    sum (c in CurriculumHasCourses[cu], p in Periods, r in Rooms) 
*    Taught[p][r][c] == CurriculumHasEventsCount[cu]; 
*/
bool CutManagerI::genCutsFromCurriculumChecks(RelaxationSummary vals) {

  int cuts = 0;

  int u, ui, p, r;  // index within a curriculum, course, period, room
  std::vector< std::vector<int> > &cs = solver.conflictGraph.cliques;
  std::vector<Vertex> &vs = solver.conflictGraph.vs;
  std::vector<Edge> &es = solver.conflictGraph.es;

  for(u = 0; u < solver.instance.getCurriculumCount(); u++) {

    float has = 0;
    float shouldHave = 0;
    for (ui = 0; ui < solver.instance.getCurriculum(u).courseIds.size(); ui++) {
      IloInt c = solver.instance.getCurriculum(u).courseIds.at(ui);
      shouldHave += solver.instance.getCourse(c).lectures;
      for(p = 0; p < solver.instance.getPeriodCount(); p++)
        has += (*vals)[c][p];
    }

    if (std::fabs(shouldHave - has) > 0.001) {
      IloExpr expr(solver.env);
      for (ui = 0; ui < solver.instance.getCurriculum(u).courseIds.size(); ui++) {
        IloInt c = solver.instance.getCurriculum(u).courseIds.at(ui);
        for(p = 0; p < solver.instance.getPeriodCount(); p++)
          for(r = 0; r < solver.instance.getRoomCount(); r++)    
            expr += solver.vars.x[p][r][c];
      }
      add(expr == shouldHave);      
      cuts += 1;
      expr.end();
    }
  }      

  if (cuts > 0) { 
    totalCutsAdded += cuts;    
    std::cout << "Mycuts: Added " << cuts << " cut(s) from curricul checks in round " << totalCalls << std::endl;
    return true; 
  } else return false;
}


bool CutManagerI::genCutsFromPatterns(RelaxationSummary vals) {

  int cuts = 0;
  PatternDB patterns = solver.instance.getPatterns();

  int r, u, ui, d, pd;  // room, curriculum, index within, day, period within
  int pati;             // pati for index within patterns
  for(u = 0; u < solver.instance.getProperCurriculumCount(); u++)
    for(d = 0; d < solver.instance.getDayCount(); d++) {
      IloExpr rhsExpr(solver.env);
      try {
        // Get the current penalties, which are fixed across all patterns
        // for(s = 0; s < solver.instance.getCheckCount(); s++)
        rhsExpr += solver.vars.singletonChecks[u][d][0];
        float rhs = getValue(rhsExpr);

        // Compute the LHS for each pattern
        for (pati = 0; pati < patterns.size(); pati++) {
          float lhs = 0;
          for (pd = 0; pd < solver.instance.getPeriodsPerDayCount(); pd++) {
            IloInt p = d * solver.instance.getPeriodsPerDayCount() + pd;
            float factor = 0;
            for (ui = 0; ui < solver.instance.getCurriculum(u).courseIds.size(); ui++)
              factor += (*vals)[solver.instance.getCurriculum(u).courseIds.at(ui)][p];
            lhs += patterns[pati].coefs[pd] * factor;
          }
          lhs += 1 - patterns[pati].rhs;
          lhs *= patterns[pati].penalty;

          // If the constraint is violated, add the cut
          if (lhs - rhs > 0.001) {
            // std::cout << "Mycuts: Violation of pattern cut for curriculum " << u << " day " << d << " ..." << std::endl;

            IloExpr lhsExpr(solver.env);
            for (ui = 0; ui < solver.instance.getCurriculum(u).courseIds.size(); ui++) {
              IloInt c = solver.instance.getCurriculum(u).courseIds.at(ui);
              for (pd = 0; pd < solver.instance.getPeriodsPerDayCount(); pd++) {
                IloExpr factor(solver.env);
                IloInt p = d * solver.instance.getPeriodsPerDayCount() + pd;
                for (r = 0; r < solver.instance.getRoomCount(); r++)
                  factor += solver.vars.x[p][r][c];
                lhsExpr += patterns[pati].coefs[pd] * factor;
              }
            }
            lhsExpr += 1 - patterns[pati].rhs;
            lhsExpr *= patterns[pati].penalty;
            IloConstraint cut(lhsExpr - rhsExpr <= 0);
            add(cut);
            lhsExpr.end();
            cuts += 1; 
          }

        }      

      } catch (...) {
        std::cerr << "Solver: An exception intercepted." << std::endl;
      }
      rhsExpr.end();
    }

    if (cuts > 0) { 
      totalCutsAdded += cuts;    
      std::cout << "Mycuts: Added " << cuts << " cut(s) from patterns in round " << totalCalls << std::endl;
      return true; 
    } else { return false; }
}


void CutManagerI::logProgress() {
  std::ofstream file;
  std::string filename = solver.instance.getFilename();
  filename.append(".log");
  file.open(filename.c_str(), std::ofstream::app);
  IloNum LB = getBestObjValue();
  if (LB < 0.001) LB = 0;
  file << getEnv().getTime() << " " << getNnodes() << " " << LB << std::endl;
  file.close();  
}