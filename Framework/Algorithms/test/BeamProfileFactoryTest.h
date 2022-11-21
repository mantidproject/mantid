// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cxxtest/TestSuite.h>

class BeamProfileFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamProfileFactoryTest *createSuite() { return new BeamProfileFactoryTest(); }
  static void destroySuite(BeamProfileFactoryTest *suite) { delete suite; }

  void test_Beam_Height_Calculation_With_Offset_Sample() {
    using namespace Mantid::Kernel;
    using namespace Mantid::Algorithms;
    using namespace Mantid::Geometry;
    // Define a sample shape
    constexpr double sampleRadius{0.006};
    constexpr double sampleHeight{0.04};
    const V3D sampleBaseCentre{0., sampleHeight / 2., 0.};
    const V3D yAxis{0., 1., 0.};
    auto sampleShape = ComponentCreationHelper::createCappedCylinder(sampleRadius, sampleHeight, sampleBaseCentre,
                                                                     yAxis, "sample-cylinder");
    auto instrument = std::make_shared<Mantid::Geometry::Instrument>("test");
    instrument->setReferenceFrame(std::make_shared<ReferenceFrame>(Y, Z, Right, ""));
    ObjComponent *source = new ObjComponent("moderator");
    source->setPos(V3D(0.0, 0.0, -20.0));
    instrument->add(source);
    instrument->markAsSource(source);
    Mantid::DataObjects::Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(5, 5);
    ws->mutableSample().setShape(sampleShape);
    std::shared_ptr<IBeamProfile> beam;
    TS_ASSERT_THROWS_NOTHING(beam = BeamProfileFactory::createBeamProfile(*instrument, ws->sample()));
    TS_ASSERT(std::dynamic_pointer_cast<RectangularBeamProfile>(beam)->maxPoint()[1] == 0.06);
  }
};
