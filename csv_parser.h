#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "member.h"
#include "slip.h"
#include "assignment.h"
#include <vector>
#include <string>

class CsvParser {
public:
    static std::vector<Member> parseMembers(const std::string &filename);
    static std::vector<Slip> parseSlips(const std::string &filename);
    static void writeAssignments(const std::vector<Assignment> &assignments);
};

#endif
