#ifndef SAVENISTDATTEST_H_
#define SAVENISTDATTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/SaveNISTDAT.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class SaveNISTDATTest : public CxxTest::TestSuite
{
public:

  void test_writer()
  {
    std::string outputFile = "SaveNISTDAT_Output.dat";

    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "saveNISTDAT_data.nxs"));
    loader.setPropertyValue("OutputWorkspace","SaveNISTDAT_Input");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    SaveNISTDAT writer;
    writer.initialize();
    writer.setPropertyValue("InputWorkspace","SaveNISTDAT_Input");
    writer.setPropertyValue("Filename",outputFile);
    outputFile = writer.getPropertyValue("Filename");
    TS_ASSERT_THROWS_NOTHING(writer.execute());

    TS_ASSERT( Poco::File(outputFile).exists() );

    std::ifstream testFile(outputFile.c_str(), std::ios::in);
    TS_ASSERT ( testFile );

    std::string fileLine;
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "Qx - Qy - I(Qx,Qy)\r" );
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "ASCII data\r" );
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "-0.0105  -0.0735  6.13876e+08\r" );

    // remove file created by this algorithm
    Poco::File(outputFile).remove();
  }
};

#endif /*SAVENISTDATTEST_H_*/
