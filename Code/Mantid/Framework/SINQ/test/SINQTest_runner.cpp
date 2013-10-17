/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#include <fstream>
#define _CXXTEST_LONGLONG long long
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>
#include <cxxtest/TestMain.h>
#include <cxxtest/XUnitPrinter.h>

int main( int argc, char *argv[] ) {
    std::string output_filename = "TEST-SINQTest.xml";  
    // Look for an argument giving the suite name (not starting with -) and change the output filename to use it.  
     if (argc > 1) { 
         if (argv[1][0] != '-') { 
            output_filename = "TEST-SINQTest." + std::string(argv[1]) + ".xml"; 
        } } 
    // Create the output XML file 
    std::ofstream ofstr( output_filename.c_str() );
    CxxTest::XUnitPrinter tmp(ofstr);
    CxxTest::RealWorldDescription::_worldName = "SINQTest";
    return CxxTest::Main<CxxTest::XUnitPrinter>( tmp, argc, argv );
}
#include <cxxtest/Root.cpp>
const char* CxxTest::RealWorldDescription::_worldName = "cxxtest";
