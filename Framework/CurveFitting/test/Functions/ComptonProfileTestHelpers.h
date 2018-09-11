#ifndef COMPTONPROFILETESTHELPERS_H_
#define COMPTONPROFILETESTHELPERS_H_

#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <random>

// Define helper functions to create test workspaces with appropriate
// instruments set up
namespace ComptonProfileTestHelpers {
enum class NoiseType { None = 0, Full = 1 };

// Forward declare all functions
static Mantid::API::MatrixWorkspace_sptr
createTestWorkspace(const size_t nhist, const double x0, const double x1,
                    const double dx, const NoiseType noise,
                    const bool singleMassSpectrum = false,
                    const bool addFoilChanger = false);
static Mantid::Geometry::Instrument_sptr
createTestInstrumentWithFoilChanger(const Mantid::detid_t id,
                                    const Mantid::Kernel::V3D &,
                                    const std::string &detShapeXML = "");
static Mantid::Geometry::Instrument_sptr
createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                      const Mantid::Kernel::V3D &,
                                      const std::string &detShape = "");
static void addResolutionParameters(const Mantid::API::MatrixWorkspace_sptr &ws,
                                    const Mantid::detid_t detID);
static void addFoilResolution(const Mantid::API::MatrixWorkspace_sptr &ws,
                              const std::string &name);

static Mantid::API::MatrixWorkspace_sptr
createTestWorkspace(const size_t nhist, const double x0, const double x1,
                    const double dx, const NoiseType noise,
                    const bool singleMassSpectrum, const bool addFoilChanger) {
  bool isHist(false);
  auto ws2d = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
      [](const double, std::size_t) { return 1.0; }, static_cast<int>(nhist),
      x0, x1, dx, isHist);
  ws2d->getAxis(0)->setUnit("TOF");
  if (singleMassSpectrum) {
    // Generate a test mass profile with some noise so any calculated spectrum
    // won't exactly match
    const double peakCentre(164.0), sigmaSq(16 * 16), peakHeight(0.2);
    for (size_t i = 0; i < nhist; ++i) {
      auto &dataXi = ws2d->mutableX(i);
      auto &dataYi = ws2d->mutableY(i);
      for (auto xit = std::begin(dataXi), yit = std::begin(dataYi);
           xit != std::end(dataXi); ++xit, ++yit) {
        *yit = peakHeight * exp(-0.5 * pow(*xit - peakCentre, 2.) / sigmaSq);
      }
    }
    if (noise == NoiseType::Full) {
      const double meanNoise(0.02);
      std::mt19937 rng(1);
      std::uniform_real_distribution<double> flat(0.0, 1.0);
      for (size_t i = 0; i < nhist; ++i) {
        auto &dataYi = ws2d->mutableY(i);
        for (auto &y : dataYi) {
          const double r(flat(rng));
          if (r > 0.5)
            y += r * meanNoise;
          else
            y -= r * meanNoise;
        }
      }
    }
  }

  Mantid::detid_t id(1);
  if (addFoilChanger) {
    const double r(0.553), theta(66.5993), phi(138.6);
    Mantid::Kernel::V3D detPos;
    detPos.spherical_rad(r, theta * M_PI / 180.0, phi * M_PI / 180.0);
    ws2d->setInstrument(createTestInstrumentWithFoilChanger(id, detPos));
  } else {
    double r(0.55), theta(66.5993), phi(0.0);
    Mantid::Kernel::V3D detPos;
    detPos.spherical_rad(r, theta * M_PI / 180.0, phi * M_PI / 180.0);
    ws2d->setInstrument(createTestInstrumentWithNoFoilChanger(id, detPos));
  }

  addResolutionParameters(ws2d, id);
  if (addFoilChanger) {
    addFoilResolution(ws2d, "foil-pos0");
    addFoilResolution(ws2d, "foil-pos1");
  }

  // Link workspace with detector
  Mantid::Indexing::IndexInfo indexInfo(nhist);
  Mantid::SpectrumDefinition specDef;
  specDef.add(0); // id 1
  indexInfo.setSpectrumDefinitions(
      std::vector<Mantid::SpectrumDefinition>(nhist, specDef));
  ws2d->setIndexInfo(indexInfo);

  return ws2d;
}

static Mantid::Geometry::Instrument_sptr
createTestInstrumentWithFoilChanger(const Mantid::detid_t id,
                                    const Mantid::Kernel::V3D &detPos,
                                    const std::string &detShapeXML) {
  using Mantid::Kernel::V3D;
  using namespace Mantid::Geometry;

  auto inst = createTestInstrumentWithNoFoilChanger(id, detPos, detShapeXML);
  // add changer
  auto changerShape = ComponentCreationHelper::createCappedCylinder(
      0.05, 0.4, V3D(0.0, -0.2, 0.0), V3D(0.0, 1, 0.0), "cylinder");
  auto *changer = new ObjComponent("foil-changer", changerShape);
  changer->setPos(V3D(0.0, 0.0, 0.0));
  inst->add(changer);

  // add single foil in position 0
  auto foilShape = ComponentCreationHelper::createCuboid(0.02);
  auto *foilPos0 = new ObjComponent("foil-pos0", foilShape);
  V3D pos0;
  pos0.spherical(0.225, -42, 0);
  foilPos0->setPos(pos0);
  inst->add(foilPos0);

  auto *foilPos1 = new ObjComponent("foil-pos1", foilShape);
  V3D pos1;
  pos1.spherical(0.225, -31, 0);
  foilPos1->setPos(pos1);
  inst->add(foilPos1);

  return inst;
}

static Mantid::Geometry::Instrument_sptr
createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                      const Mantid::Kernel::V3D &detPos,
                                      const std::string &detShapeXML) {
  using Mantid::Kernel::V3D;
  using namespace Mantid::Geometry;

  // Requires an instrument.
  auto inst = boost::make_shared<Instrument>();

  // Source/sample
  auto *source = new ObjComponent("source");
  source->setPos(V3D(0.0, 0.0, -11.005));
  inst->add(source);
  inst->markAsSource(source);
  auto *sampleHolder = new ObjComponent("samplePos");
  sampleHolder->setPos(V3D(0.0, 0.0, 0.0));
  inst->add(sampleHolder);
  inst->markAsSamplePos(sampleHolder);

  // Just give it a single detector
  Detector *det0(nullptr);
  if (!detShapeXML.empty()) {
    auto shape = ShapeFactory().createShape(detShapeXML);
    det0 = new Detector("det0", id, shape, nullptr);
  } else {
    det0 = new Detector("det0", id, nullptr);
  }
  det0->setPos(detPos);
  inst->add(det0);
  inst->markAsDetector(det0);

  return inst;
}

static void addResolutionParameters(const Mantid::API::MatrixWorkspace_sptr &ws,
                                    const Mantid::detid_t detID) {
  // Parameters
  auto &pmap = ws->instrumentParameters();
  const auto &detectorInfo = ws->detectorInfo();
  const auto detIndex = detectorInfo.indexOf(detID);
  const auto compID = detectorInfo.detector(detIndex).getComponentID();

  pmap.addDouble(compID, "sigma_l1", 0.021);
  pmap.addDouble(compID, "sigma_l2", 0.023);
  pmap.addDouble(compID, "sigma_theta", 0.028);
  pmap.addDouble(compID, "efixed", 4908);
  pmap.addDouble(compID, "t0", -0.32);
  pmap.addDouble(compID, "hwhm_lorentz", 24);
  pmap.addDouble(compID, "sigma_gauss", 73);
  pmap.addDouble(compID, "sigma_tof", 0.3);
}

static void addFoilResolution(const Mantid::API::MatrixWorkspace_sptr &ws,
                              const std::string &name) {
  // Parameters
  auto &pmap = ws->instrumentParameters();
  auto comp = ws->getInstrument()->getComponentByName(name);
  auto compID = comp->getComponentID();

  pmap.addDouble(compID, "hwhm_lorentz", 144);
  pmap.addDouble(compID, "sigma_gauss", 20);
}
} // namespace ComptonProfileTestHelpers

#endif /* COMPTONPROFILETESTHELPERS_H_ */
