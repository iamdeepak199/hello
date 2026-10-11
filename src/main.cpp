#include "mismoparser.h"
#include <iostream>
#include "db_handler.h"

int main(int argc, char* argv[]) {
    std::string xmlPath = "data/sample_mismo.xml";
    std::string txtPath = "extracted_scores.txt";

    if (argc > 1) {
        xmlPath = argv[1];
    }
    if (argc > 2) {
        txtPath = argv[2];
    }

    std::cout << "Parsing MISMO XML file: " << xmlPath << "..." << std::endl;

    MismoParser parser;
    LoanScoreRecord record;

    if (parser.parseXmlFile(xmlPath, record)) {
        if (parser.exportToTxtFile(txtPath, record)) {
            std::cout << "Successfully extracted data and saved to: " << txtPath << std::endl;
            
            // --- ADDED: Database Insertion Step ---
            std::cout << "Inserting data into database..." << std::endl;
            if (!insertIntoDatabase(txtPath)) {
                std::cerr << "Failed to insert into database." << std::endl;
            } else {
                std::cout << "Database insertion successful." << std::endl;
            }
            // --------------------------------------

        } else {
            std::cerr << "Error writing to TXT file." << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Failed to parse XML file." << std::endl;
        return 1;
    }

    return 0;
}