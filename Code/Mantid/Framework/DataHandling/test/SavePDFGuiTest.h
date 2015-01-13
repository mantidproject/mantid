#ifndef MANTID_DATAHANDLING_SAVEPDFGUITEST_H_
#define MANTID_DATAHANDLING_SAVEPDFGUITEST_H_

#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include <fstream>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataHandling/SavePDFGui.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using Mantid::DataHandling::SavePDFGui;
using Mantid::DataHandling::LoadNexusProcessed;
using namespace Mantid::API;

class SavePDFGuiTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SavePDFGuiTest *createSuite() { return new SavePDFGuiTest(); }
  static void destroySuite( SavePDFGuiTest *suite ) { delete suite; }


  void test_Init()
  {
    SavePDFGui alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  size_t read( std::istream & is, std::vector <char> & buff ) {
      is.read( &buff[0], buff.size() );
      return is.gcount();
  }

  size_t countEOL( const std::vector <char> & buff, size_t sz ) {
      size_t newlines = 0;
      const char * p = &buff[0];
      for ( size_t i = 0; i < sz; i++ ) {
        if ( p[i] == '\n' ) {
          newlines++;
        }
      }
      return newlines;
  }

  size_t countLines(const std::string &filename)
  {
    const size_t BUFFER_SIZE = 1024 * 1024;
    std::vector <char> buffer( BUFFER_SIZE );
    std::ifstream in( filename.c_str() );
    size_t n = 0;
    while( size_t cc = read( in, buffer ) ) {
      n += countEOL( buffer, cc );
    }
    return n;
  }

  bool loadWorkspace(const std::string &filename, const std::string wsName)
  {
    LoadNexusProcessed load;
    load.initialize();
    load.setProperty("Filename", filename);
    load.setProperty("OutputWorkspace", wsName);
    load.execute();
    return load.isExecuted();
  }

  void test_exec()
  {
    // name of workspace to create and save
    const std::string wsName("SavePDFGuiTest_OutputWS");
    // name of the output file
    const std::string outFilename("SavePDFGuiTest_Output.gr");

    // Load a file to save out
    TS_ASSERT(loadWorkspace("nom_gr.nxs", wsName));

    // save the file
    SavePDFGui alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", outFilename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // do the checks
    Poco::File outFile(outFilename);
    TS_ASSERT( outFile.isFile());
    TS_ASSERT_EQUALS( countLines(outFilename), 1003);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);

    // remove the output file
    outFile.remove(false);
  }

  void test_exec_ws_group()
  {
    // Create a group
    const std::string groupName("SavePDFGUIGroup");
    TS_ASSERT(loadWorkspace("nom_gr.nxs", groupName+"_1"));
    TS_ASSERT(loadWorkspace("nom_gr.nxs", groupName+"_2"));

    auto grpAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    grpAlg->initialize();
    grpAlg->setPropertyValue("InputWorkspaces", groupName+"_1,"+groupName+"_2");
    grpAlg->setPropertyValue("OutputWorkspace", groupName);
    grpAlg->execute();

    // name of the output file
    const std::string outFilename("SavePDFGUIGroup.gr");

    // run the algorithm with a group
    SavePDFGui alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", groupName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename","SavePDFGUIGroup.gr") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    // remove the workspace group
    AnalysisDataService::Instance().deepRemoveGroup(groupName);
  }
};


#endif /* MANTID_DATAHANDLING_SAVEPDFGUITEST_H_ */
