// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/SaveNexusGeometry.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/filesystem.hpp>
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::SaveNexusGeometry;
using MatrixWorkspace_sptr = boost::shared_ptr<Mantid::API::MatrixWorkspace>;
using Workspace_sptr = boost::shared_ptr<Mantid::API::Workspace>;

class ScopedFileHandle {

public:
  ScopedFileHandle(const std::string &fileName) {

    const auto temp_dir = boost::filesystem::temp_directory_path();
    auto temp_full_path = temp_dir;
    // append full path to temp directory to user input file name
    temp_full_path /= fileName;

    // Check proposed location and throw std::invalid argument if file does
    // not exist. otherwise set m_full_path to location.

    if (boost::filesystem::is_directory(temp_dir)) {
      m_full_path = temp_full_path;

    } else {
      throw std::invalid_argument("failed to load temp directory: " +
                                  temp_dir.generic_string());
    }
  }

  std::string fullPath() const { return m_full_path.generic_string(); }

  ~ScopedFileHandle() {

    // file is removed at end of file handle's lifetime
    if (boost::filesystem::is_regular_file(m_full_path)) {
      boost::filesystem::remove(m_full_path);
    }
  }

private:
  boost::filesystem::path m_full_path; // full path to file

  // prevent heap allocation for ScopedFileHandle
protected:
  static void *operator new(std::size_t); // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
};

class SaveNexusGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusGeometryTest *createSuite() {
    return new SaveNexusGeometryTest();
  }
  static void destroySuite(SaveNexusGeometryTest *suite) { delete suite; }

  void test_Init() {
    SaveNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    ScopedFileHandle fileResource("algorithm_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary
    Mantid::API::MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::
        create2DDetectorScanWorkspaceWithFullInstrument(1, 5, 1);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().add("testWS", inputWS));

    SaveNexusGeometry alg;
    // Don't put output in ADS by default

    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("H5Path", "algorithm_test_data"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }
};

#endif /* MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_ */