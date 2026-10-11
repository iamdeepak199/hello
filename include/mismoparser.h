#ifndef MISMO_PARSER_H
#define MISMO_PARSER_H

#include <string>
#include <map>
#include <vector>

struct LoanScoreRecord {
    // Basic identifiers
    std::string loan_id;
    std::string application_number;
    std::string current_file_number;
    std::string previous_file_number;
    std::string hard_soft_indicator;
    std::string application_id;
    int number_of_borrowers = 0;
    std::string loan_number;
    std::string group_code;
    std::string ice_instance_id;

    // Primary Borrower Scores
    std::string borr_eq_fico_classic;
    std::string borr_ex_fico_classic;
    std::string borr_tu_fico_classic;
    std::string borr_eq_fico_10t;
    std::string borr_ex_fico_10t;
    std::string borr_tu_fico_10t;
    std::string borr_eq_vantage_4_0;
    std::string borr_ex_vantage_4_0;
    std::string borr_tu_vantage_4_0;

    // Co-Borrower Scores
    std::string coborr_eq_fico_classic;
    std::string coborr_ex_fico_classic;
    std::string coborr_tu_fico_classic;
    std::string coborr_eq_fico_10t;
    std::string coborr_ex_fico_10t;
    std::string coborr_tu_fico_10t;
    std::string coborr_eq_vantage_4_0;
    std::string coborr_ex_vantage_4_0;
    std::string coborr_tu_vantage_4_0;
};

class MismoParser {
public:
    bool parseXmlFile(const std::string& filepath, LoanScoreRecord& record);
    bool exportToTxtFile(const std::string& outputPath, const LoanScoreRecord& record);

private:
    std::string determineScoreCategory(const std::string& modelName);
};

#endif // MISMO_PARSER_H