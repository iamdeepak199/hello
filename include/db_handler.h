#pragma once
#include <string>

// Function to parse the text file and insert into MySQL database
bool insertIntoDatabase(const std::string& filename);