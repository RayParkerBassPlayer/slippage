#include "csv_parser.hpp"
#include "assignment_engine.hpp"
#include "version.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

void printVersion() {
  std::cout << "Slippage v" << SLIPPAGE_VERSION << "\n";
  std::cout << "Boat slip assignment system for marina clubs\n";
  std::cout << "\n";
}

void printHelp(const char* programName) {
  printVersion();
  std::cout << "USAGE:\n";
  std::cout << "  " << programName << " --slips <slips.csv> --members <members.csv> [OPTIONS]\n";
  std::cout << "  " << programName << " --version\n";
  std::cout << "  " << programName << " --help\n";
  std::cout << "\n";
  std::cout << "DESCRIPTION:\n";
  std::cout << "  Assigns boat slips to marina club members based on:\n";
  std::cout << "    - Boat and slip dimensions (boats must fit)\n";
  std::cout << "    - Member priority (lower IDs have higher priority)\n";
  std::cout << "    - Current slip occupancy and preferences\n";
  std::cout << "    - Permanent vs. temporary assignments\n";
  std::cout << "\n";
  std::cout << "REQUIRED ARGUMENTS:\n";
  std::cout << "  --slips <file>     CSV file containing slip information\n";
  std::cout << "  --members <file>   CSV file containing member information\n";
  std::cout << "\n";
  std::cout << "OPTIONS:\n";
  std::cout << "  --output <file>    Write assignments to file instead of stdout\n";
  std::cout << "  --verbose          Print detailed assignment progress (phases and passes)\n";
  std::cout << "  --ignore-length    Only check width when determining fit (show length\n";
  std::cout << "                     differences in comments)\n";
  std::cout << "  --price-per-sqft <amount>\n";
  std::cout << "                     Calculate price per square foot (uses larger of boat\n";
  std::cout << "                     or slip area); adds 'price' column to output\n";
  std::cout << "  --help, -h         Show this help message and exit\n";
  std::cout << "  --version, -v      Show version information and exit\n";
  std::cout << "\n";
  std::cout << "INPUT FILE FORMATS:\n";
  std::cout << "\n";
  std::cout << "  slips.csv format:\n";
  std::cout << "    slip_id,max_length_ft,max_length_in,max_width_ft,max_width_in\n";
  std::cout << "    S1,20,0,10,0\n";
  std::cout << "    S2,25,6,12,0\n";
  std::cout << "\n";
  std::cout << "  members.csv format:\n";
  std::cout << "    member_id,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,current_slip,dock_status\n";
  std::cout << "    M001,18,6,8,0,S1,temporary\n";
  std::cout << "    M002,22,0,10,0,S2,permanent\n";
  std::cout << "\n";
  std::cout << "OUTPUT:\n";
  std::cout << "  Results are written to stdout (or file if --output specified) in CSV format:\n";
  std::cout << "    member_id,assigned_slip,status,boat_length_ft,boat_length_in,\n";
  std::cout << "    boat_width_ft,boat_width_in,price,comment\n";
  std::cout << "\n";
  std::cout << "  When writing to stdout without --verbose, output is wrapped with markers:\n";
  std::cout << "    >>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS START\n";
  std::cout << "    [CSV content]\n";
  std::cout << "    >>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS END\n";
  std::cout << "\n";
  std::cout << "  Note: Members who keep their current slip are automatically upgraded\n";
  std::cout << "        to PERMANENT status; see 'upgraded' column in output.\n";
  std::cout << "\n";
  std::cout << "  Status values:\n";
  std::cout << "    PERMANENT   - Member has permanent assignment\n";
  std::cout << "    SAME        - Member kept their current slip\n";
  std::cout << "    NEW         - Member assigned to different slip\n";
  std::cout << "    UNASSIGNED  - Member did not receive assignment\n";
  std::cout << "\n";
  std::cout << "EXAMPLES:\n";
  std::cout << "  # Basic usage (output to stdout)\n";
  std::cout << "  " << programName << " --slips slips.csv --members members.csv\n";
  std::cout << "\n";
  std::cout << "  # Save output to file\n";
  std::cout << "  " << programName << " --slips slips.csv --members members.csv --output assignments.csv\n";
  std::cout << "\n";
  std::cout << "  # Verbose mode with detailed progress\n";
  std::cout << "  " << programName << " --slips slips.csv --members members.csv --verbose\n";
  std::cout << "\n";
  std::cout << "  # Verbose with file output (progress to stdout, CSV to file)\n";
  std::cout << "  " << programName << " --slips slips.csv --members members.csv --output out.csv --verbose\n";
  std::cout << "\n";
  std::cout << "  # Show version\n";
  std::cout << "  " << programName << " --version\n";
  std::cout << "\n";
  std::cout << "DOCUMENTATION:\n";
  std::cout << "  For detailed assignment rules, see:\n";
  std::cout << "    /usr/share/doc/slippage/ASSIGNMENT_RULES.md\n";
  std::cout << "  Or online at: https://github.com/RayParkerBassPlayer/slippage\n";
  std::cout << "\n";
}

void printUsage(const char* programName) {
  std::cerr << "Error: Missing required arguments\n\n";
  std::cerr << "Usage: " << programName << " --slips <slips.csv> --members <members.csv>\n";
  std::cerr << "Try '" << programName << " --help' for more information.\n";
}

int main(int argc, char* argv[]) {
  // Handle no arguments
  if (argc == 1) {
    printUsage(argv[0]);
    return 1;
  }

  std::string slipsFile;
  std::string membersFile;
  std::string outputFile;
  bool verbose = false;
  bool ignoreLength = false;
  double pricePerSqFt = 0.0;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--slips") == 0 && i + 1 < argc) {
      slipsFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "--members") == 0 && i + 1 < argc) {
      membersFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
      outputFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    }
    else if (std::strcmp(argv[i], "--ignore-length") == 0) {
      ignoreLength = true;
    }
    else if (std::strcmp(argv[i], "--price-per-sqft") == 0 && i + 1 < argc) {
      pricePerSqFt = std::stod(argv[++i]);
    }
    else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      printHelp(argv[0]);
      return 0;
    }
    else if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
      printVersion();
      return 0;
    }
    else if (std::strcmp(argv[i], "--slips") == 0 || std::strcmp(argv[i], "--members") == 0 || std::strcmp(argv[i], "--output") == 0 || std::strcmp(argv[i], "--price-per-sqft") == 0) {
      std::cerr << "Error: " << argv[i] << " requires an argument\n\n";
      std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
      return 1;
    }
    else {
      std::cerr << "Error: Unknown argument '" << argv[i] << "'\n\n";
      std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
      return 1;
    }
  }

  if (slipsFile.empty() || membersFile.empty()) {
    printUsage(argv[0]);
    return 1;
  }

  try {
    auto slips = CsvParser::parseSlips(slipsFile);
    auto members = CsvParser::parseMembers(membersFile);

    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setVerbose(verbose);
    engine.setIgnoreLength(ignoreLength);
    engine.setPricePerSqFt(pricePerSqFt);
    auto assignments = engine.assign();

    if (outputFile.empty()) {
      // Show markers only when NOT in verbose mode
      if (!verbose) {
        std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS START\n";
      }
      std::cout << assignments;

      if (!verbose) {
        std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS END\n";
      }
    }
    else {
      std::ofstream outFile(outputFile);

      if (!outFile) {
        std::cerr << "Error: Cannot open output file '" << outputFile << "'\n";
        return 1;
      }

      outFile << assignments;
      outFile.close();

      if (verbose) {
        std::cout << "\nAssignments written to: " << outputFile << "\n";
      }
    }

    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
