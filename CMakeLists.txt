cmake_minimum_required(VERSION 3.10)
project(MismoParser)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(include)

# Find MySQL/MariaDB library
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
include_directories(${MYSQL_INCLUDE_DIRS})

# Add all source files to the executable
add_executable(mismo_parser.exe 
    src/main.cpp 
    src/mismoparser.cpp 
    src/pugixml.cpp 
    src/db_handler.cpp  # <-- Add the new source file here
)

# Link libraries (PugiXML if external, and MySQL)
target_link_libraries(mismo_parser.exe ${MYSQL_LIBRARIES})