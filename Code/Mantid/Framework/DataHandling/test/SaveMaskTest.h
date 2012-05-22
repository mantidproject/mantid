#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveMask.h"
#include "MantidDataHandling/LoadMask.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"

#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveMaskTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveMaskTest *createSuite() { return new SaveMaskTest(); }

  static void destroySuite( SaveMaskTest *suite ) { delete suite; }

  void test_Initialize()
  {
    SaveMask savealg;
    savealg.initialize();

    TS_ASSERT(savealg.isInitialized());
  }


  /*
   * (1) Load an existing masking file.  It is in 1x1
   * (2) Save masking workspace to file.
   * (3) Load the newly saved file, and compare with original masking workspace
   * Notice: the prerequisit is that LoadMask() is correct.
   */
  void test_SaveFile()
  {
    // 1. Init SaveDetectorMasking
    SaveMask savealg;
    savealg.initialize();

    // 2. Run LoadMask
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    TS_ASSERT_EQUALS(loadfile.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
          AnalysisDataService::Instance().retrieveWS<DataObjects::SpecialWorkspace2D>("PG3Mask");

    // 3. Set property and run
    TS_ASSERT(savealg.setProperty("InputWorkspace", maskws));
    TS_ASSERT(savealg.setProperty("OutputFile", "maskcopy.xml"));

    savealg.execute();
    TS_ASSERT(savealg.isExecuted());

    // Retrieve the full path to file
    std::string file1 = savealg.getPropertyValue("OutputFile");

    // 4. Load the new XML file
    LoadMask loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "POWGEN");
    loadfile2.setProperty("InputFile", file1);
    loadfile2.setProperty("OutputWorkspace", "PG3MaskCopy");

    TS_ASSERT_EQUALS(loadfile2.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws2 =
          AnalysisDataService::Instance().retrieveWS<DataObjects::SpecialWorkspace2D>("PG3MaskCopy");

    // 5. Compare
    TS_ASSERT_EQUALS(maskws->getNumberHistograms(), maskws2->getNumberHistograms());
    for (size_t i = 0; i < maskws->getNumberHistograms(); i ++){
      TS_ASSERT_EQUALS(maskws->dataY(i)[0], maskws2->dataY(i)[0]);
    }

    // 6. Clean the file
    Poco::File cleanfile(file1);
    cleanfile.remove(false);

    return;
  }


  /*
   * Generate a Workspace with grouped detectors
   * and some of them are masked
   */
  void setUpWSwGroupedDetectors(std::set<size_t> maskwsindexList, const std::string & name = "testSpace")
  {
    // 1. Instrument
    // a) By this will create an instrument with 9 detectors
    Mantid::Geometry::Instrument_sptr instr = boost::dynamic_pointer_cast<Mantid::Geometry::Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(1, false));
    // b) Detectors ID
    for (detid_t i = 10; i <= 36; ++i)
    {
      Mantid::Geometry::Detector *d = new Mantid::Geometry::Detector("det", i, 0);
      instr->markAsDetector(d);
    }

    // 2. Workspace
    MatrixWorkspace_sptr space;
    space = WorkspaceFactory::Instance().create("EventWorkspace",9, 6, 5);
    Mantid::DataObjects::EventWorkspace_sptr spaceEvent = boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(space);

    // b) Set by each spectrum
    MantidVecPtr x,vec;
    vec.access().resize(5,1.0);
    detid_t currdetid = 1;
    for (int j = 0; j < 9; ++j)
    {
      //Just one event per pixel
      for (int k = 0; k < 4; ++ k)
      {
        Mantid::DataObjects::TofEvent event(1.23*(1.+double(k)*0.01), int64_t(4.56));
        spaceEvent->getEventList(j).addEventQuickly(event);
      }
      // spaceEvent->getEventList(j).setDetectorID(j);
      spaceEvent->getAxis(1)->spectraNo(j) = j;
      Mantid::API::ISpectrum* spec = spaceEvent->getSpectrum(j);
      std::vector<int> detids;
      for (size_t k = 0; k < 4; ++k)
      {
        detids.push_back(currdetid);
        currdetid ++;
      }
      spec->addDetectorIDs(detids);
    }
    spaceEvent->doneAddingEventLists();
    x.access().push_back(0.0);
    x.access().push_back(10.0);
    spaceEvent->setAllX(x);

    space->setInstrument(instr);
    space->generateSpectraMap();

    // 3. Mask some spectra
    for (std::set<size_t>::iterator wsiter = maskwsindexList.begin(); wsiter != maskwsindexList.end(); ++ wsiter)
    {
      size_t wsindex = *wsiter;
      space->maskWorkspaceIndex(wsindex);
    }

    // 4. Register the workspace in the data service
    AnalysisDataService::Instance().addOrReplace(name, space);

    return;
  }

  void test_SaveFileGroupedDetectors()
  {
    // 1. Init SaveDetectorMasking
    SaveMask savealg;
    savealg.initialize();

    // 2. Create input workspace
    std::set<size_t> maskwsindexList;
    maskwsindexList.insert(1);
    maskwsindexList.insert(3);
    maskwsindexList.insert(6);
    std::string wsname("GroupedDetectorWS");
    setUpWSwGroupedDetectors(maskwsindexList, wsname);
    DataObjects::EventWorkspace_const_sptr inpws =
          AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>(wsname);

    // 3. Set property and run
    API::MatrixWorkspace_const_sptr inpmatrixws = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(inpws);
    TS_ASSERT(inpmatrixws);

    TS_ASSERT(savealg.setProperty("InputWorkspace", wsname));
    TS_ASSERT(savealg.setProperty("OutputFile", "groupeddetmask.xml"));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());

    // Retrieve the full path to file
    std::string file1 = savealg.getPropertyValue("OutputFile");

    // 4. Load the new XML file
    LoadMask loadfile2;
    loadfile2.initialize();

    // Note: must put a real instrument or LoadMask() won't work.
    loadfile2.setProperty("Instrument", "POWGEN");
    loadfile2.setProperty("InputFile", file1);
    loadfile2.setProperty("OutputWorkspace", "PG3MaskCopy");

    TS_ASSERT(loadfile2.execute());

    DataObjects::MaskWorkspace_const_sptr maskws2 =
          AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("PG3MaskCopy");
    TS_ASSERT(maskws2);
    if (!maskws2)
      return;

    // 5. Check
    std::vector<detid_t> detidinfile = loadfile2.getProperty("ToMaskDetectorIDsList");
    TS_ASSERT_EQUALS(detidinfile.size(), 12);
    std::sort(detidinfile.begin(), detidinfile.end());
    for (size_t i = 0; i < 4; i ++)
      TS_ASSERT_EQUALS(detidinfile[i], 5+i);
    for (size_t i = 0; i < 4; i ++)
      TS_ASSERT_EQUALS(detidinfile[i+4], 13+i);
    for (size_t i = 0; i < 4; i ++)
      TS_ASSERT_EQUALS(detidinfile[i+8], 25+i);

    // 6. Clean the file
    Poco::File cleanfile(file1);
    cleanfile.remove(false);

    return;
  }

};


#endif /* MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_ */
