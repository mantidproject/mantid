#ifndef MANTID_ALGORITHMS_REFLECTOMETRYQRESOLUTIONTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYQRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryQResolution.h"

#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;

class ReflectometryQResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryQResolutionTest *createSuite() { return new ReflectometryQResolutionTest(); }
  static void destroySuite( ReflectometryQResolutionTest *suite ) { delete suite; }


  void test_Init() {
    Algorithms::ReflectometryQResolution alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    auto inputWS = make_ws();
    API::MatrixWorkspace_sptr directWS = inputWS->clone();
    std::vector<int> foreground(2);
    foreground.front() = 1;
    foreground.back() = 1;
    constexpr double pixelSize{1.5};
    constexpr double detectorResolution{2.};
    constexpr double chopperSpeed{990.};
    constexpr double chopperOpeningAngle{33.};
    constexpr double chopperpairDist{0.23};
    Algorithms::ReflectometryQResolution alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DirectBeamWorkspace", directWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Foreground", foreground))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummationType", "SumInLambda"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Polarized", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", pixelSize))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorResolution", detectorResolution))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperSpeed", chopperSpeed))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperOpening", chopperOpeningAngle))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperpairDistance", chopperpairDist))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Slit1Name", "slit1"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Slit1SizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Slit2Name", "slit2"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Slit2SizeSampleLog", "slit2.size"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TOFChannelWidth", 20.))
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
  }

private:
  API::MatrixWorkspace_sptr make_ws() {
    using namespace WorkspaceCreationHelper;
    constexpr double startX{0.};
    const Kernel::V3D slit1Pos{0., 0., -0.3};
    const Kernel::V3D slit2Pos{0., 0., -1.2};
    constexpr double slit1Size{0.03};
    constexpr double slit2Size{0.02};
    const Kernel::V3D sourcePos{0., 0., -8.};
    const Kernel::V3D &monitorPos = sourcePos;
    const Kernel::V3D samplePos{0., 0., 0.,};
    const Kernel::V3D detectorPos{0., 0., 4.};
    constexpr int nHisto{2};
    constexpr int nBins{100};
    constexpr double binWidth{20.};
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
          startX, slit1Pos, slit2Pos, slit1Size, slit2Size, sourcePos, monitorPos, samplePos, detectorPos,
          nHisto, nBins, binWidth);
    // Add slit sizes to sample logs, too.
    auto &run = ws->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", slit1Size, meters, overwrite);
    run.addProperty("slit2.size", slit2Size, meters, overwrite);
    Algorithms::ConvertUnits toWavelength;
    toWavelength.initialize();
    toWavelength.setChild(true);
    toWavelength.setRethrows(true);
    toWavelength.setProperty("InputWorkspace", ws);
    toWavelength.setPropertyValue("OutputWorkspace", "_unused_for_child");
    toWavelength.setProperty("Target", "Wavelength");
    toWavelength.setProperty("EMode", "Elastic");
    toWavelength.execute();
    return toWavelength.getProperty("OutputWorkspace");
  }
};


#endif /* MANTID_ALGORITHMS_REFLECTOMETRYQRESOLUTIONTEST_H_ */
