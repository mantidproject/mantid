#ifndef LOADEMPTYINSTRUMENTTEST_H_
#define LOADEMPTYINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include <vector>
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using ScopedFileHelper::ScopedFile;

class LoadEmptyInstrumentTest : public CxxTest::TestSuite {
public:
  /// Helper that checks that each spectrum has one detector
  void check_workspace_detectors(MatrixWorkspace_sptr output,
                                 size_t numberDetectors) {
    TS_ASSERT_EQUALS(output->getNumberHistograms(), numberDetectors);
    for (size_t i = 0; i < output->getNumberHistograms(); i++) {
      TS_ASSERT_EQUALS(output->getSpectrum(i)->getDetectorIDs().size(), 1);
    }
  }

  // Helper method to create the IDF File.
  ScopedFile createIDFFileObject(const std::string &idf_filename,
                                 const std::string &idf_file_contents) {
    const std::string instrument_dir =
        ConfigService::Instance().getInstrumentDirectory() +
        "/IDFs_for_UNIT_TESTING/";

    return ScopedFile(idf_file_contents, idf_filename, instrument_dir);
  }

  void testExecSLS() {
    LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT(loaderSLS.isInitialized());
    loaderSLS.setPropertyValue("Filename", "SANDALS_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loaderSLS.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(result, inputFile);

    TS_ASSERT_THROWS_NOTHING(result =
                                 loaderSLS.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT(loaderSLS.isExecuted());

    MatrixWorkspace_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 683);
    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecENGINEX() {
    LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT(loaderSLS.isInitialized());
    loaderSLS.setPropertyValue("Filename", "ENGIN-X_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestEngineX";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loaderSLS.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(result, inputFile);

    TS_ASSERT_THROWS_NOTHING(result =
                                 loaderSLS.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT(loaderSLS.isExecuted());

    MatrixWorkspace_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 2502);
  }

  void testExecMUSR() {
    LoadEmptyInstrument loaderMUSR;

    TS_ASSERT_THROWS_NOTHING(loaderMUSR.initialize());
    TS_ASSERT(loaderMUSR.isInitialized());
    loaderMUSR.setPropertyValue("Filename", "MUSR_Definition.xml");
    inputFile = loaderMUSR.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestMUSR";
    loaderMUSR.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loaderMUSR.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(result, inputFile);

    TS_ASSERT_THROWS_NOTHING(
        result = loaderMUSR.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderMUSR.execute());

    TS_ASSERT(loaderMUSR.isExecuted());

    MatrixWorkspace_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 64);
  }

  void testParameterTags() {

    LoadEmptyInstrument loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename",
                            "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    ParameterMap &paramMap = ws->instrumentParameters();

    // check that parameter have been read into the instrument parameter map
    std::vector<V3D> ret1 = paramMap.getV3D("monitors", "pos");
    TS_ASSERT_DELTA(ret1[0].X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(ret1[0].Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ret1[0].Z(), 0.0, 0.0001);

    // get detector corresponding to workspace index 0
    IDetector_const_sptr det = ws->getDetector(0);

    TS_ASSERT_EQUALS(det->getID(), 1001);
    TS_ASSERT_EQUALS(det->getName(), "upstream_monitor_det");
    TS_ASSERT_DELTA(det->getPos().X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(det->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(det->getPos().Z(), 0.0, 0.0001);

    Parameter_sptr param = paramMap.get(&(*det), "boevs2");
    TS_ASSERT_DELTA(param->value<double>(), 16.0, 0.0001);

    param = paramMap.get(&(*det), "boevs3");
    TS_ASSERT_DELTA(param->value<double>(), 32.0, 0.0001);

    param = paramMap.get(&(*det), "boevs");
    TS_ASSERT(param == NULL);

    param = paramMap.getRecursive(&(*det), "boevs", "double");
    TS_ASSERT_DELTA(param->value<double>(), 8.0, 0.0001);

    param = paramMap.getRecursive(&(*det), "fiddo", "fitting");
    const FitParameter &fitParam = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam.getValue(), 84.0, 0.0001);
    TS_ASSERT(fitParam.getTie().compare("") == 0);
    TS_ASSERT(fitParam.getFunction().compare("somefunction") == 0);

    param = paramMap.getRecursive(&(*det), "toplevel", "fitting");
    const FitParameter &fitParam1 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam1.getValue(), 100.0, 0.0001);
    TS_ASSERT(fitParam1.getTie().compare("") == 0);
    TS_ASSERT(fitParam1.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam1.getConstraint().compare("80 < toplevel < 120") == 0);
    TS_ASSERT(!fitParam1.getLookUpTable().containData());

    param = paramMap.getRecursive(&(*det), "toplevel2", "fitting");
    const FitParameter &fitParam2 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(0), -48.5, 0.0001);
    TS_ASSERT_DELTA(fitParam2.getValue(5), 1120.0, 0.0001);
    TS_ASSERT(fitParam2.getTie().compare("") == 0);
    TS_ASSERT(fitParam2.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam2.getConstraint().compare("") == 0);
    TS_ASSERT(fitParam2.getLookUpTable().containData());
    TS_ASSERT(fitParam2.getLookUpTable().getMethod().compare("linear") == 0);
    TS_ASSERT(fitParam2.getLookUpTable().getXUnit()->unitID().compare("TOF") ==
              0);
    TS_ASSERT(fitParam2.getLookUpTable().getYUnit()->unitID().compare(
                  "dSpacing") == 0);

    param = paramMap.getRecursive(&(*det), "formula", "fitting");
    const FitParameter &fitParam3 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam3.getValue(0), 100.0, 0.0001);
    TS_ASSERT_DELTA(fitParam3.getValue(5), 175.0, 0.0001);
    TS_ASSERT(fitParam3.getTie().compare("") == 0);
    TS_ASSERT(fitParam3.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam3.getConstraint().compare("") == 0);
    TS_ASSERT(!fitParam3.getLookUpTable().containData());
    TS_ASSERT(fitParam3.getFormula().compare("100.0+10*centre+centre^2") == 0);
    TS_ASSERT(fitParam3.getFormulaUnit().compare("TOF") == 0);
    TS_ASSERT(fitParam3.getResultUnit().compare("dSpacing") == 0);

    param = paramMap.getRecursive(&(*det), "percentage", "fitting");
    const FitParameter &fitParam4 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam4.getValue(), 250.0, 0.0001);
    TS_ASSERT(fitParam4.getTie().compare("") == 0);
    TS_ASSERT(fitParam4.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam4.getConstraint().compare("200 < percentage < 300") == 0);
    TS_ASSERT(fitParam4.getConstraintPenaltyFactor().compare("9.1") == 0);
    TS_ASSERT(!fitParam4.getLookUpTable().containData());
    TS_ASSERT(fitParam4.getFormula().compare("") == 0);

    // check reserved keywords
    std::vector<double> dummy = paramMap.getDouble("nickel-holder", "klovn");
    TS_ASSERT_DELTA(dummy[0], 1.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "pos");
    TS_ASSERT_EQUALS(dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "rot");
    TS_ASSERT_EQUALS(dummy.size(), 0);
    dummy = paramMap.getDouble("nickel-holder", "taabe");
    TS_ASSERT_DELTA(dummy[0], 200.0, 0.0001);
    dummy = paramMap.getDouble("nickel-holder", "mistake");
    TS_ASSERT_EQUALS(dummy.size(), 0);

    // check if <component-link> works
    dummy = paramMap.getDouble("nickel-holder", "fjols");
    TS_ASSERT_DELTA(dummy[0], 200.0, 0.0001);

    boost::shared_ptr<const Instrument> i = ws->getInstrument();
    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(1008);
    TS_ASSERT_EQUALS(ptrDet->getID(), 1008);
    TS_ASSERT_EQUALS(ptrDet->getName(), "combined translation6");
    param = paramMap.get(&(*ptrDet), "fjols");
    TS_ASSERT_DELTA(param->value<double>(), 20.0, 0.0001);
    param = paramMap.get(&(*ptrDet), "nedtur");
    TS_ASSERT_DELTA(param->value<double>(), 77.0, 0.0001);

    // test that can hold of "string" parameter in two ways
    boost::shared_ptr<const IComponent> ptrNickelHolder =
        i->getComponentByName("nickel-holder");
    std::string dummyString =
        paramMap.getString(&(*ptrNickelHolder), "fjols-string");
    TS_ASSERT(dummyString.compare("boevs") == 0);
    std::vector<std::string> dummyStringVec =
        paramMap.getString("nickel-holder", "fjols-string");
    TS_ASSERT(dummyStringVec[0].compare("boevs") == 0);

    // check if combined translation works
    ptrDet = i->getDetector(1003);
    TS_ASSERT_EQUALS(ptrDet->getName(), "combined translation");
    TS_ASSERT_EQUALS(ptrDet->getID(), 1003);
    TS_ASSERT_DELTA(ptrDet->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet->getPos().Z(), 0.0, 0.0001);

    boost::shared_ptr<const IDetector> ptrDet1 = i->getDetector(1004);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation2");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1004);
    TS_ASSERT_DELTA(ptrDet1->getRelativePos().X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 3.0, 0.0001);

    ptrDet1 = i->getDetector(1005);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation3");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1005);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1006);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation4");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1006);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1007);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation5");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1007);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1008);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation6");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1008);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1009);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation7");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1009);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1010);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation8");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 8.0, 0.0001);

    ptrDet1 = i->getDetector(1011);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation9");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), -8.0, 0.0001);

    ptrDet1 = i->getDetector(1012);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation10");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1013);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation11");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    // test parameter rotation
    ptrDet1 = i->getDetector(1200);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1200);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10.5, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), -0.866, 0.0001);

    ptrDet1 = i->getDetector(1201);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1201);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10.5, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), -0.866, 0.0001);

    ptrDet1 = i->getDetector(1202);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1202);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0, 0.0001);

    ptrDet1 = i->getDetector(1203);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1203);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0, 0.0001);

    ptrDet1 = i->getDetector(1204);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1204);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0, 0.0001);

    ptrDet1 = i->getDetector(1205);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1205);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0, 0.0001);

    ptrDet1 = i->getDetector(1206);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "param rot-test");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1206);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 10, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 1, 0.0001);

    // testing r-position, t-position and p-position parameters
    boost::shared_ptr<const IComponent> ptrRTP_Test =
        i->getComponentByName("rtpTest1");
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Z(), 20.0, 0.0001);
    ptrRTP_Test = i->getComponentByName("rtpTest2");
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Y(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Z(), 12.0, 0.0001);
    ptrRTP_Test = i->getComponentByName("rtpTest3");
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Y(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Z(), 12.0, 0.0001);
    ptrRTP_Test = i->getComponentByName("rtpTest4");
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Y(), 12.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Z(), 12.0, 0.0001);
    ptrRTP_Test = i->getComponentByName("rtpTest5");
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrRTP_Test->getPos().Z(), 0.0, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  // Tests specific to when  <offsets spherical="delta" /> is set in IDF
  void testIDFwhenSphericalOffsetSet() {

    LoadEmptyInstrument loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename",
                            "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING4.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    boost::shared_ptr<const Instrument> i = ws->getInstrument();

    // check if combined translation works
    boost::shared_ptr<const IDetector> ptrDet1 = i->getDetector(1001);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translationA");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 10.0, 0.0001);

    ptrDet1 = i->getDetector(1002);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translationB");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1003);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1003);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1004);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation2");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1004);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 25.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1005);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation3");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1005);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 28.0, 0.0001);

    ptrDet1 = i->getDetector(1006);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation4");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1006);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 28.0, 0.0001);

    ptrDet1 = i->getDetector(1007);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation5");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1007);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 28.0, 0.0001);

    ptrDet1 = i->getDetector(1008);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation6");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1008);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 28.0, 0.0001);

    ptrDet1 = i->getDetector(1009);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation7");
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1009);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 19.0, 0.0001);

    ptrDet1 = i->getDetector(1010);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation8");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 8.0, 0.0001);

    ptrDet1 = i->getDetector(1011);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation9");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), -8.0, 0.0001);

    ptrDet1 = i->getDetector(1012);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation10");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    ptrDet1 = i->getDetector(1013);
    TS_ASSERT_EQUALS(ptrDet1->getName(), "combined translation11");
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  // also test that when loading in instrument a 2nd time that parameters
  // defined
  // in instrument gets loaded as well
  void testToscaParameterTags() {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOSCA_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamToscaTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    ParameterMap &paramMap = ws->instrumentParameters();

    // get detector corresponding to workspace index 0
    IDetector_const_sptr det = ws->getDetector(69);

    TS_ASSERT_EQUALS(det->getID(), 78);
    TS_ASSERT_EQUALS(det->getName(), "Detector #70");

    Parameter_sptr param = paramMap.get(&(*det), "Efixed");
    TS_ASSERT_DELTA(param->value<double>(), 4.00000, 0.0001);

    AnalysisDataService::Instance().remove(wsName);

    // load the instrument a second time to check that the parameters are still
    // there

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOSCA_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamToscaTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    ParameterMap &paramMap2 = ws->instrumentParameters();

    det = ws->getDetector(69);

    TS_ASSERT_EQUALS(det->getID(), 78);
    TS_ASSERT_EQUALS(det->getName(), "Detector #70");

    param = paramMap2.get(&(*det), "Efixed");
    TS_ASSERT_DELTA(param->value<double>(), 4.00000, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  // also test that when loading in instrument a 2nd time that parameters
  // defined
  // in instrument gets loaded as well
  void testHRPDParameterTags() {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "HRPD_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamHRPDTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    ParameterMap &paramMap = ws->instrumentParameters();

    boost::shared_ptr<const Instrument> i = ws->getInstrument();
    boost::shared_ptr<const IDetector> det =
        i->getDetector(1100); // should be a detector from bank_bsk
    Parameter_sptr param = paramMap.getRecursive(&(*det), "S", "fitting");
    const FitParameter &fitParam1 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam1.getValue(1.0), 11.8159, 0.0001);
    TS_ASSERT(fitParam1.getFunction().compare("BackToBackExponential") == 0);

    // load the instrument a second time to check that the parameters are still
    // there

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "HRPD_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    std::string wsName2 = "LoadEmptyInstrumentParamHRPDTest";
    loader.setPropertyValue("OutputWorkspace", wsName2);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName2);

    // get parameter map
    ParameterMap &paramMap2 = ws->instrumentParameters();

    i = ws->getInstrument();
    det = i->getDetector(1100); // should be a detector from bank_bsk
    param = paramMap2.getRecursive(&(*det), "S", "fitting");
    const FitParameter &fitParam2 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(1.0), 11.8159, 0.0001);
    TS_ASSERT(fitParam2.getFunction().compare("BackToBackExponential") == 0);

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsName2);
  }

  void testGEMParameterTags() {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "GEM_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamGemTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    ParameterMap &paramMap = ws->instrumentParameters();

    IDetector_const_sptr det = ws->getDetector(101);
    TS_ASSERT_EQUALS(det->getID(), 102046);
    TS_ASSERT_EQUALS(det->getName(), "Det45");
    Parameter_sptr param = paramMap.getRecursive(&(*det), "Alpha0", "fitting");
    const FitParameter &fitParam = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam.getValue(0.0), 0.734079, 0.0001);

    IDetector_const_sptr det1 = ws->getDetector(501);
    TS_ASSERT_EQUALS(det1->getID(), 211001);
    Parameter_sptr param1 =
        paramMap.getRecursive(&(*det1), "Alpha0", "fitting");
    const FitParameter &fitParam1 = param1->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam1.getValue(0.0), 0.734079, 0.0001);

    IDetector_const_sptr det2 = ws->getDetector(341);
    TS_ASSERT_EQUALS(det2->getID(), 201001);
    Parameter_sptr param2 =
        paramMap.getRecursive(&(*det2), "Alpha0", "fitting");
    const FitParameter &fitParam2 = param2->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(0.0), 0.734079, 0.0001);
    // TS_ASSERT( fitParam2.getTie().compare("Alpha0=0.734079") == 0 );
    TS_ASSERT(fitParam2.getFunction().compare("IkedaCarpenterPV") == 0);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_DUM_Instrument_asEventWorkspace() { do_test_DUM_Instrument(true); }

  void test_DUM_Instrument() { do_test_DUM_Instrument(false); }

  void do_test_DUM_Instrument(bool asEvent) {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename",
                            "IDFs_for_UNIT_TESTING/DUM_Definition.xml");
    loader.setProperty("MakeEventWorkspace", asEvent);
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyDUMInstrumentTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TS_ASSERT(ws);
    if (!ws)
      return;

    if (asEvent) {
      EventWorkspace_sptr eventWS =
          boost::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(eventWS);
    }

    // get parameter map
    ParameterMap &paramMap = ws->instrumentParameters();

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4);
    if (ws->getNumberHistograms() < 4)
      return;

    IDetector_const_sptr det = ws->getDetector(1);
    TS_ASSERT_EQUALS(det->getID(), 1);
    TS_ASSERT_EQUALS(det->getName(), "pixel");
    Parameter_sptr param = paramMap.get(&(*det), "tube_pressure");
    TS_ASSERT_DELTA(param->value<double>(), 10.0, 0.0001);
    param = paramMap.get(&(*det), "tube_thickness");
    TS_ASSERT_DELTA(param->value<double>(), 0.0008, 0.0001);
    param = paramMap.get(&(*det), "tube_temperature");
    TS_ASSERT_DELTA(param->value<double>(), 290.0, 0.0001);

    // same tests as above but using getNumberParameter()
    TS_ASSERT_DELTA((det->getNumberParameter("tube_pressure"))[0], 10.0,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_thickness"))[0], 0.0008,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_temperature"))[0], 290.0,
                    0.0001);
    det = ws->getDetector(2);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_pressure"))[0], 10.0,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_thickness"))[0], 0.0008,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_temperature"))[0], 290.0,
                    0.0001);
    det = ws->getDetector(3);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_pressure"))[0], 10.0,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_thickness"))[0], 0.0008,
                    0.0001);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_temperature"))[0], 290.0,
                    0.0001);

    // demonstrate recursive look-up: tube_pressure2 defined in 'dummy' but
    // accessed from 'pixel'
    det = ws->getDetector(1);
    TS_ASSERT_DELTA((det->getNumberParameter("tube_pressure2"))[0], 35.0,
                    0.0001);

    // Alternative way of doing a recursive look-up
    param = paramMap.getRecursive(&(*det), "tube_pressure2");
    TS_ASSERT_DELTA(param->value<double>(), 35.0, 0.0001);

    // And finally demonstrate that the get() method does not perform recursive
    // look-up
    param = paramMap.get(&(*det), "tube_pressure2");
    TS_ASSERT(param == NULL);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_BIOSANS_Instrument() {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "BIOSANS_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadBIOSANS";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    IDetector_const_sptr det = ws->getDetector(1);
    TS_ASSERT_EQUALS((det->getNumberParameter("number-of-x-pixels"))[0], 192);

    Instrument_const_sptr inst = ws->getInstrument();
    TS_ASSERT_EQUALS((inst->getNumberParameter("number-of-x-pixels")).size(),
                     1);
    TS_ASSERT_EQUALS((inst->getNumberParameter("number-of-x-pixels"))[0], 192);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testSANS2D() {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "SANS2D_Definition.xml");
    wsName = "LoadEmptyInstrumentParaSans2dTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    boost::shared_ptr<const Instrument> i = ws->getInstrument();

    double pixelLength = 0.0051;
    double bankLength = 192 * pixelLength;

    double startX = -bankLength / 2 + pixelLength / 2;
    double startY = -bankLength / 2 + pixelLength / 2;

    for (int iy = 0; iy <= 191; iy++) {
      for (int ix = 0; ix <= 191; ix++) {
        boost::shared_ptr<const IDetector> ptrDet1 =
            i->getDetector(1000000 + iy * 1000 + ix);
        TS_ASSERT_DELTA(ptrDet1->getPos().X(), startX + pixelLength * ix,
                        0.0000001);
        TS_ASSERT_DELTA(ptrDet1->getPos().Y(), startY + pixelLength * iy,
                        0.0000001);
        TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 23.281, 0.0000001);
      }

      for (int ix = 0; ix <= 191; ix++) {
        boost::shared_ptr<const IDetector> ptrDet1 =
            i->getDetector(2000000 + iy * 1000 + ix);
        TS_ASSERT_DELTA(ptrDet1->getPos().X(), startX + pixelLength * ix + 1.1,
                        0.0000001);
        TS_ASSERT_DELTA(ptrDet1->getPos().Y(), startY + pixelLength * iy,
                        0.0000001);
        TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 23.281, 0.0000001);
      }
    }

    // check for solid angle also
    const V3D samplePos = i->getSample()->getPos();
    boost::shared_ptr<const IDetector> det = i->getDetector(1000000);
    const double solidAngle = det->solidAngle(samplePos);
    TS_ASSERT_DELTA(10E5 * solidAngle, 6.23454, 0.00001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testCheckIfVariousInstrumentsLoad() {
    LoadEmptyInstrument loaderPolRef;
    loaderPolRef.initialize();
    loaderPolRef.setPropertyValue("Filename", "POLREF_Definition.xml");
    wsName = "LoadEmptyInstrumentParamPOLREFTest";
    loaderPolRef.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderPolRef.execute());
    TS_ASSERT(loaderPolRef.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderEMU;
    loaderEMU.initialize();
    loaderEMU.setPropertyValue("Filename", "EMU_Definition_32detectors.xml");
    wsName = "LoadEmptyInstrumentParamEMUTest";
    loaderEMU.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderEMU.execute());
    TS_ASSERT(loaderEMU.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderEMU2;
    loaderEMU2.initialize();
    loaderEMU2.setPropertyValue("Filename", "EMU_Definition_96detectors.xml");
    wsName = "LoadEmptyInstrumentParamEMUTest2";
    loaderEMU2.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderEMU2.execute());
    TS_ASSERT(loaderEMU2.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderARGUS;
    loaderARGUS.initialize();
    loaderARGUS.setPropertyValue("Filename", "ARGUS_Definition.xml");
    wsName = "LoadEmptyInstrumentParamARGUSTest";
    loaderARGUS.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderARGUS.execute());
    TS_ASSERT(loaderARGUS.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    loaderEMU2.initialize();
    loaderEMU2.setPropertyValue(
        "Filename", "IDFs_for_UNIT_TESTING/EMU_for_UNIT_TESTING.XML");
    wsName = "LoadEmptyInstrumentParamEMU2Test";
    loaderEMU2.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderEMU2.execute());
    TS_ASSERT(loaderEMU2.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderINES;
    loaderINES.initialize();
    loaderINES.setPropertyValue("Filename", "INES_Definition.xml");
    wsName = "LoadEmptyInstrumentINESTest";
    loaderINES.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderINES.execute());
    TS_ASSERT(loaderINES.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderHIFI;
    loaderHIFI.initialize();
    loaderHIFI.setPropertyValue("Filename", "HIFI_Definition.xml");
    wsName = "LoadEmptyInstrumentHIFITest";
    loaderHIFI.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderHIFI.execute());
    TS_ASSERT(loaderHIFI.isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    LoadEmptyInstrument loaderVESUVIO;
    loaderVESUVIO.initialize();
    loaderVESUVIO.setPropertyValue("Filename", "VESUVIO_Definition.xml");
    wsName = "LoadEmptyInstrumentVESUVIOTest";
    loaderVESUVIO.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderVESUVIO.execute());
    TS_ASSERT(loaderVESUVIO.isExecuted());

    AnalysisDataService::Instance().remove(wsName);
  }

  /**
   Here we test that the correct exception is thrown, if the instrument has no
   detector
  */
  void testIDFFileWithNoDetector() {
    const std::string instrumentName = "Minimal_Definition";
    const std::string idfFilename = instrumentName + "_MissingDetectorIDs.xml";

    const std::string idfFileContents =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<instrument name=\"" +
        instrumentName +
        "\" valid-from   =\"1900-01-31 23:59:59\" valid-to=\"2100-01-31 "
        "23:59:59\" last-modified=\"2012-10-05 11:00:00\">"
        "<defaults/>"
        "<component type=\"cylinder-right\" >" // this component here is no
                                               // detector and therefore there
                                               // is no idlist
        "<location/>"
        "</component>"
        "<type name=\"cylinder-right\" >" // is=\"detector\" missing
        "<cylinder id=\"some-shape\">"
        "  <centre-of-bottom-base r=\"0.0\" t=\"0.0\" p=\"0.0\" />"
        "  <axis x=\"0.0\" y=\"0.0\" z=\"1.0\" />"
        "  <radius val=\"0.01\" />"
        "  <height val=\"0.03\" />"
        "</cylinder>"
        "</type>"
        "</instrument>";

    ScopedFile idfFile = createIDFFileObject(idfFilename, idfFileContents);

    LoadEmptyInstrument loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", idfFile.getFileName());
    wsName = "LoadEmptyInstrumentNoDetectorsTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    std::string errorMsg("");
    try {
      loader.execute();
      errorMsg = "Exception not thrown";
    } catch (Exception::InstrumentDefinitionError &e) {
      errorMsg = e.what();
    } catch (...) {
      errorMsg = "Unexpected exception";
    }
    TS_ASSERT_EQUALS(errorMsg, "No detectors found in instrument");
  }

private:
  std::string inputFile;
  std::string wsName;
};

#endif /*LOADEMPTYINSTRUMENTTEST_H_*/
