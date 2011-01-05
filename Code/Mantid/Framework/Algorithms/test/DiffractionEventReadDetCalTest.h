#ifndef DIFFRACTIONEVENTREADDETCALTEST_H_
#define DIFFRACTIONEVENTREADDETCALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DiffractionEventReadDetCal.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "Poco/File.h"
#include <fstream>
#include <cstring>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class DiffractionEventReadDetCalTest : public CxxTest::TestSuite
{
public:

  void testSNAP()
  {
    LoadEmptyInstrument loaderCAL;

    loaderCAL.initialize();
    loaderCAL.isInitialized();
    loaderCAL.setPropertyValue("Filename",  Kernel::ConfigService::Instance().getString(
        "instrumentDefinition.directory")+"/SNAP_Definition.xml");
    inputFile = loaderCAL.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    DiffractionEventReadDetCal testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    std::string inputFile= "test.DetCal";
    std::ofstream out(inputFile.c_str());
    out << "5      1    256    256 15.8190 15.8190  0.2000  55.33   50.0000   16.7548  -16.7548  0.00011 -0.00002  1.00000  0.00000  1.00000  0.00000\n";
    out.close();
    testerCAL.setPropertyValue("Filename", inputFile);
    inputFile = testerCAL.getPropertyValue("Filename");

    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isExecuted());


    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    
    //Get some stuff from the input workspace
    IInstrument_sptr ins = output->getInstrument();
    if (!ins)
      throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

    boost::shared_ptr<Mantid::Geometry::IComponent> det = ins->getComponentByName("E1");
    V3D PosNew = det->getPos();
    Quat rot = det->getRotation();
    TS_ASSERT_EQUALS(PosNew, V3D(0.500000,0.167548,-0.167548));
    TS_ASSERT_EQUALS(rot, Quat(0.707146,-8.47033e-22,-0.707068,-7.53079e-13));


    // remove file created by this algorithm
    Poco::File(inputFile).remove();
    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);

  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*DIFFRACTIONEVENTREADDETCALTEST_H_*/
