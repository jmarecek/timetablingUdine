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

#ifndef UDINE_LOADER
#define UDINE_LOADER

#include <cassert>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <utility>

/* This loader is based on the code of Andrea Schaerf, although
there doesn't seem to be a single line copied in verbatim.
*/

struct Course {
  std::string name, teacher; 
  int lectures, minWorkingDays, students;
};
typedef std::vector<Course> Courses;

struct Room {
  std::string name; 
  int capacity;
};
typedef std::vector<Room> Rooms;

typedef std::vector<int> CourseIds;
struct Curriculum {
  std::string name; 
  CourseIds courseIds;
};
typedef std::vector<Curriculum> Curricula;

struct Restriction {
  int courseId; 
  int period;
};
typedef std::vector<Restriction> Restrictions;

// For pattern cuts
struct Pattern { std::vector<int> coefs; int penalty; int rhs; };
typedef std::vector<Pattern> PatternDB;

class TimetablingInstance {
protected:
  std::string filename;
  int periods, periodsPerDay, days, checks, eventCnt;

  Courses courses;
  std::map< std::string, int, std::less<std::string> > cnames; // course names

  Rooms rooms;
  std::map< std::string, int, std::less<std::string> > rnames; // room names

  int origCurricula;  // the number of the original curricula, without auxiliaries
  Curricula curricula;
  Restrictions restrict;
  PatternDB patterns;

protected:
  void generatePatterns(int toAdd, int rhs = -1, std::vector<int> soFar = std::vector<int>());

public:
  bool check(const char *filename);
  void load(const char *filename);

  std::string getFilename() { return filename; } 
  int getPeriodCount() { return periods; }
  int getDayCount() { return days; }
  int getPeriodsPerDayCount() { return periodsPerDay; }
  int getCheckCount() { return checks; }
  int getCourseCount() { return courses.size(); }
  int getEventCount() { return eventCnt; } 
  int getProperCurriculumCount() { return origCurricula; }
  int getCurriculumCount() { return curricula.size(); }
  int getRoomCount() { return rooms.size(); }
  int getRestrictionCount() { return restrict.size(); }
  const Course & getCourse(int i) { assert(i >= 0 && i < courses.size());  return courses.at(i); }
  const Curriculum & getCurriculum(int i) { assert(i >= 0 && i < curricula.size());  return curricula.at(i); }
  const Restriction & getRestriction(int i) { assert(i >= 0 && i < restrict.size());  return restrict.at(i); }
  const Room getRoom(int i) { assert(i >= 0 && i < rooms.size()); return rooms.at(i); }
  int getCourseId(std::string s) { try {return cnames[s];} catch (...) {
    std::cerr << "Error: Course " << s << " not found!"; return -1;} }
  int getRoomId(std::string s) { try {return rnames[s];} catch (...) {
    std::cerr << "Error: Room " << s << " not found!"; return -1;} }
  PatternDB getPatterns() { 
    if (patterns.size() == 0) {
      std::cout << "Solver: Enumerating patterns to penalise ..." << std::endl;
      generatePatterns(getPeriodsPerDayCount());
    }
    return patterns;
  }
};

#endif // UDINE_LOADER
