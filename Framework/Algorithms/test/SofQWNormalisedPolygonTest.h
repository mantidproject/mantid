// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "SofQWTest.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace InstrumentCreationHelper;
using namespace WorkspaceCreationHelper;

class SofQWNormalisedPolygonTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Aliased_To_SofQW3() {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_EQUALS("SofQW3", alg.alias())
  }

  void test_exec() {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQWNormalisedPolygon>();

    TS_ASSERT_EQUALS(result->getAxis(0)->length(), 1904);
    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_DELTA((*(result->getAxis(0)))(0), -0.5590, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(999), -0.0971, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(1900), 0.5728, 0.0001);

    TS_ASSERT_EQUALS(result->getAxis(1)->length(), 7);
    TS_ASSERT_EQUALS(result->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(0), 0.5);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(3), 1.25);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(6), 2.0);

    constexpr double delta(1e-08);
    TS_ASSERT_DELTA(result->y(0)[1160], 22.8567683273, delta);
    TS_ASSERT_DELTA(result->e(0)[1160], 0.3135249168, delta);

    TS_ASSERT_DELTA(result->y(1)[1145], 8.0538512599, delta);
    TS_ASSERT_DELTA(result->e(1)[1145], 0.1923409971, delta);

    TS_ASSERT_DELTA(result->y(2)[1200], 2.0998983601, delta);
    TS_ASSERT_DELTA(result->e(2)[1200], 0.0926743353, delta);

    TS_ASSERT_DELTA(result->y(3)[99], 0.0417524389, delta);
    TS_ASSERT_DELTA(result->e(3)[99], 0.0234250637, delta);

    TS_ASSERT_DELTA(result->y(4)[1654], 0.0172245635, delta);
    TS_ASSERT_DELTA(result->e(4)[1654], 0.0057608185, delta);

    TS_ASSERT_DELTA(result->y(5)[1025], 0.0808168496, delta);
    TS_ASSERT_DELTA(result->e(5)[1025], 0.0208023523, delta);

    // Spectra-detector mapping
    constexpr size_t nspectra(6);
    using IDSet = std::set<int>;
    const std::vector<IDSet> expectedIDs{{IDSet{}, IDSet{13}, IDSet{23}, IDSet{23, 33}, IDSet{33, 43}, IDSet{43}}};

    for (size_t i = 0; i < nspectra; ++i) {
      const auto &spectrum = result->getSpectrum(i);
      Mantid::specnum_t specNoActual = spectrum.getSpectrumNo();
      Mantid::specnum_t specNoExpected = static_cast<Mantid::specnum_t>(i + 1);
      TS_ASSERT_EQUALS(specNoExpected, specNoActual);
      TS_ASSERT_EQUALS(expectedIDs[i], spectrum.getDetectorIDs());
    }
  }

  void testCylindricalDetectors() {
    constexpr int nhist{2};
    constexpr int nbins{10};
    constexpr bool includeMonitors{false};
    constexpr bool startYNegative{true};
    auto input = create2DWorkspaceWithFullInstrument(nhist, nbins, includeMonitors, startYNegative);
    auto &componentInfo = input->mutableComponentInfo();
    for (size_t i = 0; i < nhist; ++i) {
      const std::string name = "pixel-" + std::to_string(i) + ")";
      const auto index = componentInfo.indexOfAny(name);
      auto position = componentInfo.position(index);
      position.setY(position.Y() + 0.33);
      componentInfo.setPosition(index, position);
    }
    input->getAxis(0)->setUnit("DeltaE");
    input->mutableRun().addProperty("Ei", 23.);
    SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", input))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "unused_for_child"))
    const std::vector<double> qParams{0., 0.1, 1.};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qParams))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Direct"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 10)
    TS_ASSERT_EQUALS(output->blocksize(), nbins)
    bool atLeastOneBinChecked{false};
    for (size_t i = 0; i < 3; ++i) {
      const auto &Ys = output->y(i);
      for (size_t j = 0; j < nbins; ++j) {
        const auto y = Ys[j];
        if (std::isfinite(y)) {
          TS_ASSERT_DELTA(y, 2., 1e-10);
          atLeastOneBinChecked = true;
        }
      }
    }
    TS_ASSERT(atLeastOneBinChecked)
  }

  void testCuboidDetectors() {
    constexpr int nhist{2};
    constexpr int nbins{10};
    const BinEdges inX(nbins + 1, LinearGenerator(-5, 1.23));
    const Counts inY(nbins, 2.0);
    const Histogram inHistogram(inX, inY);
    MatrixWorkspace_sptr input = create<Workspace2D>(nhist, inHistogram);
    auto instrument = std::make_shared<Instrument>("cuboidal_detector_machine");
    addDetector(instrument, V3D(0.1, 0., 3.), 0, "det0");
    input->getSpectrum(0).setDetectorID(0);
    addDetector(instrument, V3D(0.2, 0., 3.), 1, "det1");
    input->getSpectrum(1).setDetectorID(1);
    addSource(instrument, V3D(0., 0., -4.), "source");
    addSample(instrument, V3D(0., 0., 0.), "sample");
    input->setInstrument(instrument);
    input->getAxis(0)->setUnit("DeltaE");
    input->mutableRun().addProperty("Ei", 23.);
    SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", input))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "unused_for_child"))
    const std::vector<double> qParams{0., 0.01, 1.};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qParams))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Direct"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(output->blocksize(), nbins)
    bool atLeastOneBinChecked{false};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &Ys = output->y(i);
      for (size_t j = 0; j < nbins; ++j) {
        const auto y = Ys[j];
        if (std::isfinite(y)) {
          TS_ASSERT_DELTA(y, 2.0, 1e-10)
          atLeastOneBinChecked = true;
        }
      }
    }
    TS_ASSERT(atLeastOneBinChecked)
  }

  void testEAndQBinningParams() {
    // SofQWNormalisedPolygon uses it's own setUpOutputWorkspace while
    // the other SofQW* algorithms use the one in SofQW.
    auto inWS = SofQWTest::loadTestFile();
    Mantid::Algorithms::SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Indirect"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EFixed", 1.84))
    const std::vector<double> eBinParams{-0.5, 0.1, -0.1, 0.2, 0.4};
    const std::vector<double> expectedEBinEdges{-0.5, -0.4, -0.3, -0.2, -0.1, 0.1, 0.3, 0.4};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EAxisBinning", eBinParams))
    const std::vector<double> qBinParams{0.5, 0.1, 1.0, 0.2, 2.};
    const std::vector<double> expectedQBinEdges{0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.4, 1.6, 1.8, 2.};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qBinParams))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), expectedQBinEdges.size() - 1)
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      const auto &x = outWS->x(i);
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_DELTA(x[j], expectedEBinEdges[j], 1e-12)
      }
    }
    const auto axis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis->length(), expectedQBinEdges.size())
    for (size_t i = 0; i < axis->length(); ++i) {
      TS_ASSERT_DELTA(axis->getValue(i), expectedQBinEdges[i], 1e-12)
    }
  }

  void testEBinWidthAsEAxisBinning() {
    // SofQWNormalisedPolygon uses it's own setUpOutputWorkspace while
    // the other SofQW* algorithms use the one in SofQW.
    auto inWS = SofQWTest::loadTestFile();
    Mantid::Algorithms::SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Indirect"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EFixed", 1.84))
    const double dE{0.3};
    // cppcheck-suppress unreadVariable
    const std::vector<double> eBinParams{dE};
    std::vector<double> expectedEBinEdges;
    const auto firstEdge = inWS->x(0).front();
    const auto lastEdge = inWS->x(0).back();
    auto currentEdge = firstEdge;
    while (currentEdge < lastEdge) {
      expectedEBinEdges.emplace_back(currentEdge);
      currentEdge += dE;
    }
    expectedEBinEdges.emplace_back(lastEdge);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EAxisBinning", eBinParams))
    const std::vector<double> qBinParams{0.5, 0.1, 1.0, 0.2, 2.};
    const std::vector<double> expectedQBinEdges{0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.4, 1.6, 1.8, 2.};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qBinParams))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), expectedQBinEdges.size() - 1)
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      const auto &x = outWS->x(i);
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_DELTA(x[j], expectedEBinEdges[j], 1e-12)
      }
    }
    const auto axis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis->length(), expectedQBinEdges.size())
    for (size_t i = 0; i < axis->length(); ++i) {
      TS_ASSERT_DELTA(axis->getValue(i), expectedQBinEdges[i], 1e-12)
    }
  }

  void testQBinWidthAsQAxisBinning() {
    // SofQWNormalisedPolygon uses it's own setUpOutputWorkspace while
    // the other SofQW* algorithms use the one in SofQW.
    auto inWS = SofQWTest::loadTestFile();
    Mantid::Algorithms::SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "_unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Indirect"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EFixed", 1.84))
    const double dQ{0.023};
    const std::vector<double> qBinParams{dQ};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qBinParams))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    const auto axis = outWS->getAxis(1);
    // Test only the Q bin width, not the actual edges.
    for (size_t i = 0; i < axis->length() - 1; ++i) {
      const auto delta = axis->getValue(i + 1) - axis->getValue(i);
      TS_ASSERT_DELTA(delta, dQ, 1e-12);
    }
  }

  void testDetectorTwoThetaRanges() {
    constexpr int nhist{2};
    constexpr int nbins{10};
    const BinEdges inX(nbins + 1, LinearGenerator(-5, 1.23));
    const Counts inY(nbins, 2.0);
    const Histogram inHistogram(inX, inY);
    MatrixWorkspace_sptr input = create<Workspace2D>(nhist, inHistogram);
    auto instrument = std::make_shared<Instrument>("cuboidal_detector_machine");
    addDetector(instrument, V3D(0.1, 0., 3.), 0, "det0");
    input->getSpectrum(0).setDetectorID(0);
    addDetector(instrument, V3D(0.2, 0., 3.), 1, "det1");
    input->getSpectrum(1).setDetectorID(1);
    addSource(instrument, V3D(0., 0., -4.), "source");
    addSample(instrument, V3D(0., 0., 0.), "sample");
    input->setInstrument(instrument);
    input->getAxis(0)->setUnit("DeltaE");
    input->mutableRun().addProperty("Ei", 23.);
    auto twoThetaRanges = std::make_shared<TableWorkspace>();
    twoThetaRanges->addColumn("int", "Detector ID");
    twoThetaRanges->addColumn("double", "Max two theta");
    twoThetaRanges->addColumn("double", "Min two theta");
    twoThetaRanges->setRowCount(nhist);
    auto column = twoThetaRanges->getColumn("Detector ID");
    column->cell<int>(0) = 0;
    column->cell<int>(1) = 1;
    column = twoThetaRanges->getColumn("Min two theta");
    column->cell<double>(0) = M_PI / 10.;
    column->cell<double>(1) = M_PI / 5.;
    column = twoThetaRanges->getColumn("Max two theta");
    column->cell<double>(0) = M_PI / 10. + M_PI / 50.;
    column->cell<double>(1) = M_PI / 5. + M_PI / 30.;
    SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", input))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "unused_for_child"))
    const std::vector<double> qParams{0., 0.5, 6.};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qParams))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Direct"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTwoThetaRanges", twoThetaRanges))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(output->blocksize(), nbins)
    bool atLeastOneBinAsserted{false};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &Ys = output->y(i);
      for (size_t j = 0; j < nbins; ++j) {
        const auto y = Ys[j];
        if (std::isfinite(y)) {
          TS_ASSERT_DELTA(y, 2.0, 1e-10)
          atLeastOneBinAsserted = true;
        }
      }
    }
    TS_ASSERT(atLeastOneBinAsserted)
  }

  /** Create an instance of the SofQWNormalisedPolygon algorithm object and
   * setting it's properties
   * @param inWS input matrix workspace pointer
   * @param twoThetaRanges input table workspace
   * @return the algorithm object
   */
  IAlgorithm_sptr setUpAlg(Mantid::API::MatrixWorkspace_sptr const &inputWS,
                           const std::shared_ptr<Mantid::DataObjects::TableWorkspace> &twoThetaRanges) {
    const std::vector<double> qBinParams{0.023};
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SofQWNormalisedPolygon");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("EMode", "Indirect");
    alg->setProperty("QAxisBinning", qBinParams);
    alg->setProperty("DetectorTwoThetaRanges", twoThetaRanges);
    return alg;
  }

  /** Create a table workspace
   * @param types A vector of strings of the data types of the columns
   * @param rowCount The number of detectors required
   * @return A pointer to the table workspace
   */
  std::shared_ptr<Mantid::DataObjects::TableWorkspace> createTableWorkspace(const std::vector<std::string> &dataTypes,
                                                                            const int rowCount) {
    auto twoThetaRanges = std::make_shared<TableWorkspace>();
    std::vector<std::string> names = {"Detector ID", "Max two theta", "Min two theta"};
    for (std::vector<std::string>::size_type i = 0; i < dataTypes.size(); i++) {
      if (i >= names.size()) {
        twoThetaRanges->addColumn(dataTypes[i], "N/A");
      } else {
        twoThetaRanges->addColumn(dataTypes[i], names[i]);
      }
    }
    twoThetaRanges->setRowCount(rowCount);
    return twoThetaRanges;
  }

  void testTableHasThreeColumns() {
    const std::vector<std::string> dataTypes = {"int", "double"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms()));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"], "DetectorTwoThetaRanges requires 3 columns");
  }

  void testTableHasFourColumns() {
    const std::vector<std::string> dataTypes = {"int", "double", "double", "double"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms()));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"], "DetectorTwoThetaRanges requires 3 columns");
  }

  void testColumnOneType() {
    const std::vector<std::string> dataTypes = {"double", "double", "double"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms()));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"],
                     "The first column of DetectorTwoThetaRanges should be of type int");
  }

  void testColumnTwoType() {
    const std::vector<std::string> dataTypes = {"int", "int", "double"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms()));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"],
                     "The second column of DetectorTwoThetaRanges should be of type double");
  }

  void testColumnThreeType() {
    const std::vector<std::string> dataTypes = {"int", "double", "int"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms()));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"],
                     "The third column of DetectorTwoThetaRanges should be of type double");
  }

  void testTableAndWorkspaceHaveSameNumDetectors() {
    const std::vector<std::string> dataTypes = {"int", "double", "double"};
    auto inputWS = SofQWTest::loadTestFile();
    const auto twoThetaRanges = createTableWorkspace(dataTypes, static_cast<int>(inputWS->getNumberHistograms() + 1));
    const IAlgorithm_sptr alg = setUpAlg(inputWS, twoThetaRanges);
    auto results = alg->validateInputs();
    TS_ASSERT_EQUALS(results["DetectorTwoThetaRanges"],
                     "The table and workspace do not have the same number of detectors");
  }
};

class SofQWNormalisedPolygonTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SofQWNormalisedPolygonTestPerformance *createSuite() { return new SofQWNormalisedPolygonTestPerformance(); }
  static void destroySuite(SofQWNormalisedPolygonTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void setUp() override {
    constexpr int nhist{10000};
    constexpr int nbins{1000};
    const BinEdges inX(nbins + 1, LinearGenerator(-25., 30. / nbins));
    const Counts inY(nbins, 2.0);
    const Histogram inHistogram(inX, inY);
    m_largeWS = create<Workspace2D>(nhist, inHistogram);
    auto instrument = std::make_shared<Instrument>("cuboidal_machine");
    constexpr double l2{4.};
    constexpr double twoThetaZero{M_PI / 20.};
    constexpr size_t rows{100};
    constexpr size_t columns{100};
    constexpr double twoThetaDelta{2. * M_PI / 3. / columns};
    constexpr double bankHeight{1.};
    constexpr double heightDelta{bankHeight / static_cast<double>(rows)};
    for (size_t hIndex = 0; hIndex < columns; ++hIndex) {
      const auto angle = twoThetaZero + static_cast<double>(hIndex) * twoThetaDelta;
      const auto x = l2 * std::sin(angle);
      const auto z = l2 * std::cos(angle);
      for (size_t vIndex = 0; vIndex < rows; ++vIndex) {
        const auto y = -bankHeight / 2. + static_cast<double>(vIndex) * heightDelta;
        const auto index = hIndex * rows + vIndex;
        const auto detID = static_cast<int>(index);
        addDetector(instrument, V3D(x, y, z), detID, "det-" + std::to_string(vIndex) + ":" + std::to_string(hIndex));
        m_largeWS->getSpectrum(index).setDetectorID(detID);
      }
    }
    addSource(instrument, V3D(0., 0., -4.), "source");
    addSample(instrument, V3D(0., 0., 0.), "sample");
    m_largeWS->setInstrument(instrument);
    m_largeWS->getAxis(0)->setUnit("DeltaE");
    m_largeWS->mutableRun().addProperty("Ei", 5.3);
  }

  void tearDown() override {}

  void testExec() {
    SofQWNormalisedPolygon alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_largeWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "unused_for_child"))
    const std::vector<double> qParams{0.01};
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("QAxisBinning", qParams))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EMode", "Direct"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
  }

private:
  MatrixWorkspace_sptr m_largeWS;
};
