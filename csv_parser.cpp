#include "csv_parser.hpp"
#include "external/csv-parser/single_include/csv.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>

std::vector<Member> CsvParser::parseMembers(const std::string &filename){
    std::vector<Member> members;
    csv::CSVReader reader(filename);
    
    for (csv::CSVRow &row : reader){
        std::string memberId = row["member_id"].get<>();
        int boatFeetLength = row["boat_length_ft"].get<int>();
        int boatInchesLength = row["boat_length_in"].get<int>();
        int boatFeetWidth = row["boat_width_ft"].get<int>();
        int boatInchesWidth = row["boat_width_in"].get<int>();
        
        std::optional<std::string> currentSlip;
        std::string currentSlipStr = row["current_slip"].get<>();
        
        if (!currentSlipStr.empty()){
            currentSlip = currentSlipStr;
        }
        
        std::string dockStatusStr = row["dock_status"].get<>();
        Member::DockStatus dockStatus = Member::stringToDockStatus(dockStatusStr);
        
        members.emplace_back(memberId, boatFeetLength, boatInchesLength,
                           boatFeetWidth, boatInchesWidth, currentSlip, dockStatus);
    }
    
    return members;
}

std::vector<Slip> CsvParser::parseSlips(const std::string &filename){
    std::vector<Slip> slips;
    csv::CSVReader reader(filename);
    
    for (csv::CSVRow &row : reader){
        std::string slipId = row["slip_id"].get<>();
        int feetLength = row["max_length_ft"].get<int>();
        int inchesLength = row["max_length_in"].get<int>();
        int feetWidth = row["max_width_ft"].get<int>();
        int inchesWidth = row["max_width_in"].get<int>();
        
        slips.emplace_back(slipId, feetLength, inchesLength, feetWidth, inchesWidth);
    }
    
    return slips;
}

// Escape and quote a CSV field if it contains special characters
static std::string quoteCsvField(const std::string &field){
    if (field.empty()){
        return field;
    }
    
    // Always quote non-empty comment fields for consistency and easier parsing
    // This ensures all comments are treated uniformly regardless of content
    bool needsQuoting = true;
    
    // Quote the field and escape internal quotes by doubling them
    std::ostringstream result;
    result << '"';
    
    for (char c : field){
        if (c == '"'){
            result << "\"\"";
        }
        else{
            result << c;
        }
    }
    
    result << '"';
    return result.str();
}

void CsvParser::writeAssignments(const std::vector<Assignment> &assignments, std::ostream &out){
    out << "member_id,assigned_slip,status,dock_status,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,price,upgraded,comment\n";
    
    for (const auto &assignment : assignments){
        const auto &dims = assignment.boatDimensions();
        
        int lengthFeet = dims.lengthInches() / 12;
        int lengthInches = dims.lengthInches() % 12;
        int widthFeet = dims.widthInches() / 12;
        int widthInches = dims.widthInches() % 12;
        
        out << assignment.memberId() << ","
            << assignment.slipId() << ","
            << Assignment::statusToString(assignment.status()) << ","
            << Member::dockStatusToString(assignment.dockStatus()) << ","
            << lengthFeet << ","
            << lengthInches << ","
            << widthFeet << ","
            << widthInches << ",";
        
        if (assignment.price() > 0.0){
            out << std::fixed << std::setprecision(2) << assignment.price();
        }
        
        out << "," << (assignment.upgraded() ? "true" : "false")
            << "," << quoteCsvField(assignment.comment()) << "\n";
    }
}
