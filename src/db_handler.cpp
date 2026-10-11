#include "db_handler.h"
#include <iostream>
#include <fstream>
#include <map>
#include <cstdlib> 

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool insertIntoDatabase(const std::string& filename) {
    std::map<std::string, std::string> data;
    std::ifstream file(filename);
    std::string line;

    // 1. Parse the text file
    if (!file.is_open()) {
        std::cerr << "DB Error: Could not open file " << filename << std::endl;
        return false;
    }

    while (std::getline(file, line)) {
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string key = trim(line.substr(0, delimiterPos));
            std::string value = trim(line.substr(delimiterPos + 1));
            data[key] = value;
        }
    }
    file.close();

    if (data.empty()) {
        std::cerr << "DB Error: No data parsed from file." << std::endl;
        return false;
    }

    // 2. Construct the SQL INSERT Query
    std::string columns = "";
    std::string values = "";

    for (auto const& pair : data) {
        if (!columns.empty()) {
            columns += ", ";
            values += ", ";
        }
        // ADDED BACKTICKS AROUND pair.first so MySQL accepts "Borr_EQ_VANTAGE_4.0"
        columns += "`" + pair.first + "`"; 
        values += "'" + pair.second + "'"; 
    }

    // CHANGED TABLE NAME from MismoScores to Scores
    std::string query = "INSERT INTO Scores (" + columns + ") VALUES (" + values + ");";
    //std::string query = "INSERT INTO MismoScores (" + columns + ") VALUES (" + values + ");";

    // 3. Write the query to a temporary SQL file
    std::string sqlFilename = "temp_query.sql";
    std::ofstream sqlFile(sqlFilename);
    if (!sqlFile.is_open()) {
        std::cerr << "Error creating temporary SQL file." << std::endl;
        return false;
    }
    sqlFile << query << std::endl;
    sqlFile.close();

    // 4. Execute using the installed MySQL command line tool via system()
    // Using the password '12345' and database 'Score' from your previous code
// Notice that it now says "dbo" right before the < symbol
    std::string command = "\"C:\\Program Files\\MySQL\\MySQL Server 8.0\\bin\\mysql.exe\" -h 127.0.0.1 -u root -p12345 dbo < " + sqlFilename;    int result = std::system(command.c_str());
    // 5. Clean up the temp file
    std::remove(sqlFilename.c_str());

    if (result == 0) {
        std::cout << "Successfully inserted record into database!" << std::endl;
        return true;
    } else {
        std::cerr << "DB Insert Error: MySQL command failed." << std::endl;
        return false;
    }
}