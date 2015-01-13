#ifndef SAVETOMOCONFIGTEST_H_
#define SAVETOMOCONFIGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/SaveTomoConfig.h"

#include <Poco/File.h>

class SaveTomoConfigTest : public CxxTest::TestSuite
{
public:


  void test_init()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void test_save_reload()
  {
    // Mantid::DataHandling::SaveTomoConfig tcSave;

    if (!alg.isInitialized())
      alg.initialize();

    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(out = AnalysisDataService::Instance().retrieve(outputSpace));    

    ITableWorkspace_sptr tws = boost::dynamic_pointer_cast<ITableWorkspace>(out);
  }

  void test_pass_inputworkspace_as_pointer()
  {
    Workspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(2,5);

    SaveNexus alg;
    alg.initialize();
    alg.setProperty("InputWorkspace",ws);
    alg.setProperty("Filename","out.nxs");

    std::string outputFile = alg.getPropertyValue("Filename");

    const bool executed = alg.execute();
    TSM_ASSERT( "SaveNexus did not execute successfully", executed )
    if ( executed ) Poco::File(outputFile).remove();
  }
  
private:

  SaveTomoConfig alg;
  std::string outFilename;
  std::string title;
};
#endif /*SAVETOMOCONFIGTEST_H*/
