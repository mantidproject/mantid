// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVECSVTEST_H_
#define SAVECSVTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveCSV.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::HistogramDx;
using Mantid::HistogramData::LinearGenerator;

// Notice, the SaveCSV algorithm currently does not create
// an output workspace and therefore no tests related to the
// output workspace is performed.

// Notice also that currently no tests have been added to test
// this class when trying to save a 2D workspace with SaveCSV.

namespace {
const size_t nBins = 3;
}

class SaveCSVTest : public CxxTest::TestSuite {
public:
  static SaveCSVTest *createSuite() { return new SaveCSVTest(); }
  static void destroySuite(SaveCSVTest *suite) { delete suite; }

  SaveCSVTest() {
    // create dummy 2D-workspace with one pixel
    Workspace_sptr localWorkspace =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);
    Workspace2D_sptr localWorkspace2D_onePixel =
        boost::dynamic_pointer_cast<Workspace2D>(localWorkspace);

    double d = 0.0;
    for (int i = 0; i < 10; ++i, d += 0.1) {
      localWorkspace2D_onePixel->mutableX(0)[i] = d;
      localWorkspace2D_onePixel->mutableY(0)[i] = d + 1.0;
      localWorkspace2D_onePixel->mutableE(0)[i] = d + 2.0;
    }

    AnalysisDataService::Instance().add("SAVECSVTEST-testSpace",
                                        localWorkspace);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    algToBeTested.setPropertyValue("InputWorkspace", "SAVECSVTEST-testSpace");

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);
    TS_ASSERT_EQUALS(algToBeTested.isExecuted(), false)

    // Now set it...
    // specify name of file to save 1D-workspace to
    outputFile = "testOfSaveCSV.csv";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result =
                                 algToBeTested.getPropertyValue("Filename"))
    TS_ASSERT(!result.compare(outputFile));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // has the algorithm written a file to disk?

    TS_ASSERT(Poco::File(outputFile).exists());

    // Do a few tests to see if the content of outputFile is what you
    // expect.

    std::ifstream in(outputFile.c_str());

    std::string Amarker;
    double d1, d2, d3;
    std::string separator;
    std::string number_plus_comma;

    in >> Amarker >> d1 >> separator >> d2 >> separator >> d3 >> separator >>
        number_plus_comma;

    in.close();

    TS_ASSERT_EQUALS(Amarker, "A");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_DELTA(d1, 0.0, 1e-5);
    TS_ASSERT_DELTA(d2, 0.1, 1e-5);
    TS_ASSERT_DELTA(d3, 0.2, 1e-5);
    TS_ASSERT_EQUALS(number_plus_comma, "0.3,");

    // remove file created by this algorithm
    Poco::File(outputFile).remove();
    AnalysisDataService::Instance().remove("SAVECSVTEST-testSpace");
  }

  void test_saving_1D_error_with_x_error() {
    std::string fileName = "test_save_csv_with_x_errors_1D.csv";
    const size_t numberOfSpectra = 1;
    runTestWithDx(fileName, numberOfSpectra);
  }

  void test_saving_2D_error_with_x_error() {
    std::string fileName = "test_save_csv_with_x_errors_2D.csv";
    const size_t numberOfSpectra = 3;
    runTestWithDx(fileName, numberOfSpectra);
  }

private:
  SaveCSV algToBeTested;
  std::string outputFile;

  MatrixWorkspace_sptr createWorkspaceWithDxValues(const size_t nSpec) const {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", nSpec,
                                                  nBins + 1, nBins);
    BinEdges edges(nBins + 1, LinearGenerator(0, 1));
    for (size_t j = 0; j < nSpec; ++j) {
      ws->setHistogram(j, edges, Counts(nBins, double(j)));
      ws->setPointStandardDeviations(j, nBins, sqrt(double(j)));
    }
    return ws;
  }

  void runTestWithDx(std::string fileName, const size_t nSpec) {
    // Arrange
    auto ws = createWorkspaceWithDxValues(nSpec);

    // Act
    SaveCSV saveCSV;
    TS_ASSERT_THROWS_NOTHING(saveCSV.initialize());
    TS_ASSERT(saveCSV.isInitialized());
    saveCSV.setProperty("InputWorkspace", ws);
    saveCSV.setProperty("SaveXerrors", true);
    saveCSV.setPropertyValue("Filename", fileName);
    fileName = saveCSV.getPropertyValue("Filename");

    TS_ASSERT_THROWS_NOTHING(saveCSV.execute());
    TS_ASSERT(saveCSV.isExecuted());

    // Assert
    TS_ASSERT(Poco::File(fileName).exists());
    evaluateFileWithDX(fileName, nSpec);

    // Clean up
    Poco::File(fileName).remove();
  }

  void evaluateFileWithDX(std::string fileName, const size_t nSpec) const {
    std::ifstream stream(fileName.c_str());
    std::istringstream dataStream;
    std::string line;

    std::string stringMarker;
    double indexMarker;
    double d1, d2, d3, dEnd; // There are 3 bins
    std::string separator;

    // Evalute the first line. We are expecting only one line for the Xvalues
    // here.
    getline(stream, line);
    dataStream.str(std::string());
    dataStream.clear();
    dataStream.str(line);
    dataStream >> stringMarker >> d1 >> separator >> d2 >> separator >> d3 >>
        separator >> dEnd >> separator;

    TS_ASSERT_EQUALS(stringMarker, "A");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_DELTA(d1, 0.0, 1e-5);
    TS_ASSERT_DELTA(d2, 1.0, 1e-5);
    TS_ASSERT_DELTA(d3, 2.0, 1e-5);
    TS_ASSERT_DELTA(dEnd, 3.0, 1e-5);

    // Now evaluate the Y value entries which follow directly after
    for (size_t spec = 0; spec < nSpec; ++spec) {
      getline(stream, line);
      dataStream.str(std::string());
      dataStream.clear();
      dataStream.str(line);
      dataStream >> indexMarker >> d1 >> separator >> d2 >> separator >> d3 >>
          separator;
      TS_ASSERT_EQUALS(indexMarker, double(spec));
      TS_ASSERT_EQUALS(separator, ",");
      TS_ASSERT_DELTA(d1, double(spec), 1e-5);
      TS_ASSERT_DELTA(d2, double(spec), 1e-5);
      TS_ASSERT_DELTA(d3, double(spec), 1e-5);
    }

    // We expect an empty line here
    getline(stream, line);

    // Check the ERROR identifier
    getline(stream, line);
    dataStream.str(std::string());
    dataStream.clear();
    dataStream.str(line);
    dataStream >> stringMarker;
    TS_ASSERT_EQUALS(stringMarker, "ERRORS");

    // Check the y errors
    for (size_t spec = 0; spec < nSpec; ++spec) {
      getline(stream, line);
      dataStream.str(std::string());
      dataStream.clear();
      dataStream.str(line);
      dataStream >> indexMarker >> d1 >> separator >> d2 >> separator >> d3 >>
          separator;
      TS_ASSERT_EQUALS(indexMarker, spec);
      TS_ASSERT_EQUALS(separator, ",");
      TS_ASSERT_DELTA(d1, sqrt(double(spec)), 1e-5);
      TS_ASSERT_DELTA(d2, sqrt(double(spec)), 1e-5);
      TS_ASSERT_DELTA(d3, sqrt(double(spec)), 1e-5);
    }

    // We expect an empty line here
    getline(stream, line);

    // Check the XERROR identifier
    getline(stream, line);
    dataStream.str(std::string());
    dataStream.clear();
    dataStream.str(line);
    dataStream >> stringMarker;
    TS_ASSERT_EQUALS(stringMarker, "XERRORS");

    // Check the x errors
    for (size_t spec = 0; spec < nSpec; ++spec) {
      getline(stream, line);
      dataStream.str(std::string());
      dataStream.clear();
      dataStream.str(line);
      dataStream >> indexMarker >> d1 >> separator >> d2 >> separator >> d3 >>
          separator;
      TS_ASSERT_EQUALS(indexMarker, spec);
      TS_ASSERT_EQUALS(separator, ",");
      TS_ASSERT_DELTA(d1, sqrt(double(spec)), 1e-5);
      TS_ASSERT_DELTA(d2, sqrt(double(spec)), 1e-5);
      TS_ASSERT_DELTA(d3, sqrt(double(spec)), 1e-5);
    }
    stream.close();
  }
};

#endif /*SAVECSVTEST_H_*/
