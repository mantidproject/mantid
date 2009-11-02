#ifndef SAVESPETEST_H_
#define SAVESPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSPE.h"
#include "MantidKernel/UnitFactory.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"
#include "Poco/File.h"
#include <fstream>

class SaveSPETest : public CxxTest::TestSuite
{
public:  
  void testName()
  {
    TS_ASSERT_EQUALS( saver.name(), "SaveSPE" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( saver.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( saver.category(), "DataHandling" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( saver.initialize() )
    TS_ASSERT( saver.isInitialized() )
    
    TS_ASSERT_EQUALS( saver.getProperties().size(), 2 )
  }

  void testExec()
  {
    using namespace Mantid::API;
    // Create a small test workspace
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,10,1.0);
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");
    const std::string input("input");
    AnalysisDataService::Instance().add(input,inputWS);
    
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("InputWorkspace",input) )
    const std::string outputFile("testSPE.spe");
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename",outputFile) )
    
    TS_ASSERT_THROWS_NOTHING( saver.execute() )
    TS_ASSERT( saver.isExecuted() )
    
    TS_ASSERT( Poco::File(outputFile).exists() )
    std::ifstream file(outputFile.c_str());
    
    std::string tmp;
    double tmp2;
    
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "       2      10" )
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### Phi Grid" )
    file >> tmp2;
    TS_ASSERT_EQUALS( tmp2, 0.5 )
    getline(file,tmp);
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### Energy Grid" )
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 1 )
    getline(file,tmp);
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 9 )
    getline(file,tmp);
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### S(Phi,w)" )
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 2 )
    getline(file,tmp);
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 2 )
    getline(file,tmp);
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### Errors" )
    file >> tmp2;    
    TS_ASSERT_DELTA( tmp2, 1.414, 0.001 )
    getline(file,tmp);
    file >> tmp2;
    TS_ASSERT_DELTA( tmp2, 1.414, 0.001 )
    getline(file,tmp);
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### S(Phi,w)" )
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 2 )
    getline(file,tmp);
    file >> tmp2;    
    TS_ASSERT_EQUALS( tmp2, 2 )
    getline(file,tmp);
    getline(file,tmp);
    TS_ASSERT_EQUALS( tmp, "### Errors" )
    file >> tmp2;    
    TS_ASSERT_DELTA( tmp2, 1.414, 0.001 )
    getline(file,tmp);
    file >> tmp2;    
    TS_ASSERT_DELTA( tmp2, 1.414, 0.001 )
    getline(file,tmp);
    TS_ASSERT( file.good() )
    // That should be the end of the file
    getline(file,tmp);
    TS_ASSERT( file.fail() )

    file.close();
    
    AnalysisDataService::Instance().remove(input);
    //Poco::File(outputFile).remove();
  }
  
private:
  Mantid::DataHandling::SaveSPE saver;
};

#endif /*SAVESPETEST_H_*/
