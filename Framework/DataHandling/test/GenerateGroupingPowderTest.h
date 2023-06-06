// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/System.h"
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
    const std::string PAR_OUT_FILE{parFilenameFromXmlFilename(XML_OUT_FILE)};
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
    const std::string parFilename(parFilenameFromXmlFilename(XML_OUT_FILE));
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
    std::string parFilename = parFilenameFromXmlFilename(XML_OUT_FILE);
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
    load.setPropertyValue("OutputWorkspace", "GroupPowder");

    TS_ASSERT_THROWS_NOTHING(load.execute());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve("GroupPowder"));
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

  static std::string parFilenameFromXmlFilename(const std::string &filename) {
    std::string result(filename);
    size_t back = 0;
    for (auto i = result.rbegin(); *i != '.' && i != result.rend(); i++)
      back++;
    result.replace(result.end() - back, result.end(), "par");
    return result;
  }

  bool fileExists(const std::string &filename) {
    Poco::File handle{filename};
    return handle.exists();
  }
};
