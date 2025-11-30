#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "member.h"
#include "slip.h"
#include "assignment.h"
#include <vector>
#include <string>
#include <ostream>

class CsvParser {
    static void writeAssignments(const std::vector<Assignment> &assignments, std::ostream &out);

public:
    static std::vector<Member> parseMembers(const std::string &filename);
    static std::vector<Slip> parseSlips(const std::string &filename);
    
    // Stream output operator for writing assignments to any output stream
    friend std::ostream& operator<<(std::ostream &out, const std::vector<Assignment> &assignments);
};

// Inline definition of operator<< 
inline std::ostream& operator<<(std::ostream &out, const std::vector<Assignment> &assignments) {
    CsvParser::writeAssignments(assignments, out);
    return out;
}

#endif
