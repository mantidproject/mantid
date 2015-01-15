cmake_minimum_required ( VERSION 2.8.5 )

# Project name
project ( Mantid )

# Enable testing so that a top-level ctest file is generated in build root
# @todo: This should be protected by enable_testing flags etc
enable_testing() 

# Everything else...
add_subdirectory ( Code/Mantid )

