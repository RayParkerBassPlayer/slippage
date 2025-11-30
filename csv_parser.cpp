#include "csv_parser.h"
#include "external/csv-parser/single_include/csv.hpp"
#include <iostream>
#include <stdexcept>

std::vector<Member> CsvParser::parseMembers(const std::string &filename) {
    std::vector<Member> members;
    csv::CSVReader reader(filename);
    
    for (csv::CSVRow &row : reader) {
        std::string memberId = row["member_id"].get<>();
        int boatFeetLength = row["boat_length_ft"].get<int>();
        int boatInchesLength = row["boat_length_in"].get<int>();
        int boatFeetWidth = row["boat_width_ft"].get<int>();
        int boatInchesWidth = row["boat_width_in"].get<int>();
        
        std::optional<std::string> currentSlip;
        std::string currentSlipStr = row["current_slip"].get<>();
        if (!currentSlipStr.empty()) {
            currentSlip = currentSlipStr;
        }
        
        std::string isPermanentStr = row["is_permanent"].get<>();
        bool isPermanent = (isPermanentStr == "1" || isPermanentStr == "true" || isPermanentStr == "TRUE");
        
        members.emplace_back(memberId, boatFeetLength, boatInchesLength,
                           boatFeetWidth, boatInchesWidth, currentSlip, isPermanent);
    }
    
    return members;
}

std::vector<Slip> CsvParser::parseSlips(const std::string &filename) {
    std::vector<Slip> slips;
    csv::CSVReader reader(filename);
    
    for (csv::CSVRow &row : reader) {
        std::string slipId = row["slip_id"].get<>();
        int feetLength = row["max_length_ft"].get<int>();
        int inchesLength = row["max_length_in"].get<int>();
        int feetWidth = row["max_width_ft"].get<int>();
        int inchesWidth = row["max_width_in"].get<int>();
        
        slips.emplace_back(slipId, feetLength, inchesLength, feetWidth, inchesWidth);
    }
    
    return slips;
}

void CsvParser::writeAssignments(const std::vector<Assignment> &assignments) {
    std::cout << "member_id,assigned_slip,status,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,comment\n";
    
    for (const auto &assignment : assignments) {
        const auto &dims = assignment.boatDimensions();
        
        int lengthFeet = dims.lengthInches() / 12;
        int lengthInches = dims.lengthInches() % 12;
        int widthFeet = dims.widthInches() / 12;
        int widthInches = dims.widthInches() % 12;
        
        std::cout << assignment.memberId() << ","
                  << assignment.slipId() << ","
                  << Assignment::statusToString(assignment.status()) << ","
                  << lengthFeet << ","
                  << lengthInches << ","
                  << widthFeet << ","
                  << widthInches << ","
                  << assignment.comment() << "\n";
    }
}
