#include <vector>
#include <iostream>
using namespace std;


struct Pattern { std::vector<int> coefs; int penalty; int rhs; };
typedef std::vector<Pattern> PatternDB;
PatternDB patterns; // not nice global variable


// NOTE: Does not support the trivial cases of days of less than three periods
void generatePatterns(int toAdd, int rhs = -1, std::vector<int> pat = std::vector<int>()) {

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

  Pattern p;
  p.coefs = pat;
  p.penalty = penalty;
  p.rhs = rhs + 1;    
  patterns.push_back(p);
}


int main() {
  const int periodsPerDayCount = 3;

  generatePatterns(periodsPerDayCount);

  int cut, scenario;
  for (cut = 0; cut < patterns.size(); cut++) {
    if (patterns[cut].penalty <= 0) continue;

    std::cout << std::endl << "Checker: Testing pattern ";
    std::ostream_iterator<int, char, std::char_traits<char> > out(std::cout, "\t");
    std::copy(patterns[cut].coefs.begin(), patterns[cut].coefs.end(), out);
    std::cout << " rhs " << patterns[cut].rhs << " penalty " << patterns[cut].penalty;

    float maxLhs = 0;

    for (scenario = 0; scenario < patterns.size(); scenario++) {

      std::cout << std::endl << "Checker:  against ...";
      std::ostream_iterator<int, char, std::char_traits<char> > out(std::cout, "\t");
      std::copy(patterns[scenario].coefs.begin(), patterns[scenario].coefs.end(), out);
      // std::cout << " rhs " << patterns[scenario].rhs << " penalty " << patterns[scenario].penalty << std::endl;

      float sum;
      sum = 0;
      int pd;
      for (pd = 0; pd < periodsPerDayCount; pd++)
        if (patterns[scenario].coefs[pd] > 0.1) sum += patterns[cut].coefs[pd];

      float lhs = patterns[cut].penalty * (1 - patterns[cut].rhs + sum);
      if (lhs > maxLhs) maxLhs = lhs;
      if (lhs == patterns[cut].penalty) std::cout << " LHS = " << lhs << ".";
      if (lhs > patterns[cut].penalty) std::cout << " LHS = " << lhs << "!";
    }

    if (maxLhs == patterns[cut].penalty) std::cout << std::endl << "Checker: OK!";
    else std::cout << std::endl << "Checker: Error found. LHS can reach " << maxLhs << " instead of " << patterns[cut].penalty << " it is supposed to. ";

  }
}