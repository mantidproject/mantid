// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADRKHTEST_H_
#define LOADRKHTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadRKH.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::ConfigServiceImpl;
using namespace Mantid::Kernel;

class LoadRKHTest : public CxxTest::TestSuite {
public:
  static LoadRKHTest *createSuite() { return new LoadRKHTest(); }
  static void destroySuite(LoadRKHTest *suite) { delete suite; }

  // A sample file is in the repository
  LoadRKHTest()
      : dataFile(""), tempFile("LoadRKH_test_file_2D"),
        tempFile2("LoadRKH_test_file_1D_with_DX"),
        tempFile3("LoadRKL_with_second_header") {
    dataFile = "DIRECT.041";
  }

  void testConfidence() {
    Mantid::DataHandling::LoadRKH loader;
    loader.initialize();
    loader.setPropertyValue("Filename", dataFile);

    FileDescriptor descriptor(loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(20, loader.confidence(descriptor));
  }

  void testInit() {
    Mantid::DataHandling::LoadRKH loadrkh;
    TS_ASSERT_THROWS_NOTHING(loadrkh.initialize());
    TS_ASSERT(loadrkh.isInitialized());
  }

  void test1D() {
    // Set the facility
    ConfigService::Instance().setString("default.facility", "ISIS");
    Mantid::DataHandling::LoadRKH loadrkh;
    if (!loadrkh.isInitialized())
      loadrkh.initialize();

    // No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(loadrkh.execute(), const std::runtime_error &);

    // Set the file name
    loadrkh.setPropertyValue("Filename", dataFile);

    std::string outputSpace = "outer";
    // Set an output workspace
    loadrkh.setPropertyValue("OutputWorkspace", outputSpace);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result =
                                 loadrkh.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);

    // Should now throw nothing
    TS_ASSERT_THROWS_NOTHING(loadrkh.execute());
    TS_ASSERT(loadrkh.isExecuted());

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    // Now need to test the resultant workspace, first retrieve it
    Workspace_sptr rkhspace;
    TS_ASSERT_THROWS_NOTHING(
        rkhspace = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr data = boost::dynamic_pointer_cast<Workspace2D>(rkhspace);

    // The data in the 2D workspace does not match the file data directly
    // because the
    // file contains bin-centered values and the algorithm adjusts the x values
    // so that
    // they are bin edge values

    // Single histogram
    TS_ASSERT_EQUALS(data->getNumberHistograms(), 1);

    // Test the size of the data vectors (there should be 102 data points so x
    // have 103)
    TS_ASSERT_EQUALS(static_cast<int>(data->dataX(0).size()), 102);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(0).size()), 102);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataE(0).size()), 102);

    // Test first 3 bin edges for the correct values
    double tolerance(1e-06);
    TS_ASSERT_DELTA(data->dataX(0)[0], 1.34368, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[1], 1.37789, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[2], 1.41251, tolerance);
    // Test a couple of random ones
    TS_ASSERT_DELTA(data->dataX(0)[20], 2.20313, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[45], 4.08454, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[87], 11.52288, tolerance);
    // Test the last 3
    TS_ASSERT_DELTA(data->dataX(0)[100], 15.88747, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[101], 16.28282, tolerance);

    // Now Y values
    TS_ASSERT_DELTA(data->dataY(0)[0], 0.168419, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[25], 2.019846, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[99], 0.0, tolerance);

    // Now E values
    TS_ASSERT_DELTA(data->dataE(0)[0], 0.122346, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[25], 0.018345, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[99], 0.0, tolerance);
  }

  void test2D() {
    // Set the facility
    ConfigService::Instance().setString("default.facility", "ISIS");

    // write a small file to load
    writeTestFile();

    Mantid::DataHandling::LoadRKH rkhAlg;

    rkhAlg.initialize();

    // Set the file name
    rkhAlg.setPropertyValue("Filename", tempFile);

    std::string outputSpace = "outer";
    // Set an output workspace
    rkhAlg.setPropertyValue("OutputWorkspace", outputSpace);

    // check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = rkhAlg.getPropertyValue("Filename"))
    // Paths change, so this comparison does not work in general
    // TS_ASSERT_EQUALS( result, tempFile );

    TS_ASSERT_THROWS_NOTHING(result =
                                 rkhAlg.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);

    // Should now throw nothing
    TS_ASSERT_THROWS_NOTHING(rkhAlg.execute());
    TS_ASSERT(rkhAlg.isExecuted());

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    // Now need to test the resultant workspace, first retrieve it
    Workspace_sptr rkhspace;
    TS_ASSERT_THROWS_NOTHING(
        rkhspace = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr data = boost::dynamic_pointer_cast<Workspace2D>(rkhspace);

    TS_ASSERT_EQUALS(data->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(static_cast<int>(data->dataX(0).size()), 3);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(0).size()), 2);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(1).size()), 2);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataE(0).size()), 2);

    double tolerance(1e-06);
    // check a sample of values, the workspace is pretty small and so this will
    // check nearly all of them
    TS_ASSERT_DELTA(data->dataX(0)[0], -3.000000e-01, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[1], -2.900000e-01, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[2], -2.800000e-01, tolerance);

    TS_ASSERT_DELTA(data->dataX(1)[0], -3.000000e-01, tolerance);
    TS_ASSERT_DELTA(data->dataX(1)[1], -2.900000e-01, tolerance);
    TS_ASSERT_DELTA(data->dataX(1)[2], -2.800000e-01, tolerance);

    TS_ASSERT_DELTA(data->dataY(0)[0], 11, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[1], 12, tolerance);
    TS_ASSERT_DELTA(data->dataY(1)[1], 22, tolerance);

    // Now E values
    TS_ASSERT_DELTA(data->dataE(0)[1], 2, tolerance);
    TS_ASSERT_DELTA(data->dataE(1)[0], 3, tolerance);
    TS_ASSERT_DELTA(data->dataE(1)[1], 4, tolerance);

    Axis *secondAxis = data->getAxis(1);
    TS_ASSERT_EQUALS(secondAxis->length(), 3)
    TS_ASSERT_DELTA((*secondAxis)(1), -2.850000e-01, tolerance);

    TS_ASSERT(data->isHistogramData())

    // remove(tempFile.c_str());
  }

  void test1DWithDx() {

    // Arrange
    ConfigService::Instance().setString("default.facility", "ISIS");
    writeTestFileWithDx();
    std::string outputSpace = "outer_1d_with_dx";

    // Act
    Mantid::DataHandling::LoadRKH rkhAlg;
    rkhAlg.initialize();
    rkhAlg.setPropertyValue("Filename", tempFile2);
    rkhAlg.setPropertyValue("OutputWorkspace", outputSpace);

    // Assert
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = rkhAlg.getPropertyValue("Filename"))
    TS_ASSERT_THROWS_NOTHING(result =
                                 rkhAlg.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);
    TS_ASSERT_THROWS_NOTHING(rkhAlg.execute());
    TS_ASSERT(rkhAlg.isExecuted());

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    // Now need to test the resultant workspace, first retrieve it
    Workspace_sptr rkhspace;

    TS_ASSERT_THROWS_NOTHING(
        rkhspace = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr data = boost::dynamic_pointer_cast<Workspace2D>(rkhspace);

    TS_ASSERT_EQUALS(data->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(static_cast<int>(data->dataX(0).size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(data->dx(0).size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(0).size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(0).size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataE(0).size()), 10);

    double tolerance(1e-06);

    TS_ASSERT_DELTA(data->dataX(0)[0], 0.5, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[1], 1.5, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[2], 2.5, tolerance);

    TS_ASSERT_DELTA(data->dataY(0)[0], 1.000000e+00, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[1], 1.000000e+00, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[1], 1.000000e+00, tolerance);

    TS_ASSERT_DELTA(data->dataE(0)[1], 1.000000e+00, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[0], 1.000000e+00, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[1], 1.000000e+00, tolerance);

    TS_ASSERT_DELTA(data->dx(0)[0], 5.000000e-01, tolerance);
    TS_ASSERT_DELTA(data->dx(0)[1], 1.207107e+00, tolerance);
    TS_ASSERT_DELTA(data->dx(0)[2], 1.573132e+00, tolerance);

    remove(tempFile2.c_str());
  }

  void test_LoadRKH_with_second_header() {
    // Arrange
    ConfigService::Instance().setString("default.facility", "ISIS");
    writeTestFileWithSecondHeader();
    std::string outputSpace = "rkh_with_second_header";

    // Act
    Mantid::DataHandling::LoadRKH rkhAlg;
    rkhAlg.initialize();
    rkhAlg.setPropertyValue("Filename", tempFile3);
    rkhAlg.setPropertyValue("OutputWorkspace", outputSpace);

    // Assert
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = rkhAlg.getPropertyValue("Filename"))
    TS_ASSERT_THROWS_NOTHING(result =
                                 rkhAlg.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);
    TS_ASSERT_THROWS_NOTHING(rkhAlg.execute());
    TS_ASSERT(rkhAlg.isExecuted());

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    // Now need to test the resultant workspace, first retrieve it
    Workspace_sptr rkhspace;

    TS_ASSERT_THROWS_NOTHING(
        rkhspace = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr data = boost::dynamic_pointer_cast<Workspace2D>(rkhspace);

    TS_ASSERT_EQUALS(data->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(static_cast<int>(data->dataX(0).size()), 4);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataY(0).size()), 4);
    TS_ASSERT_EQUALS(static_cast<int>(data->dataE(0).size()), 4);

    double tolerance(1e-06);

    TS_ASSERT_DELTA(data->dataX(0)[0], 0.00520, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[1], 0.00562, tolerance);
    TS_ASSERT_DELTA(data->dataX(0)[2], 0.00607, tolerance);

    TS_ASSERT_DELTA(data->dataY(0)[0], 1.055855e+00, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[1], 9.784999e-01, tolerance);
    TS_ASSERT_DELTA(data->dataY(0)[2], 1.239836e+00, tolerance);

    TS_ASSERT_DELTA(data->dataE(0)[0], 2.570181e-01, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[1], 1.365013e-01, tolerance);
    TS_ASSERT_DELTA(data->dataE(0)[2], 9.582824e-02, tolerance);

    remove(tempFile3.c_str());
  }

private:
  std::string dataFile, tempFile, tempFile2, tempFile3;

  /** Create a tiny 2x2 workspace in a tempory file that should
   *  be deleted
   */
  void writeTestFile() {
    std::ofstream file(tempFile.c_str());

    file << "Fri 17-DEC-2010 15:47 Workspace: mantid\n";
    file << "  6 q (1/Angstrom)\n";
    file << "  6 q (1/Angstrom)\n";
    file << "  0 C++ no unit found\n";
    file << "  1\n";
    file << "\n";
    file << "  3\n";
    file << "-3.000000e-01 -2.900000e-01 -2.800000e-01\n";
    file << "  3\n";
    file << "-2.950000e-01 -2.850000e-01 -2.750000e-01\n";
    file << "   2   2  1.000000000000e+00\n";
    file << "  3(8E12.4)\n";
    file << "11.0000e+00  12.0000e+00\n";
    file << "21.0000e+00  22.0000e+00\n";
    file << "1 2 3 4\n";
    file.close();
  }

  void writeTestFileWithDx() {
    std::ofstream file(tempFile2.c_str());
    file << "        Thu 20-AUG-2015 08:30 Workspace: testInputThree\n";
    file << " \n";
    file << "   10    0    0    0    1   10    0\n";
    file << "         0         0         0         0\n";
    file << " 3 (F12.5,2E16.6)\n";
    file << "     0.50000    1.000000e+00    1.000000e+00    5.000000e-01\n";
    file << "     1.50000    1.000000e+00    1.000000e+00    1.207107e+00\n";
    file << "     2.50000    1.000000e+00    1.000000e+00    1.573132e+00\n";
    file << "     3.50000    1.000000e+00    1.000000e+00    1.866025e+00\n";
    file << "     4.50000    1.000000e+00    1.000000e+00    2.118034e+00\n";
    file << "     5.50000    1.000000e+00    1.000000e+00    2.342779e+00\n";
    file << "     6.50000    1.000000e+00    1.000000e+00    2.547621e+00\n";
    file << "     7.50000    1.000000e+00    1.000000e+00    2.737089e+00\n";
    file << "     8.50000    1.000000e+00    1.000000e+00    2.914214e+00\n";
    file << "     9.50000    1.000000e+00    1.000000e+00    3.081139e+00\n";
    file.close();
  }

  void writeTestFileWithSecondHeader() {
    std::ofstream file(tempFile3.c_str());
    file << " SANS2D Fri 08-JUL-2016 11:10 Workspace: "
            "M3_42_3rd-MT_rear_cloned_temp\n";
    file << " M3-42_third_SANS\n";
    file << "   4    0    0    0    1   4    0\n";
    file << "          0         0         0         0\n";
    file << " 3 (F12.5,2E16.6)\n";
    file << "     0.00520    1.055855e+00    2.570181e-01\n";
    file << "     0.00562    9.784999e-01    1.365013e-01\n";
    file << "     0.00607    1.239836e+00    9.582824e-02\n";
    file << "     0.00655    1.260936e+00    7.041577e-02\n";
    file.close();
  }
};

#endif // LOADRKHTEST_H_
