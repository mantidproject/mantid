// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_REPLICATEMDTEST_H_
#define MANTID_MDALGORITHMS_REPLICATEMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/ReplicateMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <string>
#include <vector>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace {

MDHistoWorkspace_sptr makeHistoWorkspace(const std::vector<int> &shape,
                                         bool transpose = false,
                                         double value = 0.0) {

  IAlgorithm *create =
      FrameworkManager::Instance().createAlgorithm("CreateMDHistoWorkspace");
  create->setChild(true);
  create->initialize();

  const std::string allNames[5] = {"A", "B", "C", "D", "E"};
  const std::string allUnits[5] = {"AU", "BU", "CU", "DU", "EU"};

  std::vector<std::string> names;
  std::vector<std::string> units;
  size_t flatSize = 1;
  std::vector<double> extents;
  for (size_t i = 0; i < shape.size(); ++i) {
    flatSize *= shape[i];
    names.push_back(allNames[i]);
    units.push_back(allUnits[i]);
    extents.push_back(-10);
    extents.push_back(10);
  }

  if (value == 0.0) {
    std::vector<double> signalArray;
    signalArray.reserve(flatSize);
    for (size_t i = 0; i < flatSize; ++i) {
      signalArray.push_back(static_cast<double>(i + 1));
    }
    create->setProperty("SignalInput", signalArray);
  } else {
    create->setProperty("SignalInput", std::vector<double>(flatSize, 1.0));
  }
  create->setProperty("ErrorInput", std::vector<double>(flatSize, 1));

  create->setProperty("Dimensionality", int(shape.size()));
  create->setProperty("Extents", extents);
  create->setProperty("NumberOfBins", shape);
  create->setProperty("Names", names);
  create->setProperty("Units", units);
  create->setPropertyValue("OutputWorkspace", "dummy");
  create->execute();
  IMDHistoWorkspace_sptr outWs = create->getProperty("OutputWorkspace");

  if (transpose) {

    class Decreasing {
    private:
      size_t m_current;

    public:
      Decreasing(size_t start) : m_current(start) {}
      size_t operator()() { return --m_current; }
    };

    // Generate the axis order 0, 1, 2 ... in reverse
    std::vector<int> axes(outWs->getNumDims());
    Decreasing op(outWs->getNumDims());
    for (int &axis : axes) {
      axis = static_cast<int>(op());
    }

    IAlgorithm *transpose =
        FrameworkManager::Instance().createAlgorithm("TransposeMD");
    transpose->setChild(true);
    transpose->initialize();
    transpose->setProperty("InputWorkspace", outWs);
    transpose->setProperty("Axes", axes);
    transpose->setPropertyValue("OutputWorkspace", "dummy");
    transpose->execute();
    outWs = transpose->getProperty("OutputWorkspace");
  }

  return boost::dynamic_pointer_cast<MDHistoWorkspace>(outWs);
}
} // namespace

//=====================================================================================
// Functional Tests
//=====================================================================================
class ReplicateMDTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReplicateMDTest *createSuite() { return new ReplicateMDTest(); }
  static void destroySuite(ReplicateMDTest *suite) { delete suite; }

  void test_init() {
    ReplicateMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_size_check_on_dimensionality() {

    std::vector<int> badDataShape;
    badDataShape.push_back(3);
    badDataShape.push_back(3);
    badDataShape.push_back(3); // 3rd dimension given

    std::vector<int> goodDataShape;
    goodDataShape.push_back(3);
    goodDataShape.push_back(3);
    goodDataShape.push_back(1); // Integrate so should be OK

    std::vector<int> shapeShape;
    shapeShape.push_back(3);
    shapeShape.push_back(3);
    shapeShape.push_back(3);

    auto dataWSGood = makeHistoWorkspace(goodDataShape);
    auto dataWSBad = makeHistoWorkspace(badDataShape);
    auto shapeWS = makeHistoWorkspace(shapeShape);

    ReplicateMD alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSBad);
    alg.setProperty("ShapeWorkspace", shapeWS);
    TSM_ASSERT_EQUALS("Shape and data are the same size. Should fail.", 1,
                      alg.validateInputs().size());

    // Try again with different property value
    alg.setProperty("DataWorkspace", dataWSGood);
    TSM_ASSERT_EQUALS("Interated dim should not be counted.", 0,
                      alg.validateInputs().size());
  }

  void test_basic_shape_check() {
    auto shapeWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 3 /*numDims*/, 4 /*numBins in each dimension*/);

    // Data workspace is right size (number of dimensions), but wrong shape
    // (number of bins in each)
    auto dataWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, shapeWS->getNumDims() - 1 /*numDims*/,
        3 /*numBins in each dimension*/);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    TSM_ASSERT_EQUALS("Shape and data are different shapes. Should fail.", 1,
                      alg.validateInputs().size());
  }

  void test_very_simple_exec() {
    auto shapeWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 3 /*numDims*/, 4 /*numBins in each dimension*/);

    auto dataWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2 /*signal*/, 2 /*numDims*/, 4 /*numBins in each dimension*/);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());
    TS_ASSERT_EQUALS(dataWS->getSignalAt(0), outWS->getSignalAt(0));
  }

  void test_replicate_1d_vertical() {

    std::vector<int> shapeShape = {10, 10};
    auto shapeWS = makeHistoWorkspace(shapeShape);

    std::vector<int> dataShape = {1, 10};
    auto dataWS = makeHistoWorkspace(dataShape);
    for (int i = 0; i < dataShape[1]; ++i) {
      dataWS->setSignalAt(i, i); // Vertically increasing.
    }

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());

    // Check the output data. Should be horizontally invariant, but vertically
    // increasing

    TSM_ASSERT_EQUALS("Neighours horizontal. Should be the same.",
                      outWS->getSignalAt(0), outWS->getSignalAt(1));
    TSM_ASSERT_DIFFERS(
        "Neighours vertical. Should be different.", outWS->getSignalAt(0),
        outWS->getSignalAt(shapeShape[0] /*one row verically down*/));
    TSM_ASSERT_EQUALS(
        "Vertical points should be same in data and output",
        dataWS->getSignalAt(dataShape[0]),
        outWS->getSignalAt(shapeShape[0] /*one row verically down*/));
  }

  void test_replicate_1d_horizontal() {

    std::vector<int> shapeShape = {10, 10};
    auto shapeWS = makeHistoWorkspace(shapeShape);

    std::vector<int> dataShape = {10, 1};
    auto dataWS = makeHistoWorkspace(dataShape);
    for (int i = 0; i < dataShape[0]; ++i) {
      dataWS->setSignalAt(i, i); // Horizontally increasing.
    }

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());

    // Check the output data. Should be horizontally invariant, but vertically
    // increasing

    TSM_ASSERT_EQUALS(
        "Neighbours vertical. Should be the same.", outWS->getSignalAt(0),
        outWS->getSignalAt(shapeShape[0] /*one row vertically down*/));
    TSM_ASSERT_DIFFERS("Neighbours horizontal. Should be different.",
                       outWS->getSignalAt(0), outWS->getSignalAt(1));
    TSM_ASSERT_EQUALS("Horizontal points should be same in data and output",
                      dataWS->getSignalAt(1), outWS->getSignalAt(1));
  }

  void test_auto_transpose_2d() {

    std::vector<int> shapeShape = {10, 20, 10};
    auto shapeWS = makeHistoWorkspace(shapeShape);

    std::vector<int> dataShapePreTranspose = {10, 20};
    auto dataWSTranspose = makeHistoWorkspace(
        dataShapePreTranspose, true /*transpose it to make it 20 by 10*/);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSTranspose);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());
  }

  void test_extra_dimensions() {

    std::vector<int> shapeShape = {5, 7, 1, 1};
    auto shapeWS = makeHistoWorkspace(shapeShape, false, 1);

    std::vector<int> dataShape = {1, 7, 1, 1};
    auto dataWSTranspose = makeHistoWorkspace(dataShape);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSTranspose);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    try {
      alg.execute();
    } catch (std::exception &e) {
      TSM_ASSERT(e.what(), false);
      return;
    }
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());

    size_t index = 0;
    for (size_t i = 0; i < 7; ++i) {
      for (size_t j = 0; j < 5; ++j, ++index) {
        auto signal = outWS->signalAt(index);
        TS_ASSERT_EQUALS(signal, static_cast<double>(i + 1));
      }
    }
  }

  void test_extra_dimensions_1() {

    std::vector<int> shapeShape = {5, 7, 1, 1};
    auto shapeWS = makeHistoWorkspace(shapeShape, false, 1);

    std::vector<int> dataShape = {5, 1, 1, 1};
    auto dataWSTranspose = makeHistoWorkspace(dataShape);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSTranspose);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    try {
      alg.execute();
    } catch (std::exception &e) {
      TSM_ASSERT(e.what(), false);
      return;
    }
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    // Very basic sanity checks
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
    TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());

    size_t index = 0;
    for (size_t i = 0; i < 7; ++i) {
      for (size_t j = 0; j < 5; ++j, ++index) {
        auto signal = outWS->signalAt(index);
        TS_ASSERT_EQUALS(signal, static_cast<double>(j + 1));
      }
    }
  }

  void test_extra_dimensions_in_wrong_order() {

    std::vector<int> shapeShape = {5, 1, 7, 1};
    auto shapeWS = makeHistoWorkspace(shapeShape, false, 1);

    std::vector<int> dataShape = {1, 1, 7, 1};
    auto dataWSTranspose = makeHistoWorkspace(dataShape);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSTranspose);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_wrong_number_of_dimensions() {

    std::vector<int> shapeShape = {5, 7, 1, 1};
    auto shapeWS = makeHistoWorkspace(shapeShape, false, 1);

    std::vector<int> dataShape = {5, 1};
    auto dataWSTranspose = makeHistoWorkspace(dataShape);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSTranspose);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
class ReplicateMDTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReplicateMDTestPerformance *createSuite() {
    return new ReplicateMDTestPerformance();
  }
  static void destroySuite(ReplicateMDTestPerformance *suite) { delete suite; }

  void test_performance() {

    std::vector<int> shapeShape = {1000, 1000};
    auto shapeWS = makeHistoWorkspace(shapeShape);

    std::vector<int> dataShape = {1000, 1};
    auto dataWS = makeHistoWorkspace(dataShape);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
  }
};

#endif /* MANTID_MDALGORITHMS_REPLICATEMDTEST_H_ */
