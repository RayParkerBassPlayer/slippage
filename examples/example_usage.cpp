#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <slippage/dimensions.hpp>
#include <iostream>

void printAssignments(const std::vector<Assignment> &assignments, const std::string &title){
    std::cout << "\n" << title << "\n";
    std::cout << std::string(title.length(), '=') << "\n\n";

    for (const auto &assignment : assignments){
        std::cout << "Member: " << assignment.memberId();
        
        if (assignment.upgraded()){
            std::cout << " (UPGRADED)";
        }
        
        std::cout << "\n  Status: ";

        switch (assignment.status()){
            case Assignment::Status::PERMANENT:
                std::cout << "PERMANENT";
                break;
            case Assignment::Status::NEW:
                std::cout << "NEW";
                break;
            case Assignment::Status::UNASSIGNED:
                std::cout << "UNASSIGNED";
                break;
        }

        std::cout << "\n";

        if (assignment.status() != Assignment::Status::UNASSIGNED){
            std::cout << "  Slip: " << assignment.slipId() << "\n";
            
            int boatLengthFt = assignment.boatDimensions().lengthInches() / 12;
            int boatLengthIn = assignment.boatDimensions().lengthInches() % 12;
            int boatWidthFt = assignment.boatDimensions().widthInches() / 12;
            int boatWidthIn = assignment.boatDimensions().widthInches() % 12;
            
            std::cout << "  Boat: " << boatLengthFt << "' " << boatLengthIn << "\" x "
                      << boatWidthFt << "' " << boatWidthIn << "\"\n";
            
            int slipLengthFt = assignment.slipDimensions().lengthInches() / 12;
            int slipLengthIn = assignment.slipDimensions().lengthInches() % 12;
            int slipWidthFt = assignment.slipDimensions().widthInches() / 12;
            int slipWidthIn = assignment.slipDimensions().widthInches() % 12;
            
            std::cout << "  Slip: " << slipLengthFt << "' " << slipLengthIn << "\" x "
                      << slipWidthFt << "' " << slipWidthIn << "\"\n";
            
            if (assignment.price() > 0.0){
                std::cout << "  Price: $" << assignment.price() << "\n";
            }
        }

        if (!assignment.comment().empty()){
            std::cout << "  Comment: " << assignment.comment() << "\n";
        }

        std::cout << "\n";
    }
}

void example1_basic(){
    std::cout << "\n### Example 1: Basic Assignment ###\n";

    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 0, 10, 0);
    slips.emplace_back("S3", 40, 0, 15, 0);

    std::vector<Member> members;
    members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 24, 0, 9, 0, std::nullopt, Member::DockStatus::WAITING_LIST);
    members.emplace_back("M3", 38, 0, 14, 0, std::nullopt, Member::DockStatus::UNASSIGNED);

    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();

    printAssignments(assignments, "Basic Assignment Results");
}

void example2_ignoreLength(){
    std::cout << "\n### Example 2: Ignore Length Mode (--ignore-length) ###\n";

    std::vector<Slip> slips;
    slips.emplace_back("S1", 25, 0, 12, 0);
    slips.emplace_back("S2", 20, 0, 10, 0);

    std::vector<Member> members;
    // Boat is longer than available slips
    members.emplace_back("M1", 30, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 28, 0, 9, 0, std::nullopt, Member::DockStatus::TEMPORARY);

    AssignmentEngine engine(std::move(members), std::move(slips));
    
    // CLI equivalent: --ignore-length
    engine.setIgnoreLength(true);
    
    auto assignments = engine.assign();

    printAssignments(assignments, "Ignore Length Mode Results");
}

void example3_priceCalculation(){
    std::cout << "\n### Example 3: Price Calculation (--price-per-sqft 2.75) ###\n";

    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 0, 10, 0);

    std::vector<Member> members;
    members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 22, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);

    AssignmentEngine engine(std::move(members), std::move(slips));
    
    // CLI equivalent: --price-per-sqft 2.75
    engine.setPricePerSqFt(2.75);
    
    auto assignments = engine.assign();

    printAssignments(assignments, "Price Calculation Results");
}

void example4_verbose(){
    std::cout << "\n### Example 4: Verbose Mode (--verbose) ###\n";

    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 0, 10, 0);

    std::vector<Member> members;
    members.emplace_back("M1", 28, 0, 11, 0, "S2", Member::DockStatus::PERMANENT);
    members.emplace_back("M2", 24, 0, 9, 0, std::nullopt, Member::DockStatus::WAITING_LIST);

    AssignmentEngine engine(std::move(members), std::move(slips));
    
    // CLI equivalent: --verbose
    engine.setVerbose(true);
    
    auto assignments = engine.assign();

    printAssignments(assignments, "Verbose Mode Results");
}

void example5_combined(){
    std::cout << "\n### Example 5: Combined Options ###\n";
    std::cout << "(--ignore-length --price-per-sqft 3.50 --verbose)\n";

    std::vector<Slip> slips;
    slips.emplace_back("S1", 25, 0, 12, 0);
    slips.emplace_back("S2", 22, 0, 10, 6);

    std::vector<Member> members;
    members.emplace_back("M1", 30, 0, 11, 6, std::nullopt, Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 28, 0, 9, 0, std::nullopt, Member::DockStatus::WAITING_LIST);

    AssignmentEngine engine(std::move(members), std::move(slips));
    
    engine.setIgnoreLength(true);
    engine.setPricePerSqFt(3.50);
    engine.setVerbose(true);
    
    auto assignments = engine.assign();

    printAssignments(assignments, "Combined Options Results");
}

int main(){
    std::cout << "Slippage Library Example - CLI Options Demonstration\n";
    std::cout << "====================================================\n";
    
    example1_basic();
    example2_ignoreLength();
    example3_priceCalculation();
    example4_verbose();
    example5_combined();
    
    std::cout << "\nAll examples completed!\n";
    
    return 0;
}
