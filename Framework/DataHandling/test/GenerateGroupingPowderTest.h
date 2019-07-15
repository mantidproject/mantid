// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_
#define MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
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
#include <fstream>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class GenerateGroupingPowderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateGroupingPowderTest *createSuite() {
    return new GenerateGroupingPowderTest();
  }
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

  void test_Init() {
    GenerateGroupingPowder alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    GenerateGroupingPowder alg;
    alg.setRethrows(true);
    const std::string xmlFile("PowderGrouping.xml");
    constexpr double step = 10;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", m_emptyInstrument));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFilename", xmlFile));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const std::string parFile{parFilename(xmlFile)};

    // Check the results
    // par file
    std::ifstream pf(parFile);
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
      TS_ASSERT_EQUALS(phi, 0);
      TS_ASSERT_DELTA(dx, r * step * Geometry::deg2rad, 0.01);
      TS_ASSERT_EQUALS(dy, 0.01);
      tth = detectorInfo.twoTheta(detectorInfo.indexOf(detID)) *
            Geometry::rad2deg;
      TS_ASSERT_LESS_THAN(tth, static_cast<double>(i + 1) * step);
      TS_ASSERT_LESS_THAN(static_cast<double>(i) * step, tth);
    }
    pf.close();

    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", xmlFile));
    TS_ASSERT(load2.setProperty("OutputWorkspace", "GroupPowder"));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws2 =
        boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
            API::AnalysisDataService::Instance().retrieve("GroupPowder"));

    TS_ASSERT_DELTA(gws2->dataY(0)[0], 13.0, 1.0E-5);    // 130.6 degrees
    TS_ASSERT_DELTA(gws2->dataY(10000)[0], 9.0, 1.0E-5); // 97.4 degrees
    TS_ASSERT_DELTA(gws2->dataY(20000)[0], 6.0, 1.0E-5); // 62.9 degrees
    TS_ASSERT_DELTA(gws2->dataY(30000)[0], 2.0, 1.0E-5); // 27.8 degrees
    TS_ASSERT_DELTA(gws2->dataY(40000)[0], 1.0, 1.0E-5); // 14.5 degrees
    TS_ASSERT_DELTA(gws2->dataY(50000)[0], 4.0, 1.0E-5); // 49.7 degrees

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("GroupPowder");
    // Delete files
    remove(xmlFile.c_str());
    remove(parFile.c_str());
  }

  void test_turning_off_par_file_generation() {
    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);
    const std::string xmlFilename("PowderGrouping.xml");
    constexpr double step = 10;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", m_emptyInstrument))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("GroupingFilename", xmlFilename))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    Poco::File parFile{parFilename(xmlFilename)};
    Poco::File xmlFile{xmlFilename};
    const bool xmlExists = xmlFile.exists();
    TS_ASSERT(xmlExists)
    const bool parExists = parFile.exists();
    TS_ASSERT(!parExists)
    if (xmlExists) {
      xmlFile.remove();
    }
    if (parExists) {
      // Just in case something went wrong.
      parFile.remove();
    }
  }

  void test_ignore_detectors_without_spectra() {
    auto histogram = m_emptyInstrument->histogram(0);
    MatrixWorkspace_sptr ws =
        create<Workspace2D>(*m_emptyInstrument, 1, histogram);
    ws->getSpectrum(0).copyInfoFrom(m_emptyInstrument->getSpectrum(8));
    GenerateGroupingPowder alg;
    alg.setChild(true);
    alg.setRethrows(true);
    const std::string xmlFilename("PowderGrouping.xml");
    constexpr double step = 10;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("GroupingFilename", xmlFilename))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", step))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateParFile", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    Poco::File xmlFile{xmlFilename};
    const bool xmlExists = xmlFile.exists();
    TS_ASSERT(xmlExists)
    std::ifstream in(xmlFilename);
    Poco::XML::InputSource source{in};
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> doc{parser.parse(&source)};
    ShowDetIDsOnly filter;
    Poco::XML::NodeIterator nodeIter{doc, Poco::XML::NodeFilter::SHOW_ELEMENT,
                                     &filter};
    auto node = nodeIter.nextNode();
    TS_ASSERT(node)
    TS_ASSERT_EQUALS(node->nodeName(), "detids")
    TS_ASSERT_EQUALS(node->innerText(), "4")
    node = nodeIter.nextNode();
    in.close();
    TS_ASSERT(!node)
    if (xmlExists) {
      xmlFile.remove();
    }
    // Just in case something went wrong.
    Poco::File parFile{parFilename(xmlFilename)};
    if (parFile.exists()) {
      parFile.remove();
    }
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
  static std::string parFilename(std::string filename) {
    filename.replace(filename.end() - 3, filename.end(), "par");
    return filename;
  }
};

#endif /* MANTID_DATAHANDLING_GENERATEGROUPINGPOWDERTEST_H_ */
