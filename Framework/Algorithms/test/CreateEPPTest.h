#ifndef MANTID_ALGORITHMS_CREATEEPPTEST_H_
#define MANTID_ALGORITHMS_CREATEEPPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateEPP.h"

#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CreateEPP;
using namespace Mantid;

class CreateEPPTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateEPPTest *createSuite() { return new CreateEPPTest(); }
  static void destroySuite(CreateEPPTest *suite) { delete suite; }

  void test_Init() {
    CreateEPP alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Height() {
    const int nSpectra = 1;
    const int nBins = 33;
    const double Ei = 13.7;
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nSpectra,
                                                                     nBins);
    const auto &spectrumInfo = inputWS->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    const auto l2 = spectrumInfo.l2(0);
    const auto elasticTOF = Kernel::UnitConversion::run(
        "Energy", "TOF", Ei, l1, l2, 0.0, Kernel::DeltaEMode::Direct, Ei);
    const double binWidth = 22.7;
    // Elastic bin will be in the centre of the spectrum.
    for (size_t binIndex = 0; binIndex < nBins + 1; ++binIndex) {
      const int shift = static_cast<int>(binIndex) - nBins / 2;
      inputWS->mutableX(0)[binIndex] = elasticTOF + shift * binWidth;
    }
    const double height = 667.0;
    inputWS->mutableY(0)[nBins / 2 - 1] = height;
    const bool overwrite = true;
    inputWS->mutableRun().addProperty<double>("Ei", Ei, overwrite);
    CreateEPP alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    API::ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getRef<double>("Height", 0), height)
  }

  void test_NormalOperation() {
    const int nSpectra = 3;
    const int nBins = 13;
    const double Ei = 42.7;
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nSpectra,
                                                                     nBins);
    const auto &spectrumInfo = inputWS->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    const auto l2 = spectrumInfo.l2(0);
    const auto elasticTOF = Kernel::UnitConversion::run(
        "Energy", "TOF", Ei, l1, l2, 0.0, Kernel::DeltaEMode::Direct, Ei);
    const double binWidth = 9.17;
    // Make sane bin borders.
    for (size_t wsIndex = 0; wsIndex < nSpectra; ++wsIndex) {
      for (size_t binIndex = 0; binIndex < nBins + 1; ++binIndex) {
        const int shift = static_cast<int>(binIndex) - nBins / 2;
        inputWS->mutableX(wsIndex)[binIndex] = elasticTOF + shift * binWidth;
      }
    }
    const bool overwrite = true;
    inputWS->mutableRun().addProperty<double>("Ei", Ei, overwrite);
    CreateEPP alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    API::ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->rowCount(), nSpectra);
    const auto columnNames = outputWS->getColumnNames();
    TS_ASSERT(hasCorrectColumns(columnNames))
    for (size_t col = 0; col < 9; ++col) {
      const auto &colName = columnNames[col];
      for (size_t row = 0; row < nSpectra; ++row) {
        if (colName == "WorkspaceIndex") {
          TS_ASSERT_EQUALS(outputWS->getRef<int>(colName, row), row);
        } else if (colName == "PeakCentre") {
          const double l2 = spectrumInfo.l2(row);
          const double peakCentre = Kernel::UnitConversion::run(
              "Energy", "TOF", Ei, l1, l2, 0.0, Kernel::DeltaEMode::Direct, Ei);
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), peakCentre)
        } else if (colName == "PeakCentreError") {
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 0.0)
        } else if (colName == "Sigma") {
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 0.0)
        } else if (colName == "SigmaError") {
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 0.0)
        } else if (colName == "Height") {
          // Height from create2dWorkspaceWithFullInstrument doxygen.
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 2.0)
        } else if (colName == "HeightError") {
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 0.0)
        } else if (colName == "chiSq") {
          TS_ASSERT_EQUALS(outputWS->getRef<double>(colName, row), 1.0)
        } else if (colName == "FitStatus") {
          TS_ASSERT_EQUALS(outputWS->getRef<std::string>(colName, row),
                           "success")
        } else {
          TS_FAIL("Unknown column name.");
        }
      }
    }
  }

  void test_SetSigma() {
    const int nSpectra = 1;
    const int nBins = 1;
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nSpectra,
                                                                     nBins);
    const bool overwrite = true;
    inputWS->mutableRun().addProperty<double>("Ei", 1.0, overwrite);
    const double sigma = 2.23;
    CreateEPP alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Sigma", sigma))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    API::ITableWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getRef<double>("Sigma", 0), sigma)
  }

private:
  bool hasCorrectColumns(const std::vector<std::string> &names) {
    const size_t nNames = 9;
    const std::array<std::string, 9> columnNames = {
        {"WorkspaceIndex", "PeakCentre", "PeakCentreError", "Sigma",
         "SigmaError", "Height", "HeightError", "chiSq", "FitStatus"}};
    if (names.size() != nNames) {
      return false;
    }
    for (const auto &columnName : columnNames) {
      if (std::find(names.cbegin(), names.cend(), columnName) == names.end()) {
        return false;
      }
    }
    return true;
  }
};

#endif /* MANTID_ALGORITHMS_CREATEEPPTEST_H_ */
