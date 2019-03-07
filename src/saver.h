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

#ifndef UDINE_SAVER
#define UDINE_SAVER

#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


#include <ilcplex/ilocplex.h>

#include "solver.h"

ILOSTLBEGIN

class IncumbentSaverI : public IloCplex::IncumbentCallbackI {

protected:
  IloEnv env;
  TimetablingSolver& solver;
  char *saveToPath;

public:
  ILOCOMMONCALLBACKSTUFF(IncumbentSaver)

    IncumbentSaverI(IloEnv env, TimetablingSolver& s, char *path)
    : IloCplex::IncumbentCallbackI(env), solver(s), saveToPath(path) {
  }

  inline int roundProperly(double x) { return int(std::floor(x + 0.5f)); }

  void main() {
    try {

      std::stringstream path;
      path << saveToPath << "." << getObjValue() << ".sol";
      std::ofstream solFile(path.str().c_str(), ios::app);

      int c, p, r;  // course, period, room
      for(c = 0; c < solver.instance.getCourseCount(); c++)
        for(p = 0; p < solver.instance.getPeriodCount(); p++)
          for(r = 0; r < solver.instance.getRoomCount(); r++)
            if (getValue(solver.vars.x[p][r][c]) >= 0.999)
              //solFile << "Course " << solver.instance.getCourse(c).name << " room " << solver.instance.getRoom(r).name << " period " << p << std::endl;
              // InfArcBib r10 0 0
              solFile << solver.instance.getCourse(c).name << " " 
              << "r" << solver.instance.getRoom(r).name << " "
              << int(std::floor(float(p / solver.instance.getPeriodsPerDayCount()))) << " "
              << int(p % solver.instance.getPeriodsPerDayCount()) << std::endl;

      // check that all the type 1 cuts required were applied 
      if (std::abs(getObjValue() - getValidObjValue(solFile)) > 0.01)
        reject();

      solFile.close();
    }
    catch (IloException& e) { std::cerr << "Concert error: " << e << std::endl; }
    catch (...) { std::cerr << "Unknown error: " << std::endl; }
  }

  // Adapted from the code of di Gaspero and Schaerf
  int getValidObjValue(std::ofstream &os) {
    int x = 0; // return value
    try {

      TimetablingInstance &in = solver.instance;
      std::cout << "Solver: Trying to validate the solution found ... " << std::endl;

      // helper vars
      unsigned c, p, r, d, u;
      int ppd = in.getPeriodsPerDayCount();
      std::vector<int> distinctDays(in.getCourseCount(), 0);
      for (c = 0; c < in.getCourseCount(); c++)
        for (d = 0; d < in.getPeriodCount() / ppd; d++) {
          int i = 0;
          for (p = 0; p < ppd; p++) 
            for(r = 0; r < in.getRoomCount(); r++)
              i = std::max(i, roundProperly(getValue(solver.vars.x[p][r][c])));
          distinctDays[c] += i;
        }
        std::vector<int> distinctRooms(in.getCourseCount(), 0);   
        for(c = 0; c < in.getCourseCount(); c++)
          for(r = 0; r < in.getRoomCount(); r++) {
            int i = 0;
            for (p = 0; p < in.getPeriodCount(); p++)
              i = std::max(i, roundProperly(getValue(solver.vars.x[p][r][c])));
            distinctRooms[c] += i;
          }
          std::vector<int> fresh(solver.instance.getPeriodCount(), 0);
          std::vector< std::vector<int> > timetable(solver.instance.getCurriculumCount(), fresh);
          for (p = 0; p < in.getPeriodCount(); p++)
            for (u = 0; u < in.getCurriculumCount(); u++) {
              int i = 0;
              for (int ui = 0; ui < in.getCurriculum(u).courseIds.size(); ui++)
                for (r = 0; r < in.getRoomCount(); r++)
                  i = std::max(i, roundProperly(getValue(solver.vars.x[p][r][in.getCurriculum(u).courseIds.at(ui)])));
              timetable[u][p] = i;
            }

            // PrintViolationsOnRoomCapacity
            for (c = 0; c < in.getCourseCount(); c++)
              for (p = 0; p < in.getPeriodCount(); p++)
                if (r != 0 && in.getRoom(r).capacity < in.getCourse(c).students) {
                  os << "[S(" << in.getCourse(c).students - in.getRoom(r).capacity 
                    << ")] Room " << in.getRoom(r).name << " too small for course " 
                    << in.getCourse(c).name << " the period " 
                    << p << " (day " << p/ppd << ", timeslot " 
                    << p % ppd << ")" << std::endl;
                  x += in.getCourse(c).students - in.getRoom(r).capacity;
                }

                // PrintViolationsOnMinWorkingDays(std::ostream& os) const
                for (c = 0; c < in.getCourseCount(); c++)
                  if (distinctDays[c] < in.getCourse(c).minWorkingDays) {
                    os << "[S(" << 5 << ")] The course " 
                      << in.getCourse(c).name << " has only " << distinctDays[c]
                    << " days of lecture" << std::endl;
                    x += 5;
                  }

                  // PrintViolationsOnIsolatedLectures(std::ostream& os) const
                  for (u = 0; u < in.getCurriculumCount(); u++)
                    for (p = 0; p < in.getPeriodCount(); p++)
                      if (timetable[u][p] > 0)
                        if ((p % ppd == 0 && timetable[u][p+1] == 0)
                          || (p % ppd == ppd-1 && timetable[u][p-1] == 0)
                          || (timetable[u][p+1] == 0 && timetable[u][p-1] == 0)) {
                            os << "[S(" << 2 << ")] Curriculum " 
                              << in.getCurriculum(u).name << " has an isolated lecture at period " 
                              << p << " (day " << p/ppd << ", timeslot " << p % ppd << ")" << std::endl;
                            x += 2;
                        }

                        // PrintViolationsOnRoomStability(std::ostream& os) const
                        for (c = 0; c < in.getCourseCount(); c++)
                          if (distinctRooms[c] > 1) {
                            os << "[S(" << distinctRooms[c] - 1 << ")] Course " 
                              << in.getCourse(c).name << " uses " << distinctRooms[c] << " different rooms" 
                              << std::endl;
                            x += (distinctRooms[c] - 1);
                          }
    }
    catch (IloException& e) { std::cerr << "Concert error: " << e << std::endl; }
    catch (...) { std::cerr << "Unknown error: " << std::endl; }

    return x;
  }

};

IloCplex::Callback IncumbentSaver(IloEnv env, TimetablingSolver& s, char *saveToPath = "./") {
  return (IloCplex::Callback(new (env) IncumbentSaverI(env, s, saveToPath)));
}

#endif // UDINE_SAVER
