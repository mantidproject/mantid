// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using ScopedFileHelper::ScopedFile;

namespace {
/// static Logger definition
Logger g_log("LoadEmptyInstrumentTest");
} // namespace

class LoadEmptyInstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEmptyInstrumentTest *createSuite() { return new LoadEmptyInstrumentTest(); }
  static void destroySuite(LoadEmptyInstrumentTest *suite) { delete suite; }

  /// Helper that checks that each spectrum has one detector
  void check_workspace_detectors(const MatrixWorkspace_sptr &output, size_t numberDetectors) {
    TS_ASSERT_EQUALS(output->getNumberHistograms(), numberDetectors);
    for (size_t i = 0; i < output->getNumberHistograms(); i++) {
      TS_ASSERT_EQUALS(output->getSpectrum(i).getDetectorIDs().size(), 1);
    }
  }

  // Helper method to create the IDF File.
  ScopedFile createIDFFileObject(const std::string &idf_filename, const std::string &idf_file_contents) {
    const std::string instrument_dir = ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/";

    return ScopedFile(idf_file_contents, idf_filename, instrument_dir);
  }

  void testExecSLS() {
    g_log.notice("\ntestExecSLS...");

    LoadEmptyInstrument loaderSLS;
    loaderSLS.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT(loaderSLS.isInitialized());
    loaderSLS.setPropertyValue("Filename", "SANDALS_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loaderSLS.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(result, inputFile);

    TS_ASSERT_THROWS_NOTHING(result = loaderSLS.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    loaderSLS.execute();

    TS_ASSERT(loaderSLS.isExecuted());

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 683);
    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecENGINEX() {
    g_log.notice("\ntestExecENGINEX...");

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

    TS_ASSERT_THROWS_NOTHING(result = loaderSLS.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT(loaderSLS.isExecuted());

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 2502);
  }

  void testExecMUSR() {
    g_log.notice("\ntestExecMUSR...");

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

    TS_ASSERT_THROWS_NOTHING(result = loaderMUSR.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderMUSR.execute());

    TS_ASSERT(loaderMUSR.isExecuted());

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the total number of elements in the map for SLS
    check_workspace_detectors(output, 64);
  }

  void testLoadFromNexus() {
    g_log.notice("\ntestLoadFromNexus...");

    LoadEmptyInstrument loader;
    loader.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());

    inputFile = "PG3_46577.nxs.h5";
    wsName = "LEIT_LoadFromNexus";
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", inputFile));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", wsName));

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Retrieve the workspace that has the instrument
    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Check the instrument name
    TS_ASSERT(ws->getInstrumentName() == "POWGEN");

    // Check the total number of detectors
    check_workspace_detectors(ws, 43121);
  }

  void testLoadFromNexusNoIDF() {
    g_log.notice("\ntestLoadFromNexusNoIDF...");

    LoadEmptyInstrument loader;
    loader.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());

    inputFile = "PG3_2583.nxs";
    wsName = "LEIT_LoadFromNexusNoIDF";
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", inputFile));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", wsName));

    std::string err_msg{"No instrument XML definition found"};
    TS_ASSERT_THROWS_EQUALS(loader.execute(), const std::runtime_error &e, std::string(e.what()).find(err_msg), 0);

    TS_ASSERT(!loader.isExecuted())
  }

  void testLoadFileWithInvalidExtension() {
    g_log.notice("\ntestLoadFileWithInvalidExtension...");

    LoadEmptyInstrument loader;
    loader.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());

    inputFile = "abc.xyz";
    wsName = "LEIT_testLoadFileWithInvalidExtension";
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", inputFile));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", wsName));

    std::ostringstream os;
    os << "Instrument file name \"" << inputFile << "\" has an invalid extension.";
    TS_ASSERT_THROWS_EQUALS(loader.execute(), const std::runtime_error &e, std::string(e.what()).find(os.str()), 0);

    TS_ASSERT(!loader.isExecuted())
  }

  // Unlike testLoadFileWithInvalidExtension, this test doesn't test the LoadEmptyInstrument algorithm per se.
  // It only tests a stand-alone method for retrieving valid instrument file name extensions.
  void testRetrieveValidInstrumentFilenameExtension() {
    g_log.notice("\ntestRetrieveValidInstrumentFilenameExtension...");

    std::string inputFile;
    std::string stem{"abc"};

    // Test some invalid extensions
    std::ostringstream os;
    std::vector invalid_extensions{
        "", ".", ".xyz", ".uvw.xyz", ".rst.uvw.xyz", ".nxs.h5.xyz", ".rst.nxs.uvw", ".rst.nxs.h5.uvw"};
    for (auto const &ext : invalid_extensions) {
      inputFile = stem + ext;
      os.str("");
      os << "Instrument file name \"" << inputFile << "\" has an invalid extension.";
      TS_ASSERT_THROWS_EQUALS(LoadEmptyInstrument::retrieveValidInstrumentFilenameExtension(inputFile),
                              const std::runtime_error &e, std::string(e.what()), os.str());
    }

    // Test all valid extensions
    std::vector<std::string> valid_extensions = LoadEmptyInstrument::getValidInstrumentFilenameExtensions();
    for (auto const &ext : valid_extensions) {
      inputFile = stem + ext;
      TS_ASSERT_EQUALS(LoadEmptyInstrument::retrieveValidInstrumentFilenameExtension(inputFile), ext);
    }
  }

  void testParameterTags() {
    g_log.notice("\ntestParameterTags...");

    LoadEmptyInstrument loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "unit_testing/IDF_for_UNIT_TESTING2.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // get parameter map
    const auto &paramMap = ws->constInstrumentParameters();

    // check that parameter have been read into the instrument parameter map
    const auto &componentInfo = ws->componentInfo();
    const auto &paramInfo = *componentInfo.sharedParameterInfo();
    const auto monitors = ws->getInstrument()->getComponentByName("monitors");
    const auto monitor = componentInfo.indexOf(monitors->getComponentID());
    TS_ASSERT_DELTA(componentInfo.position(monitor).X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(componentInfo.position(monitor).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(componentInfo.position(monitor).Z(), 0.0, 0.0001);

    // get detector corresponding to workspace index 0
    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &detectorInfo = ws->detectorInfo();
    const auto det = spectrumInfo.spectrumDefinition(0)[0].first;

    TS_ASSERT_EQUALS(detectorInfo.detid(det), 1001);
    TS_ASSERT_EQUALS(componentInfo.name(det), "upstream_monitor_det");
    TS_ASSERT_DELTA(spectrumInfo.position(0).X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(spectrumInfo.position(0).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(spectrumInfo.position(0).Z(), 0.0, 0.0001);

    Parameter_sptr param = paramInfo.get(det, "boevs2");
    TS_ASSERT_DELTA(param->value<double>(), 16.0, 0.0001);

    param = paramInfo.get(det, "boevs3");
    TS_ASSERT_DELTA(param->value<double>(), 32.0, 0.0001);

    param = paramInfo.get(det, "boevs");
    TS_ASSERT(param == nullptr);

    param = paramInfo.getRecursive(componentInfo, det, "boevs", "double");
    TS_ASSERT_DELTA(param->value<double>(), 8.0, 0.0001);

    param = paramInfo.getRecursive(componentInfo, det, "fiddo", "fitting");
    const FitParameter &fitParam = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam.getValue(), 84.0, 0.0001);
    TS_ASSERT(fitParam.getTie().compare("") == 0);
    TS_ASSERT(fitParam.getFunction().compare("somefunction") == 0);

    param = paramInfo.getRecursive(componentInfo, det, "toplevel", "fitting");
    const FitParameter &fitParam1 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam1.getValue(), 100.0, 0.0001);
    TS_ASSERT(fitParam1.getTie().compare("") == 0);
    TS_ASSERT(fitParam1.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam1.getConstraint().compare("80 < toplevel < 120") == 0);
    TS_ASSERT(!fitParam1.getLookUpTable().containData());

    param = paramInfo.getRecursive(componentInfo, det, "toplevel2", "fitting");
    const FitParameter &fitParam2 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(0), -48.5, 0.0001);
    TS_ASSERT_DELTA(fitParam2.getValue(5), 1120.0, 0.0001);
    TS_ASSERT(fitParam2.getTie().compare("") == 0);
    TS_ASSERT(fitParam2.getFunction().compare("somefunction") == 0);
    TS_ASSERT(fitParam2.getConstraint().compare("") == 0);
    TS_ASSERT(fitParam2.getLookUpTable().containData());
    TS_ASSERT(fitParam2.getLookUpTable().getMethod().compare("linear") == 0);
    TS_ASSERT(fitParam2.getLookUpTable().getXUnit()->unitID().compare("TOF") == 0);
    TS_ASSERT(fitParam2.getLookUpTable().getYUnit()->unitID().compare("dSpacing") == 0);

    param = paramInfo.getRecursive(componentInfo, det, "formula", "fitting");
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

    param = paramInfo.getRecursive(componentInfo, det, "percentage", "fitting");
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

    const auto det1008Index = detectorInfo.indexOf(1008);
    TS_ASSERT_EQUALS(detectorInfo.detid(det1008Index), 1008);
    TS_ASSERT_EQUALS(componentInfo.name(det1008Index), "combined translation6");
    param = paramInfo.get(det1008Index, "fjols");
    TS_ASSERT_DELTA(param->value<double>(), 20.0, 0.0001);
    param = paramInfo.get(det1008Index, "nedtur");
    TS_ASSERT_DELTA(param->value<double>(), 77.0, 0.0001);

    // test that can hold of "string" parameter in two ways
    std::shared_ptr<const Instrument> i = ws->getInstrument();
    std::shared_ptr<const IComponent> ptrNickelHolder = i->getComponentByName("nickel-holder");
    std::string dummyString = paramMap.getString(&(*ptrNickelHolder), "fjols-string");
    TS_ASSERT(dummyString.compare("boevs") == 0);
    std::vector<std::string> dummyStringVec = paramMap.getString("nickel-holder", "fjols-string");
    TS_ASSERT(dummyStringVec[0].compare("boevs") == 0);

    // check if combined translation works
    const auto det1003Index = detectorInfo.indexOf(1003);
    TS_ASSERT_EQUALS(componentInfo.name(det1003Index), "combined translation");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1003Index), 1003);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).Z(), 0.0, 0.0001);

    const auto det1004Index = detectorInfo.indexOf(1004);
    TS_ASSERT_EQUALS(componentInfo.name(det1004Index), "combined translation2");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1004Index), 1004);
    TS_ASSERT_DELTA(componentInfo.relativePosition(det1004Index).X(), 10.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1004Index).Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1004Index).Z(), 3.0, 0.0001);

    const auto det1005Index = detectorInfo.indexOf(1005);
    TS_ASSERT_EQUALS(componentInfo.name(det1005Index), "combined translation3");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1005Index), 1005);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).Z(), 0.0, 0.0001);

    const auto det1006Index = detectorInfo.indexOf(1006);
    TS_ASSERT_EQUALS(componentInfo.name(det1006Index), "combined translation4");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1006Index), 1006);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).Z(), 0.0, 0.0001);

    const auto det1007Index = detectorInfo.indexOf(1007);
    TS_ASSERT_EQUALS(componentInfo.name(det1007Index), "combined translation5");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1007Index), 1007);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).Z(), 0.0, 0.0001);

    TS_ASSERT_EQUALS(componentInfo.name(det1008Index), "combined translation6");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1008Index), 1008);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).X(), 12.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).Z(), 0.0, 0.0001);

    const auto det1009Index = detectorInfo.indexOf(1009);
    TS_ASSERT_EQUALS(componentInfo.name(det1009Index), "combined translation7");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1009Index), 1009);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).Z(), 0.0, 0.0001);

    const auto det1010Index = detectorInfo.indexOf(1010);
    TS_ASSERT_EQUALS(componentInfo.name(det1010Index), "combined translation8");
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).Z(), 8.0, 0.0001);

    const auto det1011Index = detectorInfo.indexOf(1011);
    TS_ASSERT_EQUALS(componentInfo.name(det1011Index), "combined translation9");
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).Z(), -8.0, 0.0001);

    const auto det1012Index = detectorInfo.indexOf(1012);
    TS_ASSERT_EQUALS(componentInfo.name(det1012Index), "combined translation10");
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).Z(), 0.0, 0.0001);

    const auto det1013Index = detectorInfo.indexOf(1013);
    TS_ASSERT_EQUALS(componentInfo.name(det1013Index), "combined translation11");
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).Z(), 0.0, 0.0001);

    // test parameter rotation
    const auto det1200Index = detectorInfo.indexOf(1200);
    TS_ASSERT_EQUALS(componentInfo.name(det1200Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1200Index), 1200);
    TS_ASSERT_DELTA(detectorInfo.position(det1200Index).X(), 10.5, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1200Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1200Index).Z(), -0.866, 0.0001);

    const auto det1201Index = detectorInfo.indexOf(1201);
    TS_ASSERT_EQUALS(componentInfo.name(det1201Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1201Index), 1201);
    TS_ASSERT_DELTA(detectorInfo.position(det1201Index).X(), 10.5, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1201Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1201Index).Z(), -0.866, 0.0001);

    const auto det1202Index = detectorInfo.indexOf(1202);
    TS_ASSERT_EQUALS(componentInfo.name(det1202Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1202Index), 1202);
    TS_ASSERT_DELTA(detectorInfo.position(det1202Index).X(), 10, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1202Index).Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1202Index).Z(), 0, 0.0001);

    const auto det1203Index = detectorInfo.indexOf(1203);
    TS_ASSERT_EQUALS(componentInfo.name(det1203Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1203Index), 1203);
    TS_ASSERT_DELTA(detectorInfo.position(det1203Index).X(), 10, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1203Index).Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1203Index).Z(), 0, 0.0001);

    const auto det1204Index = detectorInfo.indexOf(1204);
    TS_ASSERT_EQUALS(componentInfo.name(det1204Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1204Index), 1204);
    TS_ASSERT_DELTA(detectorInfo.position(det1204Index).X(), 10, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1204Index).Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1204Index).Z(), 0, 0.0001);

    const auto det1205Index = detectorInfo.indexOf(1205);
    TS_ASSERT_EQUALS(componentInfo.name(det1205Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1205Index), 1205);
    TS_ASSERT_DELTA(detectorInfo.position(det1205Index).X(), 10, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1205Index).Y(), 1.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1205Index).Z(), 0, 0.0001);

    const auto det1206Index = detectorInfo.indexOf(1206);
    TS_ASSERT_EQUALS(componentInfo.name(det1206Index), "param rot-test");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1206Index), 1206);
    TS_ASSERT_DELTA(detectorInfo.position(det1206Index).X(), 10, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1206Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1206Index).Z(), 1, 0.0001);

    // testing r-position, t-position and p-position parameters
    std::shared_ptr<const IComponent> ptrRTP_Test = i->getComponentByName("rtpTest1");
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
    g_log.notice("\ntestIDFwhenSphericalOffsetSet...");

    LoadEmptyInstrument loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "unit_testing/IDF_for_UNIT_TESTING4.xml");
    inputFile = loader.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentParamTest";
    loader.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    std::shared_ptr<const Instrument> i = ws->getInstrument();

    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();

    // check if combined translation works
    const auto det1001Index = detectorInfo.indexOf(1001);
    TS_ASSERT_EQUALS(componentInfo.name(det1001Index), "combined translationA");
    TS_ASSERT_DELTA(detectorInfo.position(det1001Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1001Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1001Index).Z(), 10.0, 0.0001);

    const auto det1002Index = detectorInfo.indexOf(1002);
    TS_ASSERT_EQUALS(componentInfo.name(det1002Index), "combined translationB");
    TS_ASSERT_DELTA(detectorInfo.position(det1002Index).X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1002Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1002Index).Z(), 0.0, 0.0001);

    const auto det1003Index = detectorInfo.indexOf(1003);
    TS_ASSERT_EQUALS(componentInfo.name(det1003Index), "combined translation");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1003Index), 1003);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).X(), 20.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1003Index).Z(), 0.0, 0.0001);

    const auto det1004Index = detectorInfo.indexOf(1004);
    TS_ASSERT_EQUALS(componentInfo.name(det1004Index), "combined translation2");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1004Index), 1004);
    TS_ASSERT_DELTA(detectorInfo.position(det1004Index).X(), 25.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1004Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1004Index).Z(), 0.0, 0.0001);

    const auto det1005Index = detectorInfo.indexOf(1005);
    TS_ASSERT_EQUALS(componentInfo.name(det1005Index), "combined translation3");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1005Index), 1005);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1005Index).Z(), 28.0, 0.0001);

    const auto det1006Index = detectorInfo.indexOf(1006);
    TS_ASSERT_EQUALS(componentInfo.name(det1006Index), "combined translation4");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1006Index), 1006);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1006Index).Z(), 28.0, 0.0001);

    const auto det1007Index = detectorInfo.indexOf(1007);
    TS_ASSERT_EQUALS(componentInfo.name(det1007Index), "combined translation5");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1007Index), 1007);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1007Index).Z(), 28.0, 0.0001);

    const auto det1008Index = detectorInfo.indexOf(1008);
    TS_ASSERT_EQUALS(componentInfo.name(det1008Index), "combined translation6");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1008Index), 1008);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1008Index).Z(), 28.0, 0.0001);

    const auto det1009Index = detectorInfo.indexOf(1009);
    TS_ASSERT_EQUALS(componentInfo.name(det1009Index), "combined translation7");
    TS_ASSERT_EQUALS(detectorInfo.detid(det1009Index), 1009);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1009Index).Z(), 19.0, 0.0001);

    const auto det1010Index = detectorInfo.indexOf(1010);
    TS_ASSERT_EQUALS(componentInfo.name(det1010Index), "combined translation8");
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1010Index).Z(), 8.0, 0.0001);

    const auto det1011Index = detectorInfo.indexOf(1011);
    TS_ASSERT_EQUALS(componentInfo.name(det1011Index), "combined translation9");
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1011Index).Z(), -8.0, 0.0001);

    const auto det1012Index = detectorInfo.indexOf(1012);
    TS_ASSERT_EQUALS(componentInfo.name(det1012Index), "combined translation10");
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).Y(), 8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1012Index).Z(), 0.0, 0.0001);

    const auto det1013Index = detectorInfo.indexOf(1013);
    TS_ASSERT_EQUALS(componentInfo.name(det1013Index), "combined translation11");
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).X(), 11.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).Y(), -8.0, 0.0001);
    TS_ASSERT_DELTA(detectorInfo.position(det1013Index).Z(), 0.0, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  // also test that when loading an instrument a 2nd time that parameters
  // defined in instrument gets loaded as well
  void testToscaParameterTags() {
    g_log.notice("\ntestToscaParameterTags...");

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

    // get detector corresponding to workspace index 0
    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();
    const auto &paramInfo = *componentInfo.sharedParameterInfo();
    const auto det = spectrumInfo.spectrumDefinition(69)[0].first;

    TS_ASSERT_EQUALS(detectorInfo.detid(det), 78);
    TS_ASSERT_EQUALS(componentInfo.name(det), "Detector #70");

    Parameter_sptr param = paramInfo.get(det, "Efixed");
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

    const auto &spectrumInfoAfter = ws->spectrumInfo();
    const auto &detectorInfoAfter = ws->detectorInfo();
    const auto &componentInfoAfter = ws->componentInfo();
    const auto &paramInfoAfter = *componentInfoAfter.sharedParameterInfo();
    const auto detAfter = spectrumInfoAfter.spectrumDefinition(69)[0].first;

    TS_ASSERT_EQUALS(detectorInfoAfter.detid(detAfter), 78);
    TS_ASSERT_EQUALS(componentInfoAfter.name(detAfter), "Detector #70");

    param = paramInfoAfter.get(detAfter, "Efixed");
    TS_ASSERT_DELTA(param->value<double>(), 4.00000, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  // also test that when loading an instrument a 2nd time that parameters
  // defined in instrument gets loaded as well
  void testHRPDParameterTags() {
    g_log.notice("\ntestHRPDParameterTags...");

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

    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();
    const auto &paramInfo = *componentInfo.sharedParameterInfo();
    const auto det1100Index = detectorInfo.indexOf(1100); // should be a detector from bank_bsk
    Parameter_sptr param = paramInfo.getRecursive(componentInfo, det1100Index, "S", "fitting");
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

    const auto &detectorInfoAfter = ws->detectorInfo();
    const auto &componentInfoAfter = ws->componentInfo();
    const auto &paramInfoAfter = *componentInfoAfter.sharedParameterInfo();
    const auto detAfterIndex = detectorInfoAfter.indexOf(1100); // should be a detector from bank_bsk
    param = paramInfoAfter.getRecursive(componentInfoAfter, detAfterIndex, "S", "fitting");
    const FitParameter &fitParam2 = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(1.0), 11.8159, 0.0001);
    TS_ASSERT(fitParam2.getFunction().compare("BackToBackExponential") == 0);

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsName2);
  }

  void testGEMParameterTags() {
    g_log.notice("\ntestGEMParameterTags...");

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

    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();
    const auto &paramInfo = *componentInfo.sharedParameterInfo();
    const auto det = spectrumInfo.spectrumDefinition(101)[0].first;
    TS_ASSERT_EQUALS(detectorInfo.detid(det), 102046);
    TS_ASSERT_EQUALS(componentInfo.name(det), "Det45");
    Parameter_sptr param = paramInfo.getRecursive(componentInfo, det, "Alpha0", "fitting");
    const FitParameter &fitParam = param->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam.getValue(0.0), 0.734079, 0.0001);

    const auto det1 = spectrumInfo.spectrumDefinition(501)[0].first;
    TS_ASSERT_EQUALS(detectorInfo.detid(det1), 211001);
    Parameter_sptr param1 = paramInfo.getRecursive(componentInfo, det1, "Alpha0", "fitting");
    const FitParameter &fitParam1 = param1->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam1.getValue(0.0), 0.734079, 0.0001);

    const auto det2 = spectrumInfo.spectrumDefinition(341)[0].first;
    TS_ASSERT_EQUALS(detectorInfo.detid(det2), 201001);
    Parameter_sptr param2 = paramInfo.getRecursive(componentInfo, det2, "Alpha0", "fitting");
    const FitParameter &fitParam2 = param2->value<FitParameter>();
    TS_ASSERT_DELTA(fitParam2.getValue(0.0), 0.734079, 0.0001);
    // TS_ASSERT( fitParam2.getTie().compare("Alpha0=0.734079") == 0 );
    TS_ASSERT(fitParam2.getFunction().compare("IkedaCarpenterPV") == 0);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_DUM_Instrument_asEventWorkspace() {
    g_log.notice("\ntest_DUM_Instrument_asEventWorkspace...");

    do_test_DUM_Instrument(true);
  }

  void test_DUM_Instrument() {
    g_log.notice("\ntest_DUM_Instrument...");

    do_test_DUM_Instrument(false);
  }

  void do_test_DUM_Instrument(bool asEvent) {
    LoadEmptyInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "unit_testing/DUM_Definition.xml");
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
      EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(eventWS);
    }

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4);
    if (ws->getNumberHistograms() < 4)
      return;

    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();
    const auto &paramInfo = *componentInfo.sharedParameterInfo();
    const auto det = spectrumInfo.spectrumDefinition(1)[0].first;
    TS_ASSERT_EQUALS(detectorInfo.detid(det), 1);
    TS_ASSERT_EQUALS(componentInfo.name(det), "pixel");
    Parameter_sptr param = paramInfo.get(det, "tube_pressure");
    TS_ASSERT_DELTA(param->value<double>(), 10.0, 0.0001);
    param = paramInfo.get(det, "tube_thickness");
    TS_ASSERT_DELTA(param->value<double>(), 0.0008, 0.0001);
    param = paramInfo.get(det, "tube_temperature");
    TS_ASSERT_DELTA(param->value<double>(), 290.0, 0.0001);

    // same tests as above but using getNumberParameter()
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det, "tube_pressure"))[0], 10.0, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det, "tube_thickness"))[0], 0.0008, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det, "tube_temperature"))[0], 290.0, 0.0001);
    const auto det2 = spectrumInfo.spectrumDefinition(2)[0].first;
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det2, "tube_pressure"))[0], 10.0, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det2, "tube_thickness"))[0], 0.0008, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det2, "tube_temperature"))[0], 290.0, 0.0001);
    const auto det3 = spectrumInfo.spectrumDefinition(3)[0].first;
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det3, "tube_pressure"))[0], 10.0, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det3, "tube_thickness"))[0], 0.0008, 0.0001);
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det3, "tube_temperature"))[0], 290.0, 0.0001);

    // demonstrate recursive look-up: tube_pressure2 defined in 'dummy' but
    // accessed from 'pixel'
    const auto det1 = spectrumInfo.spectrumDefinition(1)[0].first;
    TS_ASSERT_DELTA((componentInfo.getNumberParameter(det1, "tube_pressure2"))[0], 35.0, 0.0001);

    // Alternative way of doing a recursive look-up
    param = paramInfo.getRecursive(componentInfo, det1, "tube_pressure2");
    TS_ASSERT_DELTA(param->value<double>(), 35.0, 0.0001);

    // And finally demonstrate that the get() method does not perform recursive
    // look-up
    param = paramInfo.get(det1, "tube_pressure2");
    TS_ASSERT(param == nullptr);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_BIOSANS_Instrument() {
    g_log.notice("\ntest_BIOSANS_Instrument...");

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

    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &componentInfo = ws->componentInfo();
    const auto det = spectrumInfo.spectrumDefinition(1)[0].first;
    TS_ASSERT_EQUALS((componentInfo.getNumberParameter(det, "number-of-x-pixels"))[0], 192);

    Instrument_const_sptr inst = ws->getInstrument();
    TS_ASSERT_EQUALS((inst->getNumberParameter("number-of-x-pixels")).size(), 1);
    TS_ASSERT_EQUALS((inst->getNumberParameter("number-of-x-pixels"))[0], 192);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testSANS2D() {
    g_log.notice("\ntestSANS2D...");

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

    double pixelLength = 0.0051;
    double bankLength = 192 * pixelLength;

    double startX = -bankLength / 2 + pixelLength / 2;
    double startY = -bankLength / 2 + pixelLength / 2;

    const auto &detectorInfo = ws->detectorInfo();
    const auto &componentInfo = ws->componentInfo();
    for (int iy = 0; iy <= 191; iy++) {
      for (int ix = 0; ix <= 191; ix++) {
        size_t index = detectorInfo.indexOf(1000000 + iy * 1000 + ix);
        TS_ASSERT_DELTA(detectorInfo.position(index).X(), startX + pixelLength * ix, 0.0000001);
        TS_ASSERT_DELTA(detectorInfo.position(index).Y(), startY + pixelLength * iy, 0.0000001);
        TS_ASSERT_DELTA(detectorInfo.position(index).Z(), 23.281, 0.0000001);
      }

      for (int ix = 0; ix <= 191; ix++) {
        size_t index = detectorInfo.indexOf(2000000 + iy * 1000 + ix);
        TS_ASSERT_DELTA(detectorInfo.position(index).X(), startX + pixelLength * ix + 1.1, 0.0000001);
        TS_ASSERT_DELTA(detectorInfo.position(index).Y(), startY + pixelLength * iy, 0.0000001);
        TS_ASSERT_DELTA(detectorInfo.position(index).Z(), 23.281, 0.0000001);
      }
    }

    // check for solid angle also
    const auto det1000000Index = detectorInfo.indexOf(1000000);
    const double solidAngle = componentInfo.solidAngle(det1000000Index, detectorInfo.samplePosition());
    TS_ASSERT_DELTA(10E5 * solidAngle, 6.23454, 0.00001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testCheckIfVariousInstrumentsLoad() {
    g_log.notice("\ntestCheckIfVariousInstrumentsLoad...");

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
    loaderEMU2.setRethrows(true);
    loaderEMU2.setPropertyValue("Filename", "unit_testing/EMU_for_UNIT_TESTING.XML");
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
    g_log.notice("\ntestIDFFileWithNoDetector...");

    const std::string instrumentName = "Minimal_Definition";
    const std::string idfFilename = instrumentName + "_MissingDetectorIDs.xml";

    const std::string idfFileContents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

  void test_output_workspace_contains_instrument_with_expected_name() {
    g_log.notice("\ntest_output_workspace_contains_instrument_with_expected_name...");

    LoadEmptyInstrument alg;
    alg.setChild(true);
    const std::string inputFile = "unit_testing/SMALLFAKE_example_geometry.hdf5";
    alg.initialize();
    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");

    auto &componentInfo = outputWs->componentInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "SmallFakeTubeInstrument");
  }

  void test_load_loki() {
    g_log.notice("\ntest_load_loki...");

    LoadEmptyInstrument alg;
    alg.setChild(true);
    const std::string inputFile = "LOKI_Definition.hdf5";
    alg.initialize();
    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");

    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "LOKI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 8000);

    TS_ASSERT_EQUALS(0, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[1]);
  }

  void test_load_loki_from_instrument_name() {
    g_log.notice("\ntest_load_loki_from_instrument_name...");

    LoadEmptyInstrument alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("InstrumentName", "LOKI");
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");

    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "LOKI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 8000);

    TS_ASSERT_EQUALS(0, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[1]);
  }

  void test_compare_wish_idf_vs_nexus() {
    g_log.notice("\ntest_compare_wish_idf_vs_nexus...");
    // Now rerun
    LoadEmptyInstrument alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("Filename", "WISH_Definition_10Panels.hdf5");
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    Mantid::API::MatrixWorkspace_sptr wish_nexus = alg.getProperty("OutputWorkspace");

    // Now re-run
    alg.setPropertyValue("Filename", "WISH_Definition_10Panels.xml");
    alg.execute();
    Mantid::API::MatrixWorkspace_sptr wish_xml = alg.getProperty("OutputWorkspace");

    // Sanity check that we are not comparing the same instrument (i.e. via some
    // smart caching)
    TSM_ASSERT("Premise of comparision test broken!", WorkspaceCreationHelper::baseInstrumentID(*wish_xml) !=
                                                          WorkspaceCreationHelper::baseInstrumentID(*wish_nexus));

    const auto &wish_nexus_detinfo = wish_nexus->detectorInfo();
    const auto &wish_xml_detinfo = wish_xml->detectorInfo();
    TS_ASSERT_EQUALS(wish_nexus_detinfo.size(), wish_xml_detinfo.size());
    for (size_t i = 0; i < wish_nexus_detinfo.size(); ++i) {
      TSM_ASSERT_EQUALS("Detector position mismatch", wish_nexus_detinfo.position(i), wish_xml_detinfo.position(i));
    }
  }

  void test_load_from_nxs() {
    // this file has top-level name "entry" where instrument is stored
    std::string const filename1 = "EQSANS_89157.nxs.h5";
    LoadEmptyInstrument alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setPropertyValue("Filename", filename1);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

private:
  std::string inputFile;
  std::string wsName;
};
