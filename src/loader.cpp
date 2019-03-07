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

#include <cstdlib>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <set>
#include <map>
// #include <boost/regex.hpp>

#include "loader.h"


void loadFile(std::string& s, std::istream& is) {
  s.erase();
  if(is.bad()) return;
  s.reserve(is.rdbuf()->in_avail());
  char c;
  while(is.get(c)) {
    if(s.capacity() == s.size()) s.reserve(s.capacity() * 3);
    s.append(1, c);
  }
}


bool TimetablingInstance::check(const char *filename) {
  std::ifstream file;
  file.open(filename, std::ifstream::in);    
  std::string content;
  loadFile(content, file);
  file.close();
  // TODO: if boost::regex is available, use regex to check the content
  /*
  "Name:\\s+(\\d)\\s*"
  "Courses:\\s+(\\d)\\s*"
  "Rooms:\\s+(\\d)\\s*"
  "Days:\\s+(\\d)\\s*"
  "Periods_per_day:\\s+(\\d)\\s*"
  "Curricula:\\s+(\\d)\\s*"
  "Constraints:\\s+(\\d)\\s*"
  "COURSES:\\s*"
  "((\\w+\\s\\w+\\s\\d+\\s\\d+\\s\\d+)\\s*)+\\s*";
  */
  return true;
}


std::istream& operator>>(std::istream &is, Course &c) {
  return is >> c.name >> c.teacher >> c.lectures >> c.minWorkingDays >> c.students;
}


void TimetablingInstance::load(const char *thisFilename) {
  try {
    std::ifstream file;
    assert(check(thisFilename));
    file.open(thisFilename, std::ifstream::in);    
    filename = thisFilename;

    std::string forget; int i;

    std::string iname;
    int courseCnt, roomCnt, dayCnt, curriculumCnt, constraintCnt;

    // load the header
    file >> forget >> iname; 
    file >> forget >> courseCnt; eventCnt = 0;
    file >> forget >> roomCnt;
    file >> forget >> dayCnt;
    file >> forget >> periodsPerDay;
    file >> forget >> curriculumCnt;
    file >> forget >> constraintCnt;

    periods = dayCnt * periodsPerDay;
    days = dayCnt;
    checks = periodsPerDay;
    origCurricula = curriculumCnt;

    std::cout << "Loader: Instance " << iname << " (" << filename << ")"<< std::endl; 

    // courses
    file >> forget;
    for (i = 0; i < courseCnt; i++) {
      Course c;
      assert(file.good());
      file >> c;
      eventCnt += c.lectures;
      cnames[c.name] = courses.size();
      courses.push_back(c);
    }

    // rooms and their capacities
    file >> forget;
    for (i = 0; i < roomCnt; i++) {
      std::string name; int capacity;
      assert(file.good());
      file >> name >> capacity;
      Room r;
      r.capacity = capacity;
      r.name = name;
      rnames[name] = rooms.size();
      // std::cout << "Room: " << name << rnames[name] << std::endl;
      rooms.push_back(r);
    }

    // groups of conflicting courses
    file >> forget;
    for (i = 0; i < curriculumCnt; i++) {
      Curriculum u; int ccount;
      file >> u.name >> ccount;
      for (int j = 0; j < ccount; j++) {
        std::string s;
        assert(file.good());
        file >> s;
        assert(cnames.find(s) != cnames.end());
        u.courseIds.push_back(cnames.find(s)->second);
      }
      if (ccount >= 2) curricula.push_back(u);
    }
    origCurricula = curricula.size();

    // restrictions on times when teachers are available
    file >> forget;
    for (i = 0; i < constraintCnt; i++) {
      std::string cname; int day; int period;
      assert(file.good());
      file >> cname >> day >> period;
      Restriction r;
      assert(cnames.find(cname) != cnames.end());
      r.courseId = cnames.find(cname)->second;
      r.period = day * periodsPerDay + period;
      restrict.push_back(r);
    }

    // make an overview of who teaches what
    typedef std::map< std::string, std::vector<int>, std::less<std::string> > s2veci;
    s2veci tnames;
    std::vector<Course>::iterator it = courses.begin();
    for (i = 0; it != courses.end(); it++, i++) {
      if (tnames.find(it->teacher) == tnames.end()) {
        std::vector<int> teaches;
        teaches.push_back(i);
        tnames[it->teacher] = teaches;
      } else {
        tnames.find(it->teacher)->second.push_back(i);
      }
    }
    // look for teachers teaching more than a single course 
    s2veci::iterator mapit = tnames.begin();
    for (; mapit != tnames.end(); mapit++) {
      if (mapit->second.size() > 1) {
        // create artificial curricula out of this
        Curriculum c;
        c.courseIds = mapit->second;
        c.name = mapit->first;
        curricula.push_back(c);
      }
    }

    if (file.bad()) {
      std::cout << "Loader: Instance incomplete!" << std::endl; 
      abort();
    } else {
      std::cout << "Loader: " << courseCnt << " courses, ";
      std::cout << eventCnt << " events, and ";
      std::cout << curriculumCnt << " curricula" << std::endl; 
    }

    file.close();

  } catch (std::exception &e) {
    std::cerr << "Loader: There was an error reading the instance!" << std::endl;
    std::cerr << "Exception says: " << e.what() << std::endl;
    abort();
  }
}  // END of TimetablingInstance::load


// NOTE: Does not support the trivial cases of days of less than three periods
void TimetablingInstance::generatePatterns(int toAdd, int rhs, std::vector<int> pat) {

  // Recursion
  if (toAdd > 0) {
    pat.push_back(-1);
    generatePatterns(toAdd - 1, rhs, pat);
    pat.pop_back();
    pat.push_back(1);
    generatePatterns(toAdd - 1, rhs + 1, pat);
    return;
  }

  // Evaluate the pattern, if complete 
  int penalty = 0;
  int last = pat.size() - 1;
  if (pat.size() < 3) return;

  if (pat[0] == 1 && pat[1] == -1) penalty += 1;
  if (pat[last] == 1 && pat[last - 1] == -1) penalty += 1;

  for (int i = 0; i < pat.size() - 2; i++) 
    if (pat[i] == -1 && pat[i + 1] == 1 && pat[i + 2] == -1)
      penalty += 1;

  // Save it if it has attracted any penalty
  if (penalty > 0) {
    Pattern p;
    p.coefs = pat;
    p.penalty = penalty;
    p.rhs = rhs + 1;    
    patterns.push_back(p);

    // Debugging
    std::cout << "Pattern: ";
    std::ostream_iterator<int, char, std::char_traits<char> > out(std::cout, "\t");
    std::copy(pat.begin(), pat.end(), out);
    std::cout << " rhs " << rhs + 1 << " penalty " << penalty << std::endl;
  }
}
