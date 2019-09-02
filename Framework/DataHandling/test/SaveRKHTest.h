// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVERKHTEST_H_
#define SAVERKHTEST_H_

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveRKH.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <Poco/File.h>
#include <fstream>
#include <numeric>
using namespace Mantid::API;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::PointStandardDeviations;

class SaveRKHTest : public CxxTest::TestSuite {
public:
  static SaveRKHTest *createSuite() { return new SaveRKHTest(); }
  static void destroySuite(SaveRKHTest *suite) { delete suite; }
  /// Constructor
  SaveRKHTest() : outputFile("SAVERKH.out") {}

  ~SaveRKHTest() override {
    // Remove the file
    if (Poco::File(outputFile).exists()) {
      Poco::File(outputFile).remove();
    }
  }

  void testInit() {
    using namespace Mantid::DataHandling;
    TS_ASSERT_THROWS_NOTHING(testAlgorithm1.initialize());
    TS_ASSERT_EQUALS(testAlgorithm1.isInitialized(), true);

    TS_ASSERT_THROWS_NOTHING(testAlgorithm2.initialize());
    TS_ASSERT_EQUALS(testAlgorithm2.isInitialized(), true);
  }

  void testExecHorizontal() {
    if (!testAlgorithm1.isInitialized())
      testAlgorithm1.initialize();

    // No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(testAlgorithm1.execute(), const std::runtime_error &);
    // Need a test workspace to use as input
    MatrixWorkspace_sptr inputWS1 =
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 1.0);
    inputWS1->setDistribution(true);

    // Register workspace
    AnalysisDataService::Instance().add("testInputOne", inputWS1);

    testAlgorithm1.setPropertyValue("InputWorkspace", "testInputOne");
    testAlgorithm1.setPropertyValue("Filename", outputFile);
    outputFile =
        testAlgorithm1.getPropertyValue("Filename"); // get absolute path
    testAlgorithm2.setProperty<bool>("Append", false);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlgorithm1.execute());
    TS_ASSERT(testAlgorithm1.isExecuted());

    // Check if the file exists
    TS_ASSERT(Poco::File(outputFile).exists());

    // Open it and check a few lines
    std::ifstream file(outputFile.c_str());

    TS_ASSERT(file);

    // Bury the first 5 lines and then read the next
    int count(0);
    std::string fileline;
    while (++count < 7) {
      getline(file, fileline);
    }
    std::istringstream strReader(fileline);
    double x(0.0), y(0.0), err(0.0);
    strReader >> x >> y >> err;

    // Test values
    TS_ASSERT_DELTA(x, 1.5, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    // Read some other lines
    count = 0;
    while (++count < 6) {
      getline(file, fileline);
    }

    x = 0.0;
    y = 0.0;
    err = 0.0;
    strReader.clear();
    strReader.str(fileline.c_str());
    strReader >> x >> y >> err;

    // Test values
    TS_ASSERT_DELTA(x, 6.5, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    file.close();
  }

  void xtestExecVertical() {
    // Now a workspace of the other kind
    if (!testAlgorithm2.isInitialized())
      testAlgorithm2.initialize();

    TS_ASSERT_THROWS(testAlgorithm2.execute(), const std::runtime_error &);

    using namespace Mantid::API;
    MatrixWorkspace_sptr inputWS2 =
        WorkspaceCreationHelper::create2DWorkspaceBinned(10, 1, 0.0);
    inputWS2->setDistribution(true);
    // Register workspace
    AnalysisDataService::Instance().add("testInputTwo", inputWS2);

    testAlgorithm2.setPropertyValue("InputWorkspace", "testInputTwo");
    testAlgorithm2.setPropertyValue("Filename", outputFile);
    outputFile =
        testAlgorithm2.getPropertyValue("Filename"); // get absolute path
    testAlgorithm2.setProperty<bool>("Append", false);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlgorithm2.execute());
    TS_ASSERT(testAlgorithm2.isExecuted());

    // Check if the file exists
    TS_ASSERT(Poco::File(outputFile).exists());

    // Open it and check a few lines
    std::ifstream file2(outputFile.c_str());

    TS_ASSERT(file2);

    // Bury the first 5 lines and then read the next
    int count = 0;
    std::string fileline;
    while (++count < 7) {
      getline(file2, fileline);
    }
    std::istringstream strReader(fileline);
    strReader.clear();
    strReader.str(fileline.c_str());
    double x(0.0), y(0.0), err(0.0);
    strReader >> x >> y >> err;

    // Test values
    TS_ASSERT_DELTA(x, 0.0, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    // Read some other lines
    count = 0;
    while (++count < 6) {
      getline(file2, fileline);
    }

    x = 0.0;
    y = 0.0;
    err = 0.0;
    strReader.clear();
    strReader.str(fileline.c_str());
    strReader >> x >> y >> err;

    // Test values
    TS_ASSERT_DELTA(x, 0.0, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    file2.close();
  }

  void testExecWithDx() {

    if (!testAlgorithm3.isInitialized())
      testAlgorithm3.initialize();

    // No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(testAlgorithm3.execute(), const std::runtime_error &);
    // Need a test workspace to use as input
    auto inputWS3 = createInputWorkspaceHistoWithXerror();
    inputWS3->setDistribution(false);

    // Register workspace
    AnalysisDataService::Instance().add("testInputThree", inputWS3);

    testAlgorithm3.setPropertyValue("InputWorkspace", "testInputThree");
    testAlgorithm3.setPropertyValue("Filename", outputFile);
    outputFile =
        testAlgorithm3.getPropertyValue("Filename"); // get absolute path
    testAlgorithm3.setProperty<bool>("Append", false);

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlgorithm3.execute());
    TS_ASSERT(testAlgorithm3.isExecuted());

    // Check if the file exists
    TS_ASSERT(Poco::File(outputFile).exists());

    // Open it and check a few lines
    std::ifstream file(outputFile.c_str());

    TS_ASSERT(file);

    // Bury the first 5 lines and then read the next
    int count(0);
    std::string fileline;
    while (++count < 7) {
      getline(file, fileline);
    }
    std::istringstream strReader(fileline);
    double x(0.0), y(0.0), err(0.0), dx(0.0);
    strReader >> x >> y >> err >> dx;

    // Test values
    TSM_ASSERT_DELTA("Expecting mean of 0 and 1", x, 0.5, 1e-08);
    TS_ASSERT_DELTA(y, 1.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.0, 1e-06);
    TS_ASSERT_DELTA(dx, 0.1, 1e-06);

    // We are at the first data entry now. We step over
    // Three more entries
    count = 0;
    while (++count < 4) {
      getline(file, fileline);
    }

    x = 0.0;
    y = 0.0;
    err = 0.0;
    dx = 0.0;
    strReader.clear();
    strReader.str(fileline.c_str());
    strReader >> x >> y >> err >> dx;

    // Test values
    TSM_ASSERT_DELTA("Expecting mean of 3 and 4", x, 3.5, 1e-08);
    TS_ASSERT_DELTA(y, 1.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.0, 1e-06);
    TS_ASSERT_DELTA(dx, 3.1, 1e-06);

    file.close();
  }

private:
  /// The algorithm object
  Mantid::DataHandling::SaveRKH testAlgorithm1, testAlgorithm2, testAlgorithm3;

  /// The name of the file to use as output
  std::string outputFile;

  /// Provides a workpace with a x error value
  MatrixWorkspace_sptr createInputWorkspaceHistoWithXerror() const {
    size_t nSpec = 1;
    const size_t x_length = 11;
    const size_t y_length = x_length - 1;
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", nSpec, x_length, y_length);
    BinEdges x(x_length, LinearGenerator(0.0, 1.0));
    PointStandardDeviations dx(y_length, LinearGenerator(0.1, 1.0));
    for (size_t j = 0; j < nSpec; ++j) {
      ws->setBinEdges(j, x);
      ws->setPointStandardDeviations(j, dx);
      ws->dataY(j).assign(y_length, double(1));
      ws->dataE(j).assign(y_length, double(1));
    }
    return ws;
  }
};

#endif // SAVERKHTEST_H_
