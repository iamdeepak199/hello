#include "mismoparser.h"
#include "pugixml.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "mismoparser.h"
#include "pugixml.hpp"

std::string MismoParser::determineScoreCategory(const std::string& modelName) {
    std::string lowerModel = modelName;
    std::transform(lowerModel.begin(), lowerModel.end(), lowerModel.begin(), ::tolower);

    if (lowerModel.find("vantage") != std::string::npos || lowerModel.find("vantage4") != std::string::npos) {
        return "VANTAGE_4.0";
    } else if (lowerModel.find("10t") != std::string::npos) {
        return "FICO_10T";
    } else {
        // Default Classic FICO models (Beacon 5.0, Fair Isaac, TU Classic)
        return "FICO_Classic";
    }
}

bool MismoParser::parseXmlFile(const std::string& filepath, LoanScoreRecord& record) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filepath.c_str());

    if (!result) {
        std::cerr << "XML Parsing Error: " << result.description() << std::endl;
        return false;
    }

    // 1. Extract File & Loan Identifiers
    pugi::xml_node root = doc.child("MESSAGE");
    
    // Vendor Order Identifier / Current File Number
    pugi::xml_node vendorNode = root.select_node("//SERVICE_PRODUCT_FULFILLMENT_DETAIL/VendorOrderIdentifier").node();
    if (vendorNode) {
        record.current_file_number = vendorNode.child_value();
    }

    // Loan ID / Loan Number
    pugi::xml_node loanIdNode = root.select_node("//LOAN_IDENTIFIER/LoanIdentifier").node();
    if (loanIdNode) {
        record.loan_id = loanIdNode.child_value();
        record.loan_number = record.loan_id;
    }

    // Previous File Number (if present in extension)
    pugi::xml_node prevFileNode = root.select_node("//EXTENSION//PreviousVendorOrderIdentifier").node();
    if (prevFileNode) {
        record.previous_file_number = prevFileNode.child_value();
    }

    // Hard / Soft Inquiry Indicator
    pugi::xml_node actionTypeNode = root.select_node("//CREDIT_REQUEST_DATA_DETAIL/CreditReportRequestActionType").node();
    if (actionTypeNode) {
        std::string action = actionTypeNode.child_value();
        record.hard_soft_indicator = (action.find("Soft") != std::string::npos) ? "Soft" : "Hard";
    } else {
        record.hard_soft_indicator = "Hard";
    }

    // 2. Map Parties & Count Borrowers
    pugi::xpath_node_set parties = root.select_nodes("//PARTY");
    std::map<std::string, bool> partyIsPrimaryMap; // Role Label -> IsPrimary

    int borrowerCount = 0;
    for (auto partyNode : parties) {
        pugi::xml_node pNode = partyNode.node();
        pugi::xml_node roleNode = pNode.child("ROLES").child("ROLE");
        
        if (roleNode) {
            std::string label = roleNode.attribute("d7p1:label").value();
            if (label.empty()) {
                label = roleNode.attribute("label").value();
            }

            pugi::xml_node borrowerDetail = roleNode.child("BORROWER").child("BORROWER_DETAIL");
            if (borrowerDetail) {
                borrowerCount++;
                std::string bClass = borrowerDetail.child_value("BorrowerClassificationType");
                bool isPrimary = (bClass != "Secondary" && borrowerCount == 1);
                
                if (!label.empty()) {
                    partyIsPrimaryMap[label] = isPrimary;
                }
            }
        }
    }
    record.number_of_borrowers = borrowerCount;

    // 3. Extract Credit Scores and XLink Relationships
    // Map Score Label -> Role Label via RELATIONSHIP
    std::map<std::string, std::string> scoreToRoleMap;
    pugi::xpath_node_set rels = root.select_nodes("//RELATIONSHIP");
    for (auto relNode : rels) {
        pugi::xml_node r = relNode.node();
        std::string arcrole = r.attribute("d7p1:arcrole").value();
        if (arcrole.find("CREDIT_SCORE_IsAssociatedWith_ROLE") != std::string::npos || 
            arcrole.find("CREDIT_SCORE") != std::string::npos) {
            std::string from = r.attribute("d7p1:from").value();
            std::string to = r.attribute("d7p1:to").value();
            scoreToRoleMap[from] = to;
        }
    }

    // Parse CREDIT_SCORE Nodes
    pugi::xpath_node_set scores = root.select_nodes("//CREDIT_SCORE");
    for (auto scoreItem : scores) {
        pugi::xml_node sNode = scoreItem.node();
        std::string scoreLabel = sNode.attribute("d7p1:label").value();
        if (scoreLabel.empty()) scoreLabel = sNode.attribute("label").value();

        pugi::xml_node detailNode = sNode.child("CREDIT_SCORE_DETAIL");
        if (!detailNode) continue;

        std::string modelName = detailNode.child_value("CreditScoreModelNameType");
        std::string scoreVal = detailNode.child_value("CreditScoreValue");

        // Determine Bureau from Model Name or parent node
        std::string bureau = "EQ";
        std::string lowerModel = modelName;
        std::transform(lowerModel.begin(), lowerModel.end(), lowerModel.begin(), ::tolower);

        if (lowerModel.find("experian") != std::string::npos || lowerModel.find("fair isaac") != std::string::npos) {
            bureau = "EX";
        } else if (lowerModel.find("transunion") != std::string::npos || lowerModel.find("tu") != std::string::npos) {
            bureau = "TU";
        } else if (lowerModel.find("equifax") != std::string::npos || lowerModel.find("beacon") != std::string::npos) {
            bureau = "EQ";
        }

        std::string scoreCategory = determineScoreCategory(modelName);

        // Determine if Primary or CoBorrower
        bool isPrimary = true;
        if (scoreToRoleMap.count(scoreLabel)) {
            std::string roleLabel = scoreToRoleMap[scoreLabel];
            if (partyIsPrimaryMap.count(roleLabel)) {
                isPrimary = partyIsPrimaryMap[roleLabel];
            }
        }

        // Assign to Record
        if (isPrimary) {
            if (scoreCategory == "FICO_Classic") {
                if (bureau == "EQ") record.borr_eq_fico_classic = scoreVal;
                else if (bureau == "EX") record.borr_ex_fico_classic = scoreVal;
                else if (bureau == "TU") record.borr_tu_fico_classic = scoreVal;
            } else if (scoreCategory == "FICO_10T") {
                if (bureau == "EQ") record.borr_eq_fico_10t = scoreVal;
                else if (bureau == "EX") record.borr_ex_fico_10t = scoreVal;
                else if (bureau == "TU") record.borr_tu_fico_10t = scoreVal;
            } else if (scoreCategory == "VANTAGE_4.0") {
                if (bureau == "EQ") record.borr_eq_vantage_4_0 = scoreVal;
                else if (bureau == "EX") record.borr_ex_vantage_4_0 = scoreVal;
                else if (bureau == "TU") record.borr_tu_vantage_4_0 = scoreVal;
            }
        } else { // Co-Borrower
            if (scoreCategory == "FICO_Classic") {
                if (bureau == "EQ") record.coborr_eq_fico_classic = scoreVal;
                else if (bureau == "EX") record.coborr_ex_fico_classic = scoreVal;
                else if (bureau == "TU") record.coborr_tu_fico_classic = scoreVal;
            } else if (scoreCategory == "FICO_10T") {
                if (bureau == "EQ") record.coborr_eq_fico_10t = scoreVal;
                else if (bureau == "EX") record.coborr_ex_fico_10t = scoreVal;
                else if (bureau == "TU") record.coborr_tu_fico_10t = scoreVal;
            } else if (scoreCategory == "VANTAGE_4.0") {
                if (bureau == "EQ") record.coborr_eq_vantage_4_0 = scoreVal;
                else if (bureau == "EX") record.coborr_ex_vantage_4_0 = scoreVal;
                else if (bureau == "TU") record.coborr_tu_vantage_4_0 = scoreVal;
            }
        }
    }

    return true;
}

bool MismoParser::exportToTxtFile(const std::string& outputPath, const LoanScoreRecord& record) {
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "Failed to create output TXT file: " << outputPath << std::endl;
        return false;
    }

    outFile << "=== MISMO / SCORES DATABASE EXTRACT ===" << "\n\n";
    outFile << "LoanId: " << record.loan_id << "\n";
    outFile << "ApplicationNumber: " << record.application_number << "\n";
    outFile << "CurrentFileNumber: " << record.current_file_number << "\n";
    outFile << "PreviousFileNumber: " << record.previous_file_number << "\n";
    outFile << "Hard_Soft_Indicator: " << record.hard_soft_indicator << "\n";
    outFile << "ApplicationID: " << record.application_id << "\n";
    outFile << "NumberOfBorrowers: " << record.number_of_borrowers << "\n";
    outFile << "LoanNumber: " << record.loan_number << "\n";
    outFile << "GroupCode: " << record.group_code << "\n";
    outFile << "ICE_InstanceID: " << record.ice_instance_id << "\n\n";

    outFile << "--- PRIMARY BORROWER SCORES ---" << "\n";
    outFile << "Borr_EQ_FICO_Classic: " << record.borr_eq_fico_classic << "\n";
    outFile << "Borr_EX_FICO_Classic: " << record.borr_ex_fico_classic << "\n";
    outFile << "Borr_TU_FICO_Classic: " << record.borr_tu_fico_classic << "\n";
    outFile << "Borr_EQ_FICO_10T: " << record.borr_eq_fico_10t << "\n";
    outFile << "Borr_EX_FICO_10T: " << record.borr_ex_fico_10t << "\n";
    outFile << "Borr_TU_FICO_10T: " << record.borr_tu_fico_10t << "\n";
    outFile << "Borr_EQ_VANTAGE_4.0: " << record.borr_eq_vantage_4_0 << "\n";
    outFile << "Borr_EX_VANTAGE_4.0: " << record.borr_ex_vantage_4_0 << "\n";
    outFile << "Borr_TU_VANTAGE_4.0: " << record.borr_tu_vantage_4_0 << "\n\n";

    outFile << "--- CO-BORROWER SCORES ---" << "\n";
    outFile << "CoBorr_EQ_FICO_Classic: " << record.coborr_eq_fico_classic << "\n";
    outFile << "CoBorr_EX_FICO_Classic: " << record.coborr_ex_fico_classic << "\n";
    outFile << "CoBorr_TU_FICO_Classic: " << record.coborr_tu_fico_classic << "\n";
    outFile << "CoBorr_EQ_FICO_10T: " << record.coborr_eq_fico_10t << "\n";
    outFile << "CoBorr_EX_FICO_10T: " << record.coborr_ex_fico_10t << "\n";
    outFile << "CoBorr_TU_FICO_10T: " << record.coborr_tu_fico_10t << "\n";
    outFile << "CoBorr_EQ_VANTAGE_4.0: " << record.coborr_eq_vantage_4_0 << "\n";
    outFile << "CoBorr_EX_VANTAGE_4.0: " << record.coborr_ex_vantage_4_0 << "\n";

    outFile.close();
    return true;
}