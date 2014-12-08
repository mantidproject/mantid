#ifndef SAVENXTOMOTEST_H_
#define SAVENXTOMOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveNXTomo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using Mantid::DataObjects::Workspace2D_sptr;


class SaveNXTomoTest : public CxxTest::TestSuite
{
public:
  static SaveNXTomoTest *createSuite() { return new SaveNXTomoTest(); }
  static void destroySuite(SaveNXTomoTest *suite) { delete suite; }

  SaveNXTomoTest()
  {
    inputWS = "saveNXTomo_test";
    outputFile = "SaveNXTomoTestFile.nxs"; 
    axisSize = 50;
    saver = FrameworkManager::Instance().createAlgorithm("SaveNXTomo");  
  }    

  void testName()
  {
    TS_ASSERT_EQUALS( saver->name(), "SaveNXTomo" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( saver->version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( saver->initialize() );
    TS_ASSERT( saver->isInitialized() );
  }

void testWriteSingleCreating(bool deleteWhenComplete = true)
  {    
    // Test creating a new file from a single WS
    // Create a small test workspace
    Workspace_sptr input = makeWorkspaceSingle(inputWS);

    TS_ASSERT_THROWS_NOTHING( saver->setProperty<Workspace_sptr>("InputWorkspaces", input));
    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("Filename", outputFile) );
    outputFile = saver->getPropertyValue("Filename");//get absolute path

    // Set to overwrite to ensure creation not append
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("OverwriteFile", true)); 
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("IncludeError", false));

    TS_ASSERT_THROWS_NOTHING( saver->execute() );
    TS_ASSERT( saver->isExecuted() );
    
    // Check file exists
    Poco::File file(outputFile);
    TS_ASSERT( file.exists() );

    // Check that the structure of the nxTomo file is correct
    checkNXTomoStructure();

    // Check count of entries for data / run_title / rotation_angle / image_key
    checkNXTomoDimensions(1);

    // Check rotation values
    checkNXTomoRotations(1);

    // Check main data values
    checkNXTomoData(1);

    if(deleteWhenComplete)
    {
      if( file.exists() )
        file.remove();
    }
  }

  void testWriteGroupCreating()
  {
    // Test creating a new file from a WS Group
    // Create small test workspaces
    std::vector<Workspace2D_sptr> wspaces(3);
    WorkspaceGroup_sptr input = makeWorkspacesInGroup(inputWS, wspaces);     
    AnalysisDataService::Instance().add(inputWS + "0", input);   

    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("InputWorkspaces", input->name()) );
    TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("Filename", outputFile) );
    outputFile = saver->getPropertyValue("Filename");//get absolute path

    // Set to overwrite to ensure creation not append
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("OverwriteFile", true));
    TS_ASSERT_THROWS_NOTHING( saver->setProperty("IncludeError", false));

    TS_ASSERT_THROWS_NOTHING( saver->execute() );
    // TODO:: uncomment - currently fails due to 10519 
    // TS_ASSERT( saver->isExecuted() );

    // Check file exists
    Poco::File file(outputFile);
    TS_ASSERT( file.exists() );
    
    // Check that the structure of the nxTomo file is correct
    checkNXTomoStructure();

    // Check count of entries for data / run_title / rotation_angle / image_key
    checkNXTomoDimensions(3);

    // Check rotation values
    checkNXTomoRotations(3);

    // Check main data values
    checkNXTomoData(3);  

    // Tidy up
    AnalysisDataService::Instance().remove(input->name());
    if( file.exists() )
      file.remove();
  }  

  void testWriteGroupAppending()
  {
    // Run the single workspace test again, without deleting the file at the end (to test append)
    testWriteSingleCreating(false);
   
    // Test appending a ws group to an existing file
    if( Poco::File(outputFile).exists() )
    {
      int numberOfPriorWS = 1; // Count of current workspaces in the file 
      // Create small test workspaces
      std::vector<Workspace2D_sptr> wspaces(3);
      WorkspaceGroup_sptr input = makeWorkspacesInGroup(inputWS, wspaces, numberOfPriorWS);     
      AnalysisDataService::Instance().add(inputWS + boost::lexical_cast<std::string>(numberOfPriorWS), input);   

      TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("InputWorkspaces", input->name()) );
      TS_ASSERT_THROWS_NOTHING( saver->setPropertyValue("Filename", outputFile) );
      outputFile = saver->getPropertyValue("Filename");//get absolute path

      // Ensure append not create
      TS_ASSERT_THROWS_NOTHING( saver->setProperty("OverwriteFile", false));
      TS_ASSERT_THROWS_NOTHING( saver->setProperty("IncludeError", false));

      TS_ASSERT_THROWS_NOTHING( saver->execute() );
      // TODO:: uncomment - currently fails due to 10519 
      // TS_ASSERT( saver->isExecuted() );

      // Check file exists
      Poco::File file(outputFile);
      TS_ASSERT( file.exists() );
      
      // Check that the structure of the nxTomo file is correct
      checkNXTomoStructure();

      // Check count of entries for data / run_title / rotation_angle / image_key
      checkNXTomoDimensions(static_cast<int>(wspaces.size()) + numberOfPriorWS);

      // Check rotation values
      checkNXTomoRotations(static_cast<int>(wspaces.size()) + numberOfPriorWS);

      // Check main data values
      checkNXTomoData(static_cast<int>(wspaces.size()) + numberOfPriorWS);
      
      // Tidy up
      AnalysisDataService::Instance().remove(input->name());
      file.remove();
    }
  }


private:

  Workspace_sptr makeWorkspaceSingle(const std::string &input)
  {
    // Create a single workspace    
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(axisSize*axisSize,1,1.0);     
    ws->setTitle(input);
    
    // Add axis sizes
    ws->mutableRun().addLogData(new PropertyWithValue<int>("Axis1", axisSize));  
    ws->mutableRun().addLogData(new PropertyWithValue<int>("Axis2", axisSize));  
    
    // Add log values
    ws->mutableRun().addLogData(new PropertyWithValue<double>("Rotation", 1 * 5));    
   
    return ws;
  }

  WorkspaceGroup_sptr makeWorkspacesInGroup(const std::string &input, std::vector<Workspace2D_sptr> &wspaces, int wsIndOffset = 0)
  {
    // Create a ws group with 3 workspaces.
    WorkspaceGroup_sptr wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup());   
    std::string groupName = input + boost::lexical_cast<std::string>(wsIndOffset);
    wsGroup->setTitle(groupName);
   
    for(uint32_t i=0;i<static_cast<uint32_t>(wspaces.size());++i)
    {
      wspaces[i] = WorkspaceCreationHelper::Create2DWorkspaceBinned(axisSize*axisSize,1,1.0);  
      wspaces[i]->setTitle(groupName + boost::lexical_cast<std::string>(wsIndOffset+(i+1)));
      wspaces[i]->mutableRun().addLogData(new PropertyWithValue<int>("Axis1", axisSize));  
      wspaces[i]->mutableRun().addLogData(new PropertyWithValue<int>("Axis2", axisSize));  
      wspaces[i]->mutableRun().addLogData(new PropertyWithValue<double>("Rotation", ((i+1) + wsIndOffset) * 5));
      wsGroup->addWorkspace(wspaces[i]);
    }    

    return wsGroup;
  }

  void checkNXTomoStructure()
  {
    // Checks the structure of the file - not interested in the data content
    NXhandle fileHandle;
    NXstatus status = NXopen(outputFile.c_str(), NXACC_RDWR, &fileHandle);
    
    TS_ASSERT( status != NX_ERROR);

    if(status!= NX_ERROR)
    {
      ::NeXus::File nxFile(fileHandle);
      
      // Check for entry1/tomo_entry/control { and data dataset within }
      TS_ASSERT_THROWS_NOTHING( nxFile.openPath("/entry1/tomo_entry/control") );
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("data") );
      try { nxFile.closeData(); } catch(...) {}

      // Check for entry1/tomo_entry/data { and data / rotation_angle dataset links within }
      TS_ASSERT_THROWS_NOTHING( nxFile.openPath("/entry1/tomo_entry/data") );
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("data") );
      try { nxFile.closeData(); } catch(...) {}
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("rotation_angle") );
      try { nxFile.closeData(); } catch(...) {}

      // Check for entry1/tomo_entry/instrument/detector { data and image_key dataset link within }
      TS_ASSERT_THROWS_NOTHING( nxFile.openPath("/entry1/tomo_entry/instrument/detector") );
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("data") );
      try { nxFile.closeData(); } catch(...) {}
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("image_key") );
      try { nxFile.closeData(); } catch(...) {}

      // Check for entry1/tomo_entry/instrument/sample { and rotation_angle dataset link within }
      TS_ASSERT_THROWS_NOTHING( nxFile.openPath("/entry1/tomo_entry/sample") );
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("rotation_angle") );
      try { nxFile.closeData(); } catch(...) {}
      
      // Check for entry1/log_info { and run_title dataset link within }
      TS_ASSERT_THROWS_NOTHING( nxFile.openPath("/entry1/log_info") );
      TS_ASSERT_THROWS_NOTHING( nxFile.openData("run_title") );
      try { nxFile.closeData(); } catch(...) {}

      nxFile.close();
    }    
  }
 
  void checkNXTomoDimensions(int wsCount)
  {
    // Check that the dimensions for the datasets are correct for the number of workspaces
    NXhandle fileHandle;
    NXstatus status = NXopen(outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if(status!= NX_ERROR)
    {
      ::NeXus::File nxFile(fileHandle);
       
      nxFile.openPath("/entry1/tomo_entry/data");
      nxFile.openData("data");
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[0], wsCount );
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[1], axisSize );
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[2], axisSize );
      nxFile.closeData();
      nxFile.openData("rotation_angle");
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[0], wsCount );
      nxFile.closeData();
        
      nxFile.openPath("/entry1/tomo_entry/instrument/detector");
      nxFile.openData("image_key");
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[0], wsCount );
      nxFile.closeData();
        
      nxFile.openPath("/entry1/log_info");
      nxFile.openData("run_title");
        TS_ASSERT_EQUALS( nxFile.getInfo().dims[0], wsCount );
      nxFile.closeData();
                
      nxFile.close();
    }
  }

  void checkNXTomoRotations(int wsCount)
  {
    // Check that the rotation values are correct for the rotation dataset for the number of workspaces
    NXhandle fileHandle;
    NXstatus status = NXopen(outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if(status!= NX_ERROR)
    {
      ::NeXus::File nxFile(fileHandle);
             
      nxFile.openPath("/entry1/tomo_entry/data");     
      nxFile.openData("rotation_angle");
      std::vector<double> data; 
      nxFile.getData(data);
      for(int i=0;i<wsCount;++i)
        TS_ASSERT_EQUALS( data[i], static_cast<double>((1+i)*5) );
      
      nxFile.closeData();
                      
      nxFile.close();
    }
  }

  void checkNXTomoData(int wsCount)
  {
    // Checks the first {wsCount} data entries are correct - All test data is value 2.0
    NXhandle fileHandle;
    NXstatus status = NXopen(outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if(status!= NX_ERROR)
    {
      ::NeXus::File nxFile(fileHandle);
             
      nxFile.openPath("/entry1/tomo_entry/data");     
      nxFile.openData("data");
      std::vector<double> data; 
      nxFile.getData(data);
      for(int i=0;i<wsCount;++i)
        TS_ASSERT_EQUALS( data[i], 2.0 );
      
      nxFile.closeData();
                      
      nxFile.close();
    }
  }

private:
  IAlgorithm* saver;
  std::string outputFile;
  std::string inputWS;
  int axisSize;
};

#endif /*SAVENXTOMOTEST_H_*/
