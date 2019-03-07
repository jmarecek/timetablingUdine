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
#include "solver.h"

TimetablingVariables::TimetablingVariables(IloEnv env, TimetablingInstance &i, bool subMIP, bool useCoursePeriods)
: x(IloArray< IloArray<IloNumVarArray> >(env, i.getPeriodCount())),
courseMinDayViolations(IloNumVarArray(env, i.getCourseCount(), 0.0, i.getDayCount(), IloNumVar::Int)),
courseDays(IloArray<IloNumVarArray>(env, i.getCourseCount())),
courseRooms(IloArray<IloNumVarArray>(env, i.getCourseCount())),
coursePeriods(IloArray<IloNumVarArray>(env, i.getCourseCount())),
singletonChecks(IloArray< IloArray<IloNumVarArray> >(env, i.getCurriculumCount())),
all(env)
{

  int p, d, r, c, u;  // periods, days, rooms, courses, curricula

  // Initialize the core decision variables
  for (p = 0; p < i.getPeriodCount(); p++) {
    IloArray<IloNumVarArray> forPeriod = IloArray<IloNumVarArray>(env, i.getRoomCount());
    for (r = 0; r < i.getRoomCount(); r++) {  
      IloNumVarArray forRoom(env);
      for (c = 0; c < i.getCourseCount(); c++) {  
        std::stringstream name;
        name << "xP" << p << "R" << r << "C" << c;
        // if (subMIP) IloNumVar var(env, 0, 1, IloNumVar::Float, name.str().c_str());
        IloNumVar var(env, 0, 1, IloNumVar::Bool, name.str().c_str());
        forRoom.add(var);
        all.add(var);
      }
      forPeriod[r] = forRoom;
    }
    x[p] = forPeriod;
  }

  // Initialize some auxiliary decision variables
  for (c = 0; c < i.getCourseCount(); c++) {
    IloNumVarArray forCourse(env);
    for (d = 0; d < i.getDayCount(); d++) {  
      std::stringstream name;
      name << "CourseDayC" << c << "D" << d;
      IloNumVar var(env, 0, 1, IloNumVar::Bool, name.str().c_str());
      forCourse.add(var);
      all.add(var);
    }
    courseDays[c] = forCourse;
  }

  if (useCoursePeriods)
  for (c = 0; c < i.getCourseCount(); c++) {
    IloNumVarArray forCourse(env);
    for (p = 0; p < i.getPeriodCount(); p++) {  
      std::stringstream name;
      name << "CoursePeriodC" << c << "P" << p;
      IloNumVar var(env, 0, 1, IloNumVar::Bool, name.str().c_str());
      forCourse.add(var);
      all.add(var);
    }
    coursePeriods[c] = forCourse;
  }

  for (c = 0; c < i.getCourseCount(); c++) {
    IloNumVarArray forCourse(env);
    for (r = 0; r < i.getRoomCount(); r++) {  
      std::stringstream name;
      name << "CourseRoomC" << c << "R" << r;
      IloNumVar var(env, 0, 1, IloNumVar::Bool, name.str().c_str());
      forCourse.add(var);
      all.add(var);
    }
    courseRooms[c] = forCourse;
  }

  for (u = 0; u < i.getCurriculumCount(); u++) {
    IloArray<IloNumVarArray> forCurriculum = IloArray<IloNumVarArray>(env, i.getDayCount());
    for (d = 0; d < i.getDayCount(); d++) {  
      IloNumVarArray forDay(env);
      std::stringstream name;
      name << "PatU" << u << "D" << d;
      IloNumVar var(env, 0, i.getPeriodsPerDayCount(), IloNumVar::Int, name.str().c_str());
      forDay.add(var);
      all.add(var);
      forCurriculum[d] = forDay;
    }
    singletonChecks[u] = forCurriculum;
  }
}


void TimetablingSolver::generateConstraints(TimetablingInstance &i) {

  int p, d, r, c;  // periods, days, rooms, courses
  int u, ui;       // curricula, index within curriculum's list of courseIds
  int f;           // restrictions

  // Allocate the right amount of events for each course
  for (c = 0; c < i.getCourseCount(); c++) {
    IloExpr sum(env);
    for (p = 0; p < i.getPeriodCount(); p++)
      for (r = 0; r < i.getRoomCount(); r++)
        sum += vars.x[p][r][c];
    constraints.add(sum == i.getCourse(c).lectures);
    sum.end();
  }

  // No two lectures can take place in the same room in the same period
  for (p = 0; p < i.getPeriodCount(); p++)
    for (r = 0; r < i.getRoomCount(); r++)
      constraints.add(IloSum(vars.x[p][r]) <= 1);

  // No two lectures of a single course can take place in the same room in the same period
  for (p = 0; p < i.getPeriodCount(); p++)
    for (c = 0; c < i.getCourseCount(); c++) {
      IloExpr sum(env);
      for (r = 0; r < i.getRoomCount(); r++)
        sum += vars.x[p][r][c];
      constraints.add(sum <= 1);
      sum.end();
    }

    // Lectures in one curriculum must be scheduled at different times
    for (p = 0; p < i.getPeriodCount(); p++)
      for (u = 0; u < i.getCurriculumCount(); u++) {
        IloExpr sum(env);
        for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
          IloInt c = i.getCurriculum(u).courseIds.at(ui);
          for (r = 0; r < i.getRoomCount(); r++)
            sum += vars.x[p][r][c];
        }
        constraints.add(sum <= 1);
        sum.end();
      }

      // Teachers might be not available during some periods
      {
        IloExpr sum(env);
        for (f = 0; f < i.getRestrictionCount(); f++) {
          IloInt c = i.getRestriction(f).courseId;
          IloInt p = i.getRestriction(f).period;
          for (r = 0; r < i.getRoomCount(); r++)
            sum += vars.x[p][r][c];
        }
        constraints.add(sum == 0);
        sum.end();
      }

      // (SOFT) The lectures of each course should be held all in a single room
      // Mark the rooms where the lectures of the course are held
      for (c = 0; c < i.getCourseCount(); c++)
        for (r = 0; r < i.getRoomCount(); r++)
          for (p = 0; p < i.getPeriodCount(); p++)
            constraints.add(vars.courseRooms[c][r] - vars.x[p][r][c] >= 0);
      // NEW: Andrew's bound
      for (c = 0; c < i.getCourseCount(); c++)
        for (r = 0; r < i.getRoomCount(); r++) {
          IloExpr sum(env);
          for (p = 0; p < i.getPeriodCount(); p++)
            sum += vars.x[p][r][c];
          constraints.add(i.getCourse(c).lectures * vars.courseRooms[c][r] - sum >= 0);
          sum.end();
        }
        for (c = 0; c < i.getCourseCount(); c++)
          for (r = 0; r < i.getRoomCount(); r++) {
            IloExpr sum(env);
            for (p = 0; p < i.getPeriodCount(); p++)
              sum += vars.x[p][r][c];
            constraints.add(sum - vars.courseRooms[c][r] >= 0);
            sum.end();
          }
          // (CUT) Implied bounds
          for (c = 0; c < i.getCourseCount(); c++) {
            IloExpr sum(env);
            for (r = 0; r < i.getRoomCount(); r++)
              sum += vars.courseRooms[c][r];      
            constraints.add(sum >= 1);
            sum.end();
          }

          // Mark the periods where the lectures of the course are held
          if (useCoursePeriods) {
            for (c = 0; c < i.getCourseCount(); c++)
              for (r = 0; r < i.getRoomCount(); r++)
                for (p = 0; p < i.getPeriodCount(); p++)
                  constraints.add(vars.coursePeriods[c][p] - vars.x[p][r][c] >= 0);
            for (c = 0; c < i.getCourseCount(); c++)
              for (p = 0; p < i.getPeriodCount(); p++) {
                IloExpr sum(env);
                for (r = 0; r < i.getRoomCount(); r++) 
                  sum += vars.x[p][r][c];
                constraints.add(sum - vars.coursePeriods[c][p] >= 0);
                sum.end();
              }
              // Implied bounds
              for (c = 0; c < i.getCourseCount(); c++) {
                IloExpr sum(env);
                for (p = 0; p < i.getPeriodCount(); p++)
                  sum += vars.coursePeriods[c][p];      
                constraints.add(sum >= 1);
                sum.end();
              }
          }

            // (SOFT) The lectures of each course should be spread onto a given minimum number of days
            // Mark the days when the course has lectures in the schedule
            for (c = 0; c < i.getCourseCount(); c++)
              for (d = 0; d < i.getDayCount(); d++)
                for (p = d * i.getPeriodsPerDayCount();
                  p < (d + 1) * i.getPeriodsPerDayCount(); p++) {
                    IloExpr sum(env);
                    for (r = 0; r < i.getRoomCount(); r++) 
                      sum += vars.x[p][r][c];
                    constraints.add(sum - vars.courseDays[c][d] <= 0);
                    sum.end();
                    if (useCoursePeriods)
                      constraints.add(vars.coursePeriods[c][p] - vars.courseDays[c][d] <= 0);
                }
                for (c = 0; c < i.getCourseCount(); c++)
                  for (d = 0; d < i.getDayCount(); d++) {
                    IloExpr sum(env);
                    IloExpr sumConcise(env);
                    for (p = d * i.getPeriodsPerDayCount();
                      p < (d + 1) * i.getPeriodsPerDayCount(); p++) {
                        for (r = 0; r < i.getRoomCount(); r++) 
                          sum += vars.x[p][r][c];
                        if (useCoursePeriods)
                          sumConcise += vars.coursePeriods[c][p];
                    }
                    constraints.add(sum - vars.courseDays[c][d] >= 0);
                    if (useCoursePeriods)
                      constraints.add(sumConcise - vars.courseDays[c][d] >= 0);
                    sum.end();
                    sumConcise.end();
                  }
                  // Count the number of days the course has lectures.
                  for (c = 0; c < i.getCourseCount(); c++) {
                    IloExpr sum(env);
                    for (d = 0; d < i.getDayCount(); d++)
                      sum += vars.courseDays[c][d];
                    constraints.add(sum + vars.courseMinDayViolations[c] - i.getCourse(c).minWorkingDays >= 0);
                    sum.end();
                  }
                  // (CUT) Implied bounds
                  for (c = 0; c < i.getCourseCount(); c++) {
                    IloExpr sum(env);
                    for (d = 0; d < i.getDayCount(); d++)
                      sum += vars.courseDays[c][d];
                    constraints.add(sum >= 1);
                    sum.end();
                  }
                  // (CUT) Implied bounds
                  for (c = 0; c < i.getCourseCount(); c++)
                    constraints.add(vars.courseMinDayViolations[c] <= i.getCourse(c).minWorkingDays - 1);
                  // (CUT) Implied bounds
                  for (c = 0; c < i.getCourseCount(); c++)
                    for (d = 0; d < i.getDayCount(); d++) {
                      IloExpr sum(env);
                      IloExpr sumConcise(env);
                      for (p = d * i.getPeriodsPerDayCount(); p < (d + 1) * i.getPeriodsPerDayCount(); p++) {
                        for (r = 0; r < i.getRoomCount(); r++) 
                          sum += vars.x[p][r][c];
                        if (useCoursePeriods)
                          sumConcise += vars.coursePeriods[c][p];
                      }
                      constraints.add(sum + i.getCourse(c).minWorkingDays - i.getCourse(c).lectures - vars.courseMinDayViolations[c] <= 1);
                      if (useCoursePeriods)
                        constraints.add(sumConcise + i.getCourse(c).minWorkingDays - i.getCourse(c).lectures - vars.courseMinDayViolations[c] <= 1);
                      sum.end();
                      sumConcise.end();
                    }

                    // Count the number of isolated lectures in timetables of individual curricula
                    // First check if there is an isolated lecture during the first or the last period
                    for (u = 0; u < i.getProperCurriculumCount(); u++)
                      for (d = 0; d < i.getDayCount(); d++) {
                        IloExpr sumMorning(env);
                        IloExpr sumEvening(env);
                        IloExpr sumMorningConcise(env);
                        IloExpr sumEveningConcise(env);
                        for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
                          IloInt c = i.getCurriculum(u).courseIds.at(ui);
                          p = d * i.getPeriodsPerDayCount();
                          for (r = 0; r < i.getRoomCount(); r++)
                            sumMorning += vars.x[p][r][c] - vars.x[p + 1][r][c];
                          if (useCoursePeriods)
                            sumMorningConcise += vars.coursePeriods[c][p] - vars.coursePeriods[c][p + 1];
                          p = (d + 1) * i.getPeriodsPerDayCount() - 1;
                          for (r = 0; r < i.getRoomCount(); r++)
                            sumEvening += vars.x[p][r][c] - vars.x[p - 1][r][c];
                          if (useCoursePeriods)
                            sumEveningConcise += vars.coursePeriods[c][p] - vars.coursePeriods[c][p - 1];
                        }
                        constraints.add(sumMorning - vars.singletonChecks[u][d][0] <= 0);
                        constraints.add(sumEvening - vars.singletonChecks[u][d][0] <= 0);
                        sumMorning.end();
                        sumEvening.end();
                        if (useCoursePeriods) {
                          constraints.add(sumMorningConcise - vars.singletonChecks[u][d][0] <= 0);
                          constraints.add(sumEveningConcise - vars.singletonChecks[u][d][0] <= 0);
                          sumMorningConcise.end();
                          sumEveningConcise.end();
                        }
                      }
                      // Then check the remaining periods
                      for (u = 0; u < i.getProperCurriculumCount(); u++)
                        for (d = 0; d < i.getDayCount(); d++)
                          for (p = d * i.getPeriodsPerDayCount() + 1;
                            p < (d + 1) * i.getPeriodsPerDayCount() - 1; p++) {
                              IloExpr sumInbetween(env);     
                              IloExpr sumInbetweenConcise(env);     
                              for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
                                IloInt c = i.getCurriculum(u).courseIds.at(ui);
                                for (r = 0; r < i.getRoomCount(); r++)
                                  sumInbetween += vars.x[p][r][c] - vars.x[p + 1][r][c] - vars.x[p - 1][r][c];
                                if (useCoursePeriods)                                
                                  sumInbetweenConcise += vars.coursePeriods[c][p] - vars.coursePeriods[c][p + 1] - vars.coursePeriods[c][p - 1];
                              }
                              int s = p - d * i.getPeriodsPerDayCount() + 1;
                              constraints.add(sumInbetween - vars.singletonChecks[u][d][0] <= 0);
                              sumInbetween.end();
                              if (useCoursePeriods) {
                                constraints.add(sumInbetweenConcise - vars.singletonChecks[u][d][0] <= 0);
                                sumInbetweenConcise.end();
                              }
                          }

                          model.add(constraints);
}  // END TimetablingSolver::generateConstraints


// A little debugging helper
void TimetablingSolver::generateCutsStatically(TimetablingInstance &i) {

  bool minDays = false; // false;
  bool curriculumChecks = false; // false;
  bool patternsEnumeration = true;
  bool triangles = false;
  bool cliquePool = false;

  // bool CutManagerI::genCutsFromMindaysChecks(RelaxationSummary vals) {
  if (minDays) {
    int c, d, pd, r;
    for(c = 0; c < i.getCourseCount(); c++)
      for(d = 0; d < i.getDayCount(); d++) {
        IloExpr rhsExpr(env);
        try {
          rhsExpr += 1 + i.getCourse(c).lectures - i.getCourse(c).minWorkingDays
            + vars.courseMinDayViolations[c];
          IloExpr lhsExpr(env);
          for (pd = 0; pd < i.getPeriodsPerDayCount(); pd++)
            for (r = 0; r < i.getRoomCount(); r++)
              lhsExpr += vars.x[d * i.getPeriodsPerDayCount() + pd][r][c];
          model.add(lhsExpr <= rhsExpr);
          lhsExpr.end();
        } catch (...) { /* variable pre-processed away */ }
        rhsExpr.end();
      }
  }

  // bool CutManagerI::genCutsFromCurriculumChecks(RelaxationSummary vals) {
  if (curriculumChecks) {
    int u, ui, p, r;

    for(u = 0; u < i.getCurriculumCount(); u++) {
      float shouldHave = 0;
      for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
        IloInt c = i.getCurriculum(u).courseIds.at(ui);
        shouldHave += i.getCourse(c).lectures;
      }

      IloExpr expr(env);
      for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
        IloInt c = i.getCurriculum(u).courseIds.at(ui);
        for(p = 0; p < i.getPeriodCount(); p++)
          for(r = 0; r < i.getRoomCount(); r++)    
            expr += vars.x[p][r][c];
      }
      model.add(expr == shouldHave);      
      expr.end();
    }      
  }

  if (patternsEnumeration) {
    PatternDB patterns = i.getPatterns();

    int r, u, ui, d, pd;
    int pati;

    for(u = 0; u < i.getProperCurriculumCount(); u++)
      for(d = 0; d < i.getDayCount(); d++)
        for (pati = 0; pati < patterns.size(); pati++) {

          IloExpr sum(env);
          IloExpr sumConcise(env);

          for (pd = 0; pd < i.getPeriodsPerDayCount(); pd++) {
            IloInt p = d * i.getPeriodsPerDayCount() + pd;
            for (ui = 0; ui < i.getCurriculum(u).courseIds.size(); ui++) {
              IloInt c = i.getCurriculum(u).courseIds.at(ui);
              for (r = 0; r < i.getRoomCount(); r++) 
                sum += patterns[pati].coefs[pd] * vars.x[p][r][c];
              if (useCoursePeriods)
                sumConcise += patterns[pati].coefs[pd] * vars.coursePeriods[c][p];
            }
          }

          model.add( patterns[pati].penalty * (1 - patterns[pati].rhs + sum) - vars.singletonChecks[u][d][0] <= 0);
          sum.end();

          if (useCoursePeriods) {
            model.add( patterns[pati].penalty * (1 - patterns[pati].rhs + sumConcise) - vars.singletonChecks[u][d][0] <= 0);
            sumConcise.end();
          }
        }
  }

  if (cliquePool) {
    int p, clique, ci, r;
    std::vector< std::vector<int> > &cs = conflictGraph.cliques;
    std::vector<Vertex> &vs = conflictGraph.vs;
    std::vector<Edge> &es = conflictGraph.es;

    for(p = 0; p < i.getPeriodCount(); p++)
      for(clique = 0; clique < cs.size(); clique++) {
        IloExpr sum(env);
        IloExpr sumConcise(env);
        for(ci = 0; ci < cs.at(clique).size(); ci++) {
          for(r = 0; r < i.getRoomCount(); r++)
            sum += vars.x[p][r][cs[clique][ci]];
          if (useCoursePeriods)
            sumConcise += vars.coursePeriods[cs[clique][ci]][p];
        }
        model.add(sum <= 1);
        sum.end();
        if (useCoursePeriods) {
          model.add(sumConcise <= 1);
          sumConcise.end();
        }
      }
  }

  if (triangles) {
    int p, r;
    std::vector< std::vector<int> > &cs = conflictGraph.cliques;
    std::vector<Vertex> &vs = conflictGraph.vs;
    std::vector<Edge> &es = conflictGraph.es;

    for (int u = 0; u < vs.size(); u++)
      for (std::set<int>::iterator vi = vs[u].adj.begin(); vi != vs[u].adj.end(); vi++)
        if (u < (*vi))
          for (std::set<int>::iterator wi = vs[*vi].adj.begin(); wi != vs[*vi].adj.end(); wi++)
            if ((*vi < (*wi)) && (vs[(*wi)].adj.find(u) != vs[(*wi)].adj.end()))
              for(p = 0; p < i.getPeriodCount(); p++) {
                IloExpr sum(env);
                for(r = 0; r < i.getRoomCount(); r++)
                  sum += vars.x[p][r][u] + vars.x[p][r][*vi] + vars.x[p][r][*wi];
                model.add(sum <= 1);
                sum.end();
              }  
  }

} // end of TimetablingSolver::generateCutsStatically


void TimetablingSolver::generateObjective(TimetablingInstance &i) {

  int p, d, r, c, u;  // periods, days, rooms, courses, curricula

  IloExpr obj(env);
  // Sum up the the number of missing seats ...
  for (r = 0; r < i.getRoomCount(); r++)
    for (c = 0; c < i.getCourseCount(); c++)
      if (i.getCourse(c).students > i.getRoom(r).capacity)
        for (p = 0; p < i.getPeriodCount(); p++)
          obj += vars.x[p][r][c] * (i.getCourse(c).students - i.getRoom(r).capacity);
  // ... add the number of rooms used on the top of a single room per course
  for (c = 0; c < i.getCourseCount(); c++)
    for (r = 0; r < i.getRoomCount(); r++)
      obj += vars.courseRooms[c][r];
  obj -= i.getCourseCount();
  // ... add the number of gaps in timetables for individual curricula
  for (u = 0; u < i.getProperCurriculumCount(); u++)
    for (d = 0; d < i.getDayCount(); d++)
      // for (s = 0; s < i.getCheckCount(); s++)
      obj += 2 * vars.singletonChecks[u][d][0];
  // ... add the number of missing days of instruction
  for (c = 0; c < i.getCourseCount(); c++)
    obj += 5 * vars.courseMinDayViolations[c];
  // ... and minimize the total
  model.add(IloMinimize(env, obj));
} // END TimetablingSolver::generateObjective


// tries to import solution with the given filename, returns "success"
bool TimetablingSolver::importSolution(IloCplex &cplex, TimetablingInstance &i, const char *filename){
  std::cout << "Loader: Looking for solution " << filename << " to warm-start with ... " << std::endl; 

  IloNumVarArray setVars(env);
  IloNumArray setVals(env);

  try {
    std::ifstream file;
    file.open(filename, std::ifstream::in);

    std::string cname;
    std::string rname;    
    int d, pwd, p, r, c, cnt;

    if (file.bad() || file.eof()) {
      return false;
    }

    cnt = 0;

    while (file.good() && cnt++ < i.getEventCount()) {

      file >> cname;
      // we use std::map< std::string, int, std::less<std::string> > cnames;
      c = i.getCourseId(cname);
      if (c < 0) break;

      if (!file.good()) break;
      file >> rname;
      r = i.getRoomId(rname.substr(1));
      if (r < 0) break;

      if (!file.good()) break;
      file >> d;
      if (!file.good()) break;
      file >> pwd;
      p = d * i.getPeriodsPerDayCount() + pwd;

      setVars.add(vars.x[p][r][c]);
      setVals.add(1);
    }

    file.close();

    if (setVars.getSize() > 0) {

      // NOTE: Cplex warmstarting sucks. In theory, the following should suffice:
      cplex.setParam(IloCplex::AdvInd, 1);
      cplex.addMIPStart(setVars, setVals, IloCplex::MIPStartSolveMIP, filename);
      // cplex.setVectors(setVals, 0, setVars, 0, 0, 0);
      return true;
    }

  } 
  catch (IloException& e) { std::cerr << "Solver (Warmstart): Concert exception caught: " << e << std::endl; }
  catch (...) { std::cerr << "Solver (Warmstart): Unknown exception caught." << std::endl; }
  return false;
}
