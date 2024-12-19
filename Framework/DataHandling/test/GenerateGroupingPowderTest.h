// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Timer.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/File.h>
#include <Poco/SAX/InputSource.h>
#include <cxxtest/TestSuite.h>
#include <exception>
#include <fstream>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class GenerateGroupingPowderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateGroupingPowderTest *createSuite() { return new GenerateGroupingPowderTest(); }
  static void destroySuite(GenerateGroupingPowderTest *suite) { delete suite; }

  GenerateGroupingPowderTest() : CxxTest::TestSuite() {
    LoadEmptyInstrument lei;
    lei.initialize();
    lei.setChild(true);
    lei.setRethrows(true);
    lei.setPropertyValue("Filename", "CNCS_Definition.xml");
    lei.setPropertyValue("OutputWorkspace", "unused_for_child");
    lei.execute();
    m_emptyInstrument = lei.getProperty("OutputWorkspace");
  }

  void test_init() {
    GenerateGroupingPowder alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    const std::string XML_OUT_FILE("PowderGrouping_fulltest.xml");
    const std::string PAR_OUT_FILE{GenerateGroupingPowder::parFilenameFromXmlFilename(XML_OUT_FILE)};
    const std::string GROUP_WS("plainExecTestWS");
    constexpr double step = 10;

    GenerateGroupingPowder alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_emptyInstrument));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileFormat", "xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", XML_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check the par file results
    if (!fileExists(PAR_OUT_FILE)) {
      throw std::runtime_error("par file was not created: " + PAR_OUT_FILE);
    }
    std::ifstream pf(PAR_OUT_FILE);
    std::size_t nDet;
    pf >> nDet;
    TS_ASSERT_EQUALS(nDet, 14);
    const auto &detectorInfo = m_emptyInstrument->detectorInfo();
    for (std::size_t i = 0; i < nDet; ++i) {
      double r, th, phi, dx, dy, tth;
      detid_t detID;
      pf >> r >> th >> phi >> dx >> dy >> detID;
      TS_ASSERT_DELTA(r, 3.5, 0.2);
      TS_ASSERT_DELTA(th, step * (static_cast<double>(i) + 0.5), 0.5 * step);
      TS_ASSERT_EQUALS(phi, 0.00);
      TS_ASSERT_DELTA(dx, r * step * Geometry::deg2rad, 0.01);
      TS_ASSERT_EQUALS(dy, 0.01);
      tth = detectorInfo.twoTheta(detectorInfo.indexOf(detID)) * Geometry::rad2deg;
      TS_ASSERT_LESS_THAN(tth, static_cast<double>(i + 1) * step);
      TS_ASSERT_LESS_THAN(static_cast<double>(i) * step, tth);
    }
    pf.close();

    // check the xml grouping file
    if (!fileExists(XML_OUT_FILE))
      throw std::runtime_error("xml file was not created: " + XML_OUT_FILE);
    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", XML_OUT_FILE));
    TS_ASSERT(load2.setProperty("OutputWorkspace", "GroupPowder"));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws2 = std::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
        API::AnalysisDataService::Instance().retrieve("GroupPowder"));

    TS_ASSERT_DELTA(gws2->dataY(0)[0], 13.0, 1.0E-5);    // 130.6 degrees
    TS_ASSERT_DELTA(gws2->dataY(10000)[0], 9.0, 1.0E-5); // 97.4 degrees
    TS_ASSERT_DELTA(gws2->dataY(20000)[0], 6.0, 1.0E-5); // 62.9 degrees
    TS_ASSERT_DELTA(gws2->dataY(30000)[0], 2.0, 1.0E-5); // 27.8 degrees
    TS_ASSERT_DELTA(gws2->dataY(40000)[0], 1.0, 1.0E-5); // 14.5 degrees
    TS_ASSERT_DELTA(gws2->dataY(50000)[0], 4.0, 1.0E-5); // 49.7 degrees

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(GROUP_WS);
    // Delete files
    remove(XML_OUT_FILE.c_str());
    remove(PAR_OUT_FILE.c_str());
  }

  void test_turning_off_par_file_generation() {
    const std::string XML_OUT_FILE("PowderGrouping_nopar.xml");
    const std::string GROUP_WS("noParFileWS");
    constexpr double step = 10;

    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_emptyInstrument));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", XML_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // make sure the xml exists and cleanup
    if (fileExists(XML_OUT_FILE)) {
      remove(XML_OUT_FILE.c_str());
    } else {
      TS_FAIL("xml file " + XML_OUT_FILE + " was not created");
    }
    // make sure the parfile does not exist and cleanup
    const std::string parFilename(GenerateGroupingPowder::parFilenameFromXmlFilename(XML_OUT_FILE));
    if (fileExists(parFilename)) {
      remove(parFilename.c_str());
      TS_FAIL("par file " + parFilename + "exists and shouldn't");
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(GROUP_WS);
  }

  void test_ignore_detectors_without_spectra() {
    const std::string XML_OUT_FILE("PowderGrouping_det_wo_spectra.xml");
    const std::string GROUP_WS("noDetNoSpecWS");

    auto histogram = m_emptyInstrument->histogram(0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(*m_emptyInstrument, 1, histogram);
    ws->getSpectrum(0).copyInfoFrom(m_emptyInstrument->getSpectrum(8));
    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);
    constexpr double step = 10;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", XML_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!fileExists(XML_OUT_FILE))
      throw std::runtime_error("xml file was not created: " + XML_OUT_FILE);
    std::ifstream in(XML_OUT_FILE);
    Poco::XML::InputSource source{in};
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> doc{parser.parse(&source)};
    ShowDetIDsOnly filter;
    Poco::XML::NodeIterator nodeIter{doc, Poco::XML::NodeFilter::SHOW_ELEMENT, &filter};
    auto node = nodeIter.nextNode();
    TS_ASSERT(node)
    TS_ASSERT_EQUALS(node->nodeName(), "detids");
    TS_ASSERT_EQUALS(node->innerText(), "4");
    node = nodeIter.nextNode();
    in.close();
    TS_ASSERT(!node);

    remove(XML_OUT_FILE.c_str());

    // Just in case something went wrong.
    std::string parFilename = GenerateGroupingPowder::parFilenameFromXmlFilename(XML_OUT_FILE);
    if (fileExists(parFilename)) {
      remove(parFilename.c_str());
      TS_FAIL("par file " + parFilename + "exists and shouldn't");
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(GROUP_WS);
  }

  // save as nexus, reload, and compare
  void test_save_nexus_processed() {
    const std::string NXS_OUT_FILE("PowderGrouping.nxs");
    const std::string GROUP_WS("saveNXSWS");
    constexpr double step = 10;

    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_emptyInstrument));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileFormat", "nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", NXS_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    GroupingWorkspace_sptr gws = alg.getProperty("GroupingWorkspace");
    TS_ASSERT(gws);

    if (!fileExists(NXS_OUT_FILE)) {
      throw std::runtime_error("nexus file was not created: " + NXS_OUT_FILE);
    }

    LoadNexusProcessed load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT(load.isInitialized());
    load.setPropertyValue("Filename", NXS_OUT_FILE);
    load.setPropertyValue("OutputWorkspace", GROUP_WS);

    TS_ASSERT_THROWS_NOTHING(load.execute());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(GROUP_WS));
    TS_ASSERT(workspace);
    TS_ASSERT(workspace.get());

    MatrixWorkspace_sptr ows = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT(ows);
    if (!ows)
      return;

    auto alg2 = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    alg2->initialize();
    alg2->setProperty<MatrixWorkspace_sptr>("Workspace1", gws);
    alg2->setProperty<MatrixWorkspace_sptr>("Workspace2", ows);
    alg2->setProperty<double>("Tolerance", 0.0);
    alg2->setProperty<bool>("CheckAxes", false);
    alg2->execute();
    if (alg2->isExecuted()) {
      TS_ASSERT(alg2->getProperty("Result"));
    } else {
      TS_ASSERT(false);
    }

    // remove the output file
    remove(NXS_OUT_FILE.c_str());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(GROUP_WS);
  }

  // azimuthal grouping with a par file isn't currently supported
  void test_azimuth_with_par_fail() {
    const std::string XML_OUT_FILE("PowderGrouping_azi_with_par.xml");
    const std::string PAR_OUT_FILE{GenerateGroupingPowder::parFilenameFromXmlFilename(XML_OUT_FILE)};
    const std::string GROUP_WS("aziWithParTestWS");
    constexpr double step = 10;

    GenerateGroupingPowder alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_emptyInstrument));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileFormat", "xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", XML_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    // these next two cannot coexist
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AzimuthalStep", 180.));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", true));
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &); // TODO type may be something else
    TS_ASSERT(!alg.isExecuted());

    // cleanup in case things were created
    if (fileExists(XML_OUT_FILE)) {
      remove(XML_OUT_FILE.c_str());
      TS_FAIL("The file " + XML_OUT_FILE + " should not exist");
    }
    // this will succeed if workspace doesn't exist
    AnalysisDataService::Instance().remove(GROUP_WS);
  }

  void test_grouping_rectangular_instrument() {
    const std::string XML_OUT_FILE("PowderGrouping_rectangular.xml");
    const std::string GROUP_WS("_unused_for_child");
    constexpr int numBanks{1};
    constexpr int bankSize{6};
    constexpr int numBins{13};
    constexpr double angleStep{0.1};
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(numBanks, bankSize, numBins);

    inputWS->getAxis(0)->setUnit("TOF");
    inputWS->mutableRun().addProperty("wavelength", 1.0);
    auto &paramMap = inputWS->instrumentParameters();
    paramMap.addString(inputWS->getInstrument().get(), "l2", std::to_string(5.));

    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", GROUP_WS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileFormat", "xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", XML_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", angleStep));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    GroupingWorkspace_sptr outputWS = alg.getProperty("GroupingWorkspace");
    TS_ASSERT(outputWS);
    auto const &spectrumInfo = outputWS->spectrumInfo();
    auto const nHist = spectrumInfo.size();
    TS_ASSERT_EQUALS(nHist, numBanks * bankSize * bankSize);
    double angleStepRad = angleStep * Mantid::Geometry::deg2rad;
    // ensure each pixel angle is inside the range correspondng to its group ID
    for (size_t i = 0; i < nHist; ++i) {
      auto const twoTheta = spectrumInfo.twoTheta(i);
      auto const groupID = outputWS->dataY(i)[0];
      TS_ASSERT_LESS_THAN_EQUALS(angleStepRad * (groupID - 1), twoTheta);
      TS_ASSERT_LESS_THAN(twoTheta, angleStepRad * (groupID));
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(GROUP_WS);
    // Delete files
    remove(XML_OUT_FILE.c_str());
  }

  /*
   The angular range of the in-plane component of an in-plane detector is rougly 15 degrees. The angular step size is
   chosen to be slightly larger than that to reduce the overall number of groups that will be generated. The
   out-of-plane detector bank centers are roughly at the same two-theta angle as the in-plane, but your mileage may
   vary.
   */
  void run_SNAPliteTest(const std::string &groupWSName, const double ang1, const double ang2, const bool numberByAngle,
                        const bool splitSides, const std::vector<int> &groups_exp,
                        const std::vector<double> &pixel_groups_exp) {
    const std::string NXS_OUT_FILE("PowderGrouping.nxs");
    constexpr double TWO_THETA_STEP{18};
    const std::string INPUT_WS("SNAPlite");
    const detid_t PIXELS_PER_BANK{32 * 32};

    // create snap lite with default detector positions
    WorkspaceCreationHelper::createSNAPLiteInstrument(INPUT_WS, ang1, ang2);

    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", INPUT_WS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileFormat", "nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", NXS_OUT_FILE));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", TWO_THETA_STEP));
    if (splitSides) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("AzimuthalStep", 180.));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("AzimuthalStart", -90.));
    }
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberByAngle", numberByAngle));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // make sure the output file exists
    if (fileExists(NXS_OUT_FILE)) {
      remove(NXS_OUT_FILE.c_str());
    } else {
      TS_FAIL("Output file does not exist: " + NXS_OUT_FILE);
    }

    // get the GroupingWorkspace
    GroupingWorkspace_const_sptr outputws = alg.getProperty("GroupingWorkspace");
    if (!outputws) {
      throw std::runtime_error("Cannot get grouping workspace: " + groupWSName);
    }

    // verify the groups that were created
    const auto groups_obs = outputws->getGroupIDs();
    TS_ASSERT_EQUALS(groups_obs.size(), groups_exp.size());
    TS_ASSERT_EQUALS(groups_obs, groups_exp);

    // verify the group assigned to particular pixels in the center of each of the 18 banks
    const double TOL{0.01}; // getValue returns a double
    for (std::size_t i = 0; i < pixel_groups_exp.size(); ++i) {
      const detid_t detID = static_cast<detid_t>((0.5 + double(i)) * PIXELS_PER_BANK);
      TS_ASSERT_DELTA(outputws->getValue(detID), pixel_groups_exp[i], TOL);
    }

    // this will succeed if workspace doesn't exist
    AnalysisDataService::Instance().remove(INPUT_WS);
    AnalysisDataService::Instance().remove(groupWSName);
  }

  void test_SNAPlite_angle_numbering_at_zero() {
    const double ang1 = 0.0;
    const double ang2 = 0.0;
    const bool numberByAngle{true};
    const bool splitSides{false};
    const std::vector<int> groups_exp{1, 2};
    const std::vector<double> pixel_groups_exp{2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2};

    run_SNAPliteTest("PowderGrouping_no_azimuth_angle_numbering", ang1, ang2, numberByAngle, splitSides, groups_exp,
                     pixel_groups_exp);
  }

  // make sure that the "old" behavior of numbering based on angle is still in place
  void test_SNAPlite_no_azimuth_angle_numbering() {
    // angles taken from SNAP_57514
    /*
     bank 12 approx 114-128 - center is group 6
     bank 22 approx 97-113 - center is group 5
     bank 32 approx 81-96 - center is group 4 or 5
     bank 42 approx 41-55 - center is group 2
     bank 52 approx 57-73 - center is group 3
     bank 62 approx 75-89 - center is group 4
     some pixels will make it into group 7
     ** Must relabel the groups +1 to accound for change
     */
    const double ang1 = -65.3;
    const double ang2 = 104.95;
    const bool numberByAngle{true};
    const bool splitSides{false};
    const std::vector<int> groups_exp{3, 4, 5, 6, 7, 8}; // empty part of the instrument gets number 1,2
    const std::vector<double> pixel_groups_exp{7, 7, 7, 6, 6, 6, 5, 5, 5, 3, 3, 3, 4, 4, 4, 5, 5, 5};
    run_SNAPliteTest("PowderGrouping_no_azimuth_angle_numbering", ang1, ang2, numberByAngle, splitSides, groups_exp,
                     pixel_groups_exp);
  }

  // test labeling by angle sectors, divided over entire sphere
  void test_SNAPlite_split_sides_angle_numbering() {
    const double ang1 = -90.0;
    const double ang2 = 90.0;
    const bool numberByAngle{true};
    const bool splitSides{true};
    const std::vector<int> groups_exp{7, 8, 9, 10, 11, 12, 13, 14};
    const std::vector<double> pixel_groups_exp{11, 11, 11, 9, 9, 9, 9, 9, 9, 10, 10, 10, 12, 12, 12, 12, 12, 12};
    run_SNAPliteTest("PowderGrouping_no_azimuth_angle_numbering", ang1, ang2, numberByAngle, splitSides, groups_exp,
                     pixel_groups_exp);
  }

  // detectors centered at -65 and 105
  void test_SNAPlite_no_azimuth() {
    // angles taken from SNAP_57514
    /*
     bank 12 approx 114-128 - center is group 1
     bank 22 approx 97-113 - center is group 2
     bank 32 approx 81-96 - center is group 3
     bank 42 approx 41-55 - center is group 5
     bank 52 approx 57-73 - center is group 4
     bank 62 approx 75-89 - center is group 3
     */
    const double ang1 = -65.3;
    const double ang2 = 104.95;
    const bool numberByAngle{false};
    const bool splitSides{false};
    const std::vector<int> groups_exp{1, 2, 3, 4, 5};
    const std::vector<double> pixel_groups_exp{1, 1, 1, 2, 2, 2, 3, 3, 3, 5, 5, 5, 4, 4, 4, 3, 3, 3};
    run_SNAPliteTest("PowderGrouping_no_azimuth", ang1, ang2, numberByAngle, splitSides, groups_exp, pixel_groups_exp);
  }

  // detectors centered at -30 and 140 - strange numbers but no overlap
  void test_SNAPlite_no_azimuth_gaps() {
    // angles invented so there is space in the middle of the range that is not covered
    /*
     bank 12 - center is group 1
     bank 22 - center is group 2
     bank 32 - center is group 3
     bank 42 - center is group 5 or 6
     bank 52 - center is group 5
     bank 62 -  center is group 4
     */
    const double ang1 = -30;
    const double ang2 = 140;
    const bool numberByAngle{false};
    const bool splitSides{false};
    const std::vector<int> groups_exp{1, 2, 3, 4, 5, 6};
    const std::vector<double> pixel_groups_exp{1, 1, 1, 2, 2, 2, 3, 3, 3, 5, 6, 6, 5, 5, 5, 4, 4, 4};
    run_SNAPliteTest("PowderGrouping_no_azimuth_gaps", ang1, ang2, numberByAngle, splitSides, groups_exp,
                     pixel_groups_exp);
  }

  // detectors centered at -65 and 105
  void test_SNAPlite_with_azimuth() {
    // angles taken from SNAP_57514
    /*
     bank 12 approx 114-128 - center is group 1
     bank 22 approx 97-113 - center is group 2
     bank 32 approx 81-96 - center is group 3
     bank 42 approx 41-55 - center is group 6
     bank 52 approx 57-73 - center is group 5
     bank 62 approx 75-89 - center is group 4 (would overlap 3, but opposite side)
     */
    const double ang1 = -65.3;
    const double ang2 = 104.95;
    const bool numberByAngle{false};
    const bool splitSides{true};
    const std::vector<int> groups_exp{1, 2, 3, 4, 5, 6};
    const std::vector<double> pixel_groups_exp{1, 1, 1, 2, 2, 2, 3, 3, 3, 6, 6, 6, 5, 5, 5, 4, 4, 4};
    run_SNAPliteTest("PowderGrouping_no_azimuth", ang1, ang2, numberByAngle, splitSides, groups_exp, pixel_groups_exp);
  }

private:
  struct ShowDetIDsOnly : Poco::XML::NodeFilter {
    short acceptNode(Poco::XML::Node *node) override {
      if (node->nodeName() == "detids") {
        return FILTER_ACCEPT;
      } else {
        return FILTER_SKIP;
      }
    }
  };

  MatrixWorkspace_sptr m_emptyInstrument;

  bool fileExists(const std::string &filename) {
    Poco::File handle{filename};
    return handle.exists();
  }
};
