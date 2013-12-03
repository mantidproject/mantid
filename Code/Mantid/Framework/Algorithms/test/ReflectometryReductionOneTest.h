
#ifndef REFLECTOMETRYREDUCTIONONETEST_H_
#define REFLECTOMETRYREDUCTIONONETEST_H_

#include <cxxtest/TestSuite.h>
#include <algorithm>
#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Algorithms::ReflectometryReductionOne;

class ReflectometryReductionOneTest : public CxxTest::TestSuite
{
public:

  void test_tolam()
  {
    auto loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", "INTER00013460.nxs");
    loadAlg->setPropertyValue("OutputWorkspace", "demo");
    loadAlg->execute();

    MatrixWorkspace_sptr toConvert = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("demo");
    std::vector<int> detectorIndexRange;
    size_t workspaceIndexToKeep1 = 3;
    size_t workspaceIndexToKeep2 = 4;

    specid_t specId1 = toConvert->getSpectrum(workspaceIndexToKeep1)->getSpectrumNo();
    specid_t specId2 = toConvert->getSpectrum(workspaceIndexToKeep2)->getSpectrumNo();
    // Define one spectra to keep
    detectorIndexRange.push_back(workspaceIndexToKeep1);
    detectorIndexRange.push_back(workspaceIndexToKeep1);
    // Define another spectra to keep
    detectorIndexRange.push_back(workspaceIndexToKeep2);
    detectorIndexRange.push_back(workspaceIndexToKeep2);
    // Define a wavelength range for the detector workspace
    const double wavelengthMin = 10;
    const double wavelengthMax = 15;

    ReflectometryReductionOne alg;
    MatrixWorkspace_sptr inLam = alg.toLam(toConvert, detectorIndexRange, 0, boost::tuple<double, double>(wavelengthMin, wavelengthMax), boost::tuple<double, double>(0, 0));

    TS_ASSERT_EQUALS("Wavelength", inLam->getAxis(0)->unit()->unitID());

    // Check the number of spectrum kept.
    TS_ASSERT_EQUALS(2, inLam->getNumberHistograms());

    auto map = inLam->getSpectrumToWorkspaceIndexMap();
    // Check the spectrum ids retained.
    TS_ASSERT_EQUALS(map[specId1], 0);
    TS_ASSERT_EQUALS(map[specId2], 1);

    // Check the cropped x range
    Mantid::MantidVec copyX = inLam->readX(0);
    std::sort(copyX.begin(), copyX.end());
    TS_ASSERT(copyX.front() >= wavelengthMin);
    TS_ASSERT(copyX.back() <= wavelengthMax);

  }

};



#endif /* REFLECTOMETRYREDUCTIONONETEST_H_ */
