// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADINSTRUMENTTEST_H_
#define LOADINSTRUMENTTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <string>
#include <vector>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

class LoadInstrumentTest : public CxxTest::TestSuite {
public:
  void testInit() {
    LoadInstrument loader;
    TS_ASSERT(!loader.isInitialized());
    loader.initialize();
    TS_ASSERT(loader.isInitialized());
  }

  void testExecHET() {
    LoadInstrument loader;
    loader.initialize();
    loader.setChild(true);

    // create a workspace with some sample data
    int histogramNumber = 2584;
    int timechannels = 100;
    MatrixWorkspace_sptr ws2D = DataObjects::create<Workspace2D>(
        histogramNumber, HistogramData::Points(timechannels, timechannels));

    Points timeChannelsVec(timechannels, LinearGenerator(0.0, 100.0));
    // loop to create data
    for (int i = 0; i < histogramNumber; i++) {
      std::vector<double> v(timechannels);
      std::vector<double> e(timechannels);
      // timechannels
      for (int j = 0; j < timechannels; j++) {
        v[j] = (i + j) % 256;
        e[j] = (i + j) % 78;
      }
      // Populate the workspace.
      ws2D->setPoints(i, timeChannelsVec);
      ws2D->dataY(i) = v;
      ws2D->dataE(i) = e;
    }

    // We want to test id the spectra mapping changes
    TS_ASSERT_EQUALS(ws2D->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws2D->getSpectrum(256).getSpectrumNo(), 257);
    TS_ASSERT_EQUALS(ws2D->getNumberHistograms(), 2584);

    const std::string instrFilename = "HET_Definition.xml";
    loader.setPropertyValue("Filename", instrFilename);
    loader.setProperty("RewriteSpectraMap", OptionalBool(true));
    loader.setProperty("Workspace", ws2D);

    std::string result = loader.getPropertyValue("Filename");
    const std::string::size_type stripPath = result.find_last_of("\\/");
    result = result.substr(stripPath + 1, result.size());
    TS_ASSERT_EQUALS(result, instrFilename);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    TS_ASSERT_EQUALS(loader.getPropertyValue("MonitorList"), "601,602,603,604");

    // Get back the saved workspace
    MatrixWorkspace_sptr output = loader.getProperty("Workspace");

    boost::shared_ptr<const Instrument> i =
        output->getInstrument()->baseInstrument();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Y(), 0.0, 0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS(samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA(samplepos->getPos().Z(), 0.0, 0.01);

    const auto &detectorInfo = output->detectorInfo();
    const auto &ptrDet103 = detectorInfo.detector(detectorInfo.indexOf(103));
    TS_ASSERT_EQUALS(ptrDet103.getID(), 103);
    TS_ASSERT_EQUALS(ptrDet103.getName(), "HET_non_PSDtube");
    TS_ASSERT_DELTA(ptrDet103.getPos().X(), 0.3826351418, 0.01);
    TS_ASSERT_DELTA(ptrDet103.getPos().Z(), 2.4470, 0.01);
    double d = ptrDet103.getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 2.512, 0.0001);
    double cmpDistance = ptrDet103.getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 2.512, 0.0001);

    // test if detector with det_id=603 has been marked as a monitor
    TS_ASSERT(
        output->detectorInfo().isMonitor(output->detectorInfo().indexOf(601)));

    // Spectra mapping has been updated
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(0), 1);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(255), 256);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(256), 257);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(257), 258);

    auto ids_from_map = output->getSpectrum(257).getDetectorIDs();
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &det_from_ws = spectrumInfo.detector(257);
    TS_ASSERT_EQUALS(ids_from_map.size(), 1);
    TS_ASSERT_EQUALS(*ids_from_map.begin(), 602);
    TS_ASSERT_EQUALS(det_from_ws.getID(), 602);

    // also a few tests on the last detector and a test for the one beyond the
    // last
    const auto &ptrDetLast =
        detectorInfo.detector(detectorInfo.indexOf(413256));
    TS_ASSERT_EQUALS(ptrDetLast.getID(), 413256);
    TS_ASSERT_EQUALS(ptrDetLast.getName(), "pixel");
    TS_ASSERT_THROWS(detectorInfo.indexOf(413257), const std::out_of_range &);

    // Test input data is unchanged
    Workspace2D_sptr output2DInst =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584
    TS_ASSERT_EQUALS(output2DInst->getNumberHistograms(), histogramNumber);

    // Check running algorithm for same XML file leads to same instrument object
    // being attached
    boost::shared_ptr<Instrument> instr = boost::make_shared<Instrument>();
    output->setInstrument(instr);
    TS_ASSERT_EQUALS(output->getInstrument()->baseInstrument(), instr);
    LoadInstrument loadAgain;
    loadAgain.initialize();
    loadAgain.setPropertyValue("Filename", instrFilename);
    loadAgain.setProperty("Workspace", ws2D);
    loadAgain.setProperty("RewriteSpectraMap", OptionalBool(true));
    loadAgain.execute();
    TS_ASSERT_EQUALS(output->getInstrument()->baseInstrument(), i);

    // Valid-from/to1951-01-01 00:00:01
    Types::Core::DateAndTime validFrom("1951-01-01T00:00:01");
    Types::Core::DateAndTime validTo("2100-01-31 23:59:59");
    TS_ASSERT_EQUALS(i->getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS(i->getValidToDate(), validTo);
  }

  void testExecSLS() {
    LoadInstrument loaderSLS;
    loaderSLS.initialize();
    loaderSLS.setChild(true);

    // create a workspace with some sample data
    MatrixWorkspace_sptr ws2D =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));

    const std::string instrFilename = "SANDALS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", instrFilename);
    loaderSLS.setProperty("RewriteSpectraMap", OptionalBool(true));
    loaderSLS.setProperty("Workspace", ws2D);

    std::string result = loaderSLS.getPropertyValue("Filename");
    const std::string::size_type stripPath = result.find_last_of("\\/");
    result = result.substr(stripPath + 1, result.size());
    TS_ASSERT_EQUALS(result, instrFilename);

    loaderSLS.execute();
    TS_ASSERT(loaderSLS.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output = loaderSLS.getProperty("Workspace");

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Z(), -11.016, 0.01);

    boost::shared_ptr<const IObjComponent> samplepos =
        boost::dynamic_pointer_cast<const IObjComponent>(i->getSample());
    TS_ASSERT_EQUALS(samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA(samplepos->getPos().Y(), 0.0, 0.01);

    const auto &detectorInfo = output->detectorInfo();
    const auto &ptrDet = detectorInfo.detector(detectorInfo.indexOf(101));
    TS_ASSERT_EQUALS(ptrDet.getID(), 101);

    TS_ASSERT(output->detectorInfo().isMonitor(0));

    const auto &ptrDetShape = detectorInfo.detector(detectorInfo.indexOf(102));
    TS_ASSERT(ptrDetShape.isValid(V3D(0.0, 0.0, 0.0) + ptrDetShape.getPos()));
    TS_ASSERT(
        ptrDetShape.isValid(V3D(0.0, 0.0, 0.000001) + ptrDetShape.getPos()));
    TS_ASSERT(
        ptrDetShape.isValid(V3D(0.005, 0.1, 0.000002) + ptrDetShape.getPos()));

    // test of sample shape
    TS_ASSERT(samplepos->isValid(V3D(0.0, 0.0, 0.005) + samplepos->getPos()));
    TS_ASSERT(!samplepos->isValid(V3D(0.0, 0.0, 0.05) + samplepos->getPos()));
  }

  void testExecNIMROD() {
    LoadInstrument loaderNIMROD;
    loaderNIMROD.initialize();
    loaderNIMROD.setChild(true);

    // create a workspace with some sample data
    MatrixWorkspace_sptr ws2D =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));

    const std::string instrFilename = "NIM_Definition.xml";
    loaderNIMROD.setPropertyValue("Filename", instrFilename);
    loaderNIMROD.setProperty("RewriteSpectraMap", OptionalBool(true));
    loaderNIMROD.setProperty("Workspace", ws2D);

    std::string result = loaderNIMROD.getPropertyValue("Filename");
    const std::string::size_type stripPath = result.find_last_of("\\/");
    result = result.substr(stripPath + 1, result.size());
    TS_ASSERT_EQUALS(result, instrFilename);

    loaderNIMROD.execute();
    TS_ASSERT(loaderNIMROD.isExecuted());

    const auto &detectorInfo = ws2D->detectorInfo();
    const auto &ptrDet = detectorInfo.detector(detectorInfo.indexOf(20201001));
    TS_ASSERT_EQUALS(ptrDet.getName(), "det 1");
    TS_ASSERT_EQUALS(ptrDet.getID(), 20201001);
    TS_ASSERT_DELTA(ptrDet.getPos().X(), -0.0909, 0.0001);
    TS_ASSERT_DELTA(ptrDet.getPos().Y(), 0.3983, 0.0001);
    TS_ASSERT_DELTA(ptrDet.getPos().Z(), 4.8888, 0.0001);
  }

  void testExecNIMRODandRetrieveFromIDS() {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl &IDS = InstrumentDataService::Instance();
    IDS.clear();

    LoadInstrument loaderNIMROD;
    loaderNIMROD.initialize();
    loaderNIMROD.setChild(true);

    // create a workspace with some sample data
    MatrixWorkspace_sptr ws2D =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));

    const std::string instrFilename = "NIM_Definition.xml";
    loaderNIMROD.setPropertyValue("Filename", instrFilename);
    loaderNIMROD.setProperty("RewriteSpectraMap", OptionalBool(true));
    loaderNIMROD.setProperty("Workspace", ws2D);
    loaderNIMROD.execute();
    TS_ASSERT(loaderNIMROD.isExecuted());

    TS_ASSERT_EQUALS(IDS.size(), 1);
    if (IDS.size() != 1)
      return;
    // Retrieve the instrument from the InstrumentDataService
    Instrument_const_sptr nimrodInst = IDS.getObjects()[0];
    TS_ASSERT_EQUALS(nimrodInst->getName(), "NIM");
    TS_ASSERT_EQUALS(nimrodInst->getNumberDetectors(), 1521);
    int a_random_id = 20201001;
    TS_ASSERT_EQUALS((nimrodInst->getDetector(a_random_id))->getID(),
                     a_random_id);
    TS_ASSERT_DELTA((nimrodInst->getDetector(a_random_id))->getPos().X(),
                    -0.0909, 0.0001);
    TS_ASSERT_DELTA((nimrodInst->getDetector(a_random_id))->getPos().Y(),
                    0.3983, 0.0001);
    TS_ASSERT_DELTA((nimrodInst->getDetector(a_random_id))->getPos().Z(),
                    4.8888, 0.0001);
  }

  void testExecMARIFromInstrName() {
    LoadInstrument loaderMARI;
    loaderMARI.initialize();
    loaderMARI.setChild(true);

    // create a workspace with some sample data
    MatrixWorkspace_sptr ws2D =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));

    const std::string instrName = "MARI";
    loaderMARI.setPropertyValue("InstrumentName", instrName);
    loaderMARI.setProperty("RewriteSpectraMap", OptionalBool(true));
    loaderMARI.setProperty("Workspace", ws2D);

    loaderMARI.execute();
    TS_ASSERT(loaderMARI.isExecuted());

    std::string result = loaderMARI.getPropertyValue("Filename");
    const std::string::size_type stripPath = result.find_last_of("\\/");
    result = result.substr(stripPath + 1, result.size());
    TS_ASSERT_EQUALS(result, "MARI_Definition.xml");

    auto &componentInfo = ws2D->componentInfo();
    auto &detectorInfo = ws2D->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "MARI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 921);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(4816, detectorInfo.detectorIDs()[920]);

    const auto &ptrDet1 = detectorInfo.detector(detectorInfo.indexOf(1));
    TS_ASSERT_EQUALS(ptrDet1.getName(), "monitor");
    TS_ASSERT_EQUALS(ptrDet1.getID(), 1);
    TS_ASSERT_DELTA(ptrDet1.getPos().X(), 0.0000, 0.0001);
    TS_ASSERT_DELTA(ptrDet1.getPos().Y(), 0.0000, 0.0001);
    TS_ASSERT_DELTA(ptrDet1.getPos().Z(), -3.2500, 0.0001);

    const auto &ptrDet2 = detectorInfo.detector(detectorInfo.indexOf(4816));
    TS_ASSERT_EQUALS(ptrDet2.getName(), "tall He3 element");
    TS_ASSERT_EQUALS(ptrDet2.getID(), 4816);
    TS_ASSERT_DELTA(ptrDet2.getPos().X(), 0.6330, 0.0001);
    TS_ASSERT_DELTA(ptrDet2.getPos().Y(), 0.6330, 0.0001);
    TS_ASSERT_DELTA(ptrDet2.getPos().Z(), 3.9211, 0.0001);
  }

  /// Common initialisation for Nexus loading tests
  MatrixWorkspace_sptr doLoadNexus(const std::string filename) {
    LoadInstrument nexusLoader;
    nexusLoader.initialize();
    nexusLoader.setChild(true);
    // Create a workspace
    // Note that using `auto ws = DataObjects::create<Workspace2D>...` here
    // prevents us from then setting `loaderLOKI.setProperty("Workspace", ws)`
    // because the `unique_ptr` will not allow itself to be copied into the
    // `shared_ptr` as a l-value reference.
    MatrixWorkspace_sptr ws =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));
    nexusLoader.setPropertyValue("Filename", filename);
    nexusLoader.setProperty("Workspace", ws);
    nexusLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    nexusLoader.execute();
    TS_ASSERT(nexusLoader.isExecuted());
    return nexusLoader.getProperty("Workspace");
  }

  /// Test the Nexus geometry loader from LOKI file
  void testExecNexusLOKI() {
    MatrixWorkspace_sptr outputWs = doLoadNexus("LOKI_Definition.hdf5");
    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "LOKI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 8000);
    TS_ASSERT_EQUALS(0, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[1]);
  }

  /// Test the Nexus geometry loader from SANS2D file
  void testExecNexusSANS2D() {
    MatrixWorkspace_sptr outputWs = doLoadNexus("SANS2D_Definition_Tubes.hdf5");
    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "SANS2D");
    TS_ASSERT_EQUALS(detectorInfo.size(), 122888);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(2, detectorInfo.detectorIDs()[1]);
  }

  /// Test the Nexus geometry loader from WISH file
  void testExecNexusWISH() {
    MatrixWorkspace_sptr outputWs =
        doLoadNexus("WISH_Definition_10Panels.hdf5");
    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "WISH");
    TS_ASSERT_EQUALS(detectorInfo.size(), 778245);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(2, detectorInfo.detectorIDs()[1]);
    TS_ASSERT_EQUALS(10707511, detectorInfo.detectorIDs()[778244]);
    TS_ASSERT_THROWS(detectorInfo.indexOf(778245), const std::out_of_range &);
  }

  /// Test the Nexus geometry loader from LOKI name
  void testExecNexusLOKIFromInstrName() {
    LoadInstrument loaderLOKI;
    loaderLOKI.initialize();
    loaderLOKI.setChild(true);
    // Create a workspace
    MatrixWorkspace_sptr ws =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));
    loaderLOKI.setPropertyValue("InstrumentName", "LOKI");
    loaderLOKI.setProperty("Workspace", ws);
    loaderLOKI.setProperty("RewriteSpectraMap", OptionalBool(true));
    loaderLOKI.execute();
    TS_ASSERT(loaderLOKI.isExecuted());

    std::string result = loaderLOKI.getPropertyValue("Filename");
    const std::string::size_type stripPath = result.find_last_of("\\/");
    result = result.substr(stripPath + 1, result.size());
    TS_ASSERT_EQUALS(result, "LOKI_Definition.hdf5");

    auto &componentInfo = ws->componentInfo();
    auto &detectorInfo = ws->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "LOKI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 8000);
    TS_ASSERT_EQUALS(0, detectorInfo.detectorIDs()[0]);
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[1]);
  }

  /// Test the Nexus geometry loader from LOKI file
  void testExecNexusLOKIandRetrieveFromIDS() {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl &IDS = InstrumentDataService::Instance();
    IDS.clear();
    MatrixWorkspace_sptr outputWs = doLoadNexus("LOKI_Definition.hdf5");
    TS_ASSERT_EQUALS(IDS.size(), 1);
    if (IDS.size() != 1)
      return;
    // Retrieve the instrument from the InstrumentDataService
    Instrument_const_sptr lokiInst = IDS.getObjects()[0];
    TS_ASSERT_EQUALS(lokiInst->getName(), "LOKI");
    TS_ASSERT_EQUALS(lokiInst->getNumberDetectors(), 8000);
    TS_ASSERT_EQUALS((lokiInst->getDetector(1001))->getID(), 1001);
    TS_ASSERT_EQUALS((lokiInst->getDetector(7777))->getID(), 7777);
  }

  void testExecHRP2() {
    // Test Parameter file in instrument folder is used by an IDF file not in
    // the instrument folder
    doTestParameterFileSelection("unit_testing/HRPD_Definition.xml",
                                 "HRPD_Parameters.xml", "S");
  }

  void testExecHRP3() {
    // Test Parameter file in instrument folder is used by an IDF file not in
    // the instrument folder and
    // with an extension of its name after the 'Definition' not present in a
    // parameter file.
    doTestParameterFileSelection("unit_testing/HRPD_Definition_Test3.xml",
                                 "HRPD_Parameters.xml", "S");
  }

  void testExecHRP4() {
    // Test Parameter file outside of instrument folder is used by an IDF file
    // in the same folder and
    // with the same extension ('_Test4') of its name after the 'Definition' or
    // 'Parameter'.
    doTestParameterFileSelection("unit_testing/HRPD_Definition_Test4.xml",
                                 "unit_testing/HRPD_Parameters_Test4.xml", "T");
  }

  void testExecHRP5() {
    // Test Parameter file outside instrument folder is used by an IDF file in
    // the same folder
    doTestParameterFileSelection("unit_testing/HRPDTEST_Definition.xml",
                                 "unit_testing/HRPDTEST_Parameters.xml", "U");
  }

  void testExecHRP6() {
    // Test Parameter file outside of instrument folder is used by an IDF file
    // in the same folder and
    // with the same extension ('_Test6') of its name after the 'Definition' or
    // 'Parameter'
    // even though there is a definition file without an extension in the same
    // folder.
    doTestParameterFileSelection("unit_testing/HRPDTEST_Definition_Test6.xml",
                                 "unit_testing/HRPDTEST_Parameters_Test6.xml",
                                 "V");
  }

  void testExecHRP7() {
    // Test Parameter file outside instrument folder is used by an IDF file in
    // same instrument folder and
    // with an extension of its name after the 'Definition' not present in a
    // parameter file.
    doTestParameterFileSelection("unit_testing/HRPDTEST_Definition_Test7.xml",
                                 "HRPDTEST_Parameters.xml", "U");
  }

  void testNeutronicPositions() {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl &IDS = InstrumentDataService::Instance();
    IDS.clear();

    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "unit_testing/INDIRECT_Definition.xml");
    MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    loader.setProperty("Workspace", ws);
    loader.setProperty("RewriteSpectraMap", OptionalBool(true));
    TS_ASSERT(loader.execute());

    // This kind of IDF should lead to 2 instrument definitions - the physical
    // and the neutronic
    // But only 1 goes into the IDS (the neutronic instrument holds the physical
    // instrument within itself)
    TS_ASSERT_EQUALS(IDS.size(), 1);
    if (IDS.size() != 1)
      return;
    TS_ASSERT_EQUALS(IDS.getObjects()[0]->getName(), "INDIRECT");

    // Retrieve the neutronic instrument from the InstrumentDataService
    Instrument_const_sptr neutronicInst = IDS.getObjects()[0];
    // And pull out a handle to the physical instrument from within the
    // neutronic one
    Instrument_const_sptr physicalInst = neutronicInst->getPhysicalInstrument();
    // They should not be the same object
    TS_ASSERT_DIFFERS(physicalInst.get(), neutronicInst.get());
    // Not true in general, but in this case we should not be getting a
    // paramaterized instrument
    TS_ASSERT(!physicalInst->isParametrized());

    // Check the positions of the 6 detectors in the physical instrument
    TS_ASSERT_EQUALS(physicalInst->getDetector(1000)->getPos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(physicalInst->getDetector(1001)->getPos(), V3D(0, 1, 0));
    TS_ASSERT_EQUALS(physicalInst->getDetector(1002)->getPos(), V3D(1, 0, 0));
    TS_ASSERT_EQUALS(physicalInst->getDetector(1003)->getPos(), V3D(1, 1, 0));
    TS_ASSERT_EQUALS(physicalInst->getDetector(1004)->getPos(), V3D(2, 0, 0));
    TS_ASSERT_EQUALS(physicalInst->getDetector(1005)->getPos(), V3D(2, 1, 0));

    // Check the right instrument ended up on the workspace
    TS_ASSERT_EQUALS(neutronicInst.get(),
                     ws->getInstrument()->baseInstrument().get());
    // Check the neutronic positions
    const auto &detectorInfo = ws->detectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.position(detectorInfo.indexOf(1000)),
                     V3D(2, 2, 0));
    TS_ASSERT_EQUALS(detectorInfo.position(detectorInfo.indexOf(1001)),
                     V3D(2, 3, 0));
    TS_ASSERT_EQUALS(detectorInfo.position(detectorInfo.indexOf(1002)),
                     V3D(3, 2, 0));
    TS_ASSERT_EQUALS(detectorInfo.position(detectorInfo.indexOf(1003)),
                     V3D(3, 3, 0));
    // Note that one of the physical pixels doesn't exist in the neutronic
    // space
    TS_ASSERT_THROWS(detectorInfo.indexOf(1004), const std::out_of_range &);
    TS_ASSERT_EQUALS(detectorInfo.position(detectorInfo.indexOf(1005)),
                     V3D(4, 3, 0));

    // Check that the first 2 detectors share the same shape in the physical
    // instrument...
    TS_ASSERT_EQUALS(physicalInst->getDetector(1000)->shape(),
                     physicalInst->getDetector(1001)->shape())
    // ...but not in the neutronic instrument
    TS_ASSERT_DIFFERS(detectorInfo.detector(detectorInfo.indexOf(1000)).shape(),
                      neutronicInst->getDetector(1001)->shape())
    // Also, the same shape is shared between the corresponding '1000'
    // detectors
    TS_ASSERT_EQUALS(physicalInst->getDetector(1000)->shape(),
                     detectorInfo.detector(detectorInfo.indexOf(1000)).shape())

    // Check the monitor is in the same place in each instrument
    TS_ASSERT_EQUALS(physicalInst->getDetector(1)->getPos(),
                     detectorInfo.position(detectorInfo.indexOf(1)));
    // ...but is not the same object
    TS_ASSERT_DIFFERS(physicalInst->getDetector(1).get(),
                      neutronicInst->getDetector(1).get());

    // Physical instrument obtained via workspace: Make sure we do *not* get
    // positions from DetectorInfo.
    auto physInstFromWS = ws->getInstrument()->getPhysicalInstrument();
    TS_ASSERT(physInstFromWS->isParametrized());
    TS_ASSERT_DIFFERS(physInstFromWS->getDetector(1003)->getPos(),
                      detectorInfo.position(detectorInfo.indexOf(1003)));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1000)->getPos(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1001)->getPos(), V3D(0, 1, 0));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1002)->getPos(), V3D(1, 0, 0));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1003)->getPos(), V3D(1, 1, 0));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1004)->getPos(), V3D(2, 0, 0));
    TS_ASSERT_EQUALS(physInstFromWS->getDetector(1005)->getPos(), V3D(2, 1, 0));

    // Clean up
    IDS.clear();
  }

  void test_loading_via_InstrumentXML_property() {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl &IDS = InstrumentDataService::Instance();
    IDS.clear();

    // Minimal XML instrument, inspired by IDF_for_UNIT_TESTING3.xml
    const std::string instrumentXML =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
        "<instrument name=\"xmlInst\" valid-from=\"1900-01-31 23:59:59\" "
        "valid-to=\"2100-01-31 23:59:59\" "
        "last-modified=\"2010-10-06T16:21:30\">"
        "<defaults />"
        "<component type=\"panel\" idlist=\"idlist_for_bank1\">"
        "<location r=\"0\" t=\"0\" rot=\"0\" axis-x=\"0\" axis-y=\"1\" "
        "axis-z=\"0\" name=\"bank1\" xpixels=\"3\" ypixels=\"2\" />"
        "</component>"
        "<type is=\"detector\" name=\"panel\">"
        "<properties/>"
        "<component type=\"pixel\">"
        "<location y=\"1\" x=\"1\"/>"
        "</component>"
        "</type>"
        "<type is=\"detector\" name=\"pixel\">"
        "<cuboid id=\"pixel-shape\" />"
        "<algebra val=\"pixel-shape\"/>"
        "</type>"
        "<idlist idname=\"idlist_for_bank1\">"
        "<id start=\"1005\" end=\"1005\" />"
        "</idlist>"
        "</instrument>";

    LoadInstrument instLoader;
    instLoader.setRethrows(true);
    instLoader.initialize();
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    instLoader.setProperty("Workspace", WorkspaceFactory::Instance().create(
                                            "EventWorkspace", 1, 1, 1));
    instLoader.setProperty("InstrumentXML", instrumentXML);
    instLoader.setProperty(
        "InstrumentName",
        "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size())
  }

  void test_failure_if_InstrumentXML_property_set_but_not_InstrumentName() {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setProperty("Workspace", WorkspaceFactory::Instance().create(
                                            "EventWorkspace", 1, 1, 1));
    instLoader.setProperty("InstrumentXML", "<doesn't matter what>");
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    TS_ASSERT(!instLoader.execute())
  }

  void test_failure_if_InstrumentXML_is_malformed() {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setProperty("Workspace", WorkspaceFactory::Instance().create(
                                            "EventWorkspace", 1, 1, 1));
    instLoader.setProperty("InstrumentXML", "<instrument>");
    instLoader.setProperty("InstrumentName", "Nonsense");
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));

    TS_ASSERT(!instLoader.execute())
  }

  void test_failure_if_Filename_not_set() {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setChild(true);
    MatrixWorkspace_sptr ws =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));
    instLoader.setProperty("Workspace", ws);
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    TS_ASSERT_THROWS_EQUALS(
        instLoader.execute(), Kernel::Exception::FileError & e,
        std::string(e.what()),
        "Either the InstrumentName or Filename property of LoadInstrument "
        "must be specified to load an instrument in \"\"");
    TS_ASSERT(!instLoader.isExecuted());
    TS_ASSERT_EQUALS(instLoader.getPropertyValue("Filename"), "");
  }

  void test_failure_if_Filename_not_found() {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setRethrows(true);
    MatrixWorkspace_sptr ws =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));
    instLoader.setProperty("Workspace", ws);
    instLoader.setPropertyValue("Filename", "Nonsense");
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    TS_ASSERT_THROWS_EQUALS(
        instLoader.execute(), std::invalid_argument & e, std::string(e.what()),
        "FileDescriptor() - File 'Nonsense' does not exist");
    TS_ASSERT(!instLoader.isExecuted());
    TS_ASSERT_EQUALS(instLoader.getPropertyValue("Filename"), "Nonsense");
  }

  void test_if_Workspace_not_set() {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setPropertyValue("Filename",
                                "unit_testing/SMALLFAKE_example_geometry.hdf5");
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    instLoader.execute();
    TS_ASSERT(instLoader.isExecuted());
    TS_ASSERT_EQUALS(instLoader.getPropertyValue("Workspace"), "Anonymous");
  }

  void test_loading_default_view() {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl &IDS = InstrumentDataService::Instance();
    IDS.clear();

    // Minimal XML instrument
    const std::string instrumentXML =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
        "<instrument name=\"xmlInst\" valid-from=\"1900-01-31 23:59:59\" "
        "valid-to=\"2100-01-31 23:59:59\" "
        "last-modified=\"2010-10-06T16:21:30\">"
        "<defaults>"
        "<!-- view -->"
        "</defaults>"

        "<component type=\"panel\" idlist=\"idlist_for_bank1\">"
        "<location r=\"0\" t=\"0\" rot=\"0\" axis-x=\"0\" axis-y=\"1\" "
        "axis-z=\"0\" name=\"bank1\" xpixels=\"3\" ypixels=\"2\" />"
        "</component>"
        "<type is=\"detector\" name=\"panel\">"
        "<properties/>"
        "<component type=\"pixel\">"
        "<location y=\"1\" x=\"1\"/>"
        "</component>"
        "</type>"
        "<type is=\"detector\" name=\"pixel\">"
        "<cuboid id=\"pixel-shape\" />"
        "<algebra val=\"pixel-shape\"/>"
        "</type>"
        "<idlist idname=\"idlist_for_bank1\">"
        "<id start=\"1005\" end=\"1005\" />"
        "</idlist>"
        "</instrument>";

    LoadInstrument instLoader;
    instLoader.setRethrows(true);
    instLoader.initialize();
    instLoader.setProperty("Workspace", WorkspaceFactory::Instance().create(
                                            "EventWorkspace", 1, 1, 1));
    instLoader.setProperty("InstrumentXML", instrumentXML);
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    instLoader.setProperty(
        "InstrumentName",
        "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size());
    // test that the default default view is "3D"
    auto instr = IDS.getObjects().front();
    TS_ASSERT_EQUALS(instr->getDefaultView(), "3D");
    IDS.clear();

    // explicitely set the default instrument view
    const std::string instrumentXMLwithView = Mantid::Kernel::Strings::replace(
        instrumentXML, "<!-- view -->",
        "<default-view view=\"cylindrical_y\"/>");

    instLoader.setProperty("Workspace", WorkspaceFactory::Instance().create(
                                            "EventWorkspace", 1, 1, 1));
    instLoader.setProperty("InstrumentXML", instrumentXMLwithView);
    instLoader.setProperty("RewriteSpectraMap", OptionalBool(true));
    instLoader.setProperty(
        "InstrumentName",
        "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size());
    // test that the default view is cylindrical_y
    instr = IDS.getObjects().front();
    TS_ASSERT_EQUALS(instr->getDefaultView(), "CYLINDRICAL_Y");
    IDS.clear();
  }

private:
  // @param filename Filename to an IDF
  // @param paramFilename Expected parameter file to be loaded as part of
  // LoadInstrument
  // @param par A specific parameter to check if have been loaded
  void doTestParameterFileSelection(std::string filename,
                                    std::string paramFilename,
                                    std::string par) {
    InstrumentDataService::Instance().clear();

    LoadInstrument loader;
    loader.initialize();
    loader.setChild(true);

    // create a workspace with some sample data
    MatrixWorkspace_sptr ws2D =
        DataObjects::create<Workspace2D>(1, HistogramData::Points(1));

    // load IDF
    loader.setPropertyValue("Filename", filename);
    loader.setProperty("RewriteSpectraMap", OptionalBool(true));
    loader.setProperty("Workspace", ws2D);
    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output = loader.getProperty("Workspace");

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    // test if a dummy parameter has been read in
    boost::shared_ptr<const IComponent> comp =
        i->getComponentByName("bank_90degnew");
    TS_ASSERT_EQUALS(comp->getName(), "bank_90degnew");

    const auto &paramMap = output->constInstrumentParameters();

    // It's "X0" in parameter file
    // unit_testing/HRPD_Parameters_Test4.xml
    Parameter_sptr param = paramMap.getRecursive(&(*comp), par, "fitting");
    TS_ASSERT(param);
    if (param != nullptr) {
      const FitParameter &fitParam4 = param->value<FitParameter>();
      TS_ASSERT(fitParam4.getTie().compare("") == 0);
      TS_ASSERT(fitParam4.getFunction().compare("BackToBackExponential") == 0);
    } else {
      TS_FAIL("Did not select " + paramFilename + " for " + filename);
    }
  }
};

class LoadInstrumentTestPerformance : public CxxTest::TestSuite {
public:
  MatrixWorkspace_sptr ws;

  void setUp() override {
    ws = WorkspaceCreationHelper::create2DWorkspace(1, 2);
  }

  void doTest(std::string filename, size_t numTimes = 1) {
    for (size_t i = 0; i < numTimes; ++i) {
      // Remove any existing instruments, so each time they are loaded.
      InstrumentDataService::Instance().clear();
      // Load it fresh
      LoadInstrument loader;
      loader.initialize();
      loader.setProperty("RewriteSpectraMap", OptionalBool(true));
      loader.setProperty("Workspace", ws);
      loader.setPropertyValue("Filename", filename);
      loader.execute();
      TS_ASSERT(loader.isExecuted());
    }
  }

  void test_GEM() { doTest("GEM_Definition.xml", 10); }

  void test_WISH() { doTest("WISH_Definition.xml", 1); }

  void test_BASIS() { doTest("BASIS_Definition_0-2013.xml", 5); }

  void test_CNCS() { doTest("CNCS_Definition.xml", 5); }

  void test_SEQUOIA() { doTest("SEQUOIA_Definition.xml", 5); }

  void test_POWGEN_2011() { doTest("POWGEN_Definition_2011-02-25.xml", 10); }

  void test_TOPAZ_2010() { doTest("TOPAZ_Definition_2010.xml", 1); }

  void test_TOPAZ_2011() { doTest("TOPAZ_Definition_2011-01-01.xml", 1); }

  void test_SNAP() { doTest("SNAP_Definition.xml", 1); }
};

#endif /*LOADINSTRUMENTTEST_H_*/
