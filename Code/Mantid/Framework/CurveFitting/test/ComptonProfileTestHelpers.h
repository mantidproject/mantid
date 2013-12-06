#ifndef COMPTONPROFILETESTHELPERS_H_
#define COMPTONPROFILETESTHELPERS_H_

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/MersenneTwister.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

// Define helper functions to create test workspaces with appropriate instruments set up
namespace ComptonProfileTestHelpers
{
  // Forward declare all functions
  static Mantid::API::MatrixWorkspace_sptr createSingleSpectrumWorkspaceOfOnes(const double x0, const double x1, const double dx);
  static Mantid::API::MatrixWorkspace_sptr createSingleSpectrumWorkspaceWithSingleMass(const double x0, const double x1, const double dx);
  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithFoilChanger(const Mantid::detid_t id);
  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                                                                 const Mantid::Kernel::V3D &);
  static Mantid::API::MatrixWorkspace_sptr createSingleSpectrumWorkspaceOfOnes(const double x0, const double x1, const double dx);
  static void addResolutionParameters(const Mantid::API::MatrixWorkspace_sptr & ws,
                                      const Mantid::detid_t detID);
  static void addFoilResolution(const Mantid::API::MatrixWorkspace_sptr & ws,
                                const std::string & name);

  struct ones
  {
    double operator()(const double, size_t) { return 1.0; } // don't care about Y values, just use 1.0 everywhere
  };

  static Mantid::API::MatrixWorkspace_sptr createSingleSpectrumWorkspaceOfOnes(const double x0, const double x1, const double dx)
  {
    int nhist(1);
    bool isHist(false);

    auto ws2d = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(ones(), nhist, x0,x1,dx,isHist);
    ws2d->getAxis(0)->setUnit("TOF");

    Mantid::detid_t id(1);

    double r(0.55), theta(66.5993), phi(0.0);
    Mantid::Kernel::V3D detPos;
    detPos.spherical(r, theta, phi);
    ws2d->setInstrument(createTestInstrumentWithNoFoilChanger(id,detPos));
    addResolutionParameters(ws2d, id);

    // Link workspace with detector
    auto *spec0 = ws2d->getSpectrum(0);
    spec0->setSpectrumNo(1);
    spec0->clearDetectorIDs();
    spec0->addDetectorID(id);

    return ws2d;
  }

  static Mantid::API::MatrixWorkspace_sptr
  createSingleSpectrumWorkspaceWithSingleMass(const double x0, const double x1, const double dx)
  {
    int nhist(1);
    bool isHist(false);

    auto ws2d = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(ones(), nhist, x0,x1,dx,isHist);
    ws2d->getAxis(0)->setUnit("TOF");
    const size_t nvalues = ws2d->blocksize();
    // Generate a test mass profile with some noise so any calculated spectrum won't exactly match
    const double peakCentre(164.0), sigmaSq(16*16), peakHeight(0.2);
    const double noise(0.02);
    Mantid::Kernel::MersenneTwister mt1998(123456);
    for(size_t i = 0; i < nvalues; ++i)
    {
      double x=  ws2d->dataX(0)[i];
      double y = peakHeight * exp(-0.5*pow(x - peakCentre, 2.)/sigmaSq);
      double r = mt1998.nextValue();
      if(r > 0.5) y += noise*r;
      else y -= noise*r;
      ws2d->dataY(0)[i] = y;
    }

    Mantid::detid_t id(1);
    ws2d->setInstrument(createTestInstrumentWithFoilChanger(id));
    addResolutionParameters(ws2d, id);
    addFoilResolution(ws2d, "foil-pos0");
    addFoilResolution(ws2d, "foil-pos1");

    // Link workspace with detector
    auto *spec0 = ws2d->getSpectrum(0);
    spec0->setSpectrumNo(1);
    spec0->clearDetectorIDs();
    spec0->addDetectorID(id);

    return ws2d;
  }

  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithFoilChanger(const Mantid::detid_t id)
  {
    using Mantid::Kernel::V3D;
    using namespace Mantid::Geometry;

    double r(0.553), theta(66.5993), phi(138.6);
    V3D detPos;
    detPos.spherical(r, theta, phi);
    auto inst = createTestInstrumentWithNoFoilChanger(id, detPos);
    // add changer
    auto changerShape = ComponentCreationHelper::createCappedCylinder(0.05,0.4,V3D(0.0,-0.2,0.0),V3D(0.0,1,0.0), "cylinder");
    auto *changer = new ObjComponent("foil-changer",changerShape);
    changer->setPos(V3D(0.0,0.0,0.0));
    inst->add(changer);

    //add single foil in position 0
    auto foilShape = ComponentCreationHelper::createCuboid(0.02);
    auto *foilPos0 = new ObjComponent("foil-pos0",foilShape);
    V3D pos0;
    pos0.spherical(0.225,-42,0);
    foilPos0->setPos(pos0);
    inst->add(foilPos0);

    auto *foilPos1 = new ObjComponent("foil-pos1",foilShape);
    V3D pos1;
    pos1.spherical(0.225,-31,0);
    foilPos1->setPos(pos1);
    inst->add(foilPos1);

    return inst;
  }

  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                                                                 const Mantid::Kernel::V3D & detPos)
  {
    using Mantid::Kernel::V3D;
    using namespace Mantid::Geometry;

    // Requires an instrument.
    auto inst = boost::make_shared<Instrument>();

    // Source/sample
    auto *source = new ObjComponent("source");
    source->setPos(V3D(0.0,0.0,-11.005));
    inst->add(source);
    inst->markAsSource(source);
    auto *sampleHolder = new ObjComponent("samplePos");
    sampleHolder->setPos(V3D(0.0,0.0,0.0));
    inst->add(sampleHolder);
    inst->markAsSamplePos(sampleHolder);

    //Just give it a single detector
    auto *det0 = new Detector("det0",id,NULL);
    det0->setPos(detPos);
    inst->add(det0);
    inst->markAsDetector(det0);

    return inst;
  }

  static void addResolutionParameters(const Mantid::API::MatrixWorkspace_sptr & ws,
                                      const Mantid::detid_t detID)
  {
    // Parameters
    auto & pmap = ws->instrumentParameters();
    auto det0 = ws->getInstrument()->getDetector(detID);
    auto compID = det0->getComponentID();

    pmap.addDouble(compID, "sigma_l1", 0.021);
    pmap.addDouble(compID, "sigma_l2", 0.023);
    pmap.addDouble(compID, "sigma_theta", 0.028);
    pmap.addDouble(compID, "efixed", 4908);
    pmap.addDouble(compID, "t0", -0.32);
    pmap.addDouble(compID, "hwhm_lorentz", 24);
    pmap.addDouble(compID, "sigma_gauss", 73);
  }

  static void addFoilResolution(const Mantid::API::MatrixWorkspace_sptr & ws,
                                const std::string & name)
  {
    // Parameters
    auto & pmap = ws->instrumentParameters();
    auto comp = ws->getInstrument()->getComponentByName(name);
    auto compID = comp->getComponentID();

    pmap.addDouble(compID, "hwhm_lorentz", 144);
    pmap.addDouble(compID, "sigma_gauss", 20);
  }
}


#endif /* COMPTONPROFILETESTHELPERS_H_ */
