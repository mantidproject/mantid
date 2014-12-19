#ifndef COMPTONPROFILETESTHELPERS_H_
#define COMPTONPROFILETESTHELPERS_H_

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/MersenneTwister.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

// Define helper functions to create test workspaces with appropriate instruments set up
namespace ComptonProfileTestHelpers
{
  // Forward declare all functions
  static Mantid::API::MatrixWorkspace_sptr createTestWorkspace(const size_t nhist,const double x0, const double x1,
                                                               const double dx, const bool singleMassSpectrum = false,
                                                               const bool addFoilChanger = false);
  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithFoilChanger(const Mantid::detid_t id,
                                                                               const Mantid::Kernel::V3D &,
                                                                               const std::string &detShapeXML = "");
  static Mantid::Geometry::Instrument_sptr createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                                                                 const Mantid::Kernel::V3D &,
                                                                                 const std::string & detShape = "");
  static void addResolutionParameters(const Mantid::API::MatrixWorkspace_sptr & ws,
                                      const Mantid::detid_t detID);
  static void addFoilResolution(const Mantid::API::MatrixWorkspace_sptr & ws,
                                const std::string & name);

  struct ones
  {
    double operator()(const double, size_t) { return 1.0; } // don't care about Y values, just use 1.0 everywhere
  };

  static Mantid::API::MatrixWorkspace_sptr
  createTestWorkspace(const size_t nhist, const double x0, const double x1, const double dx,
                      const bool singleMassSpectrum,const bool addFoilChanger)
  {
    bool isHist(false);
    auto ws2d = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(ones(), static_cast<int>(nhist), x0,x1,dx,isHist);
    ws2d->getAxis(0)->setUnit("TOF");
    if(singleMassSpectrum)
    {
      const size_t nvalues = ws2d->blocksize();
      // Generate a test mass profile with some noise so any calculated spectrum won't exactly match
      const double peakCentre(164.0), sigmaSq(16*16), peakHeight(0.2);
      const double noise(0.02);
      Mantid::Kernel::MersenneTwister mt1998(123456);
      for(size_t i = 0; i < nhist; ++i)
      {
        for(size_t j = 0; j < nvalues; ++j)
        {
          double x=  ws2d->dataX(i)[j];
          double y = peakHeight * exp(-0.5*pow(x - peakCentre, 2.)/sigmaSq);
          double r = mt1998.nextValue();
          if(r > 0.5) y += noise*r;
          else y -= noise*r;
          ws2d->dataY(i)[j] = y;
        }
      }
    }

    Mantid::detid_t id(1);
    if(addFoilChanger)
    {
      double r(0.553), theta(66.5993), phi(138.6);
      Mantid::Kernel::V3D detPos;
      detPos.spherical_rad(r, theta*M_PI/180.0, phi*M_PI/180.0);
      ws2d->setInstrument(createTestInstrumentWithFoilChanger(id,detPos));
    }
    else
    {
      double r(0.55), theta(66.5993), phi(0.0);
      Mantid::Kernel::V3D detPos;
      detPos.spherical_rad(r, theta*M_PI/180.0, phi*M_PI/180.0);
      ws2d->setInstrument(createTestInstrumentWithNoFoilChanger(id,detPos));
    }

    addResolutionParameters(ws2d, id);
    if(addFoilChanger)
    {
      addFoilResolution(ws2d, "foil-pos0");
      addFoilResolution(ws2d, "foil-pos1");
    }

    // Link workspace with detector
    for(size_t i = 0; i < nhist; ++i)
    {
      const Mantid::specid_t specID = static_cast<Mantid::specid_t>(id + i);
      auto *spec = ws2d->getSpectrum(i);
      spec->setSpectrumNo(specID);
      spec->clearDetectorIDs();
      spec->addDetectorID(id);
    }
    return ws2d;
  }

  static Mantid::Geometry::Instrument_sptr
  createTestInstrumentWithFoilChanger(const Mantid::detid_t id,
                                      const Mantid::Kernel::V3D & detPos,
                                      const std::string &detShapeXML)
  {
    using Mantid::Kernel::V3D;
    using namespace Mantid::Geometry;

    auto inst = createTestInstrumentWithNoFoilChanger(id, detPos, detShapeXML);
    // add changer
    auto changerShape = \
        ComponentCreationHelper::createCappedCylinder(0.05, 0.4, V3D(0.0,-0.2,0.0),
                                                      V3D(0.0,1,0.0), "cylinder");
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

  static Mantid::Geometry::Instrument_sptr
  createTestInstrumentWithNoFoilChanger(const Mantid::detid_t id,
                                        const Mantid::Kernel::V3D & detPos,
                                        const std::string &detShapeXML)
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
    Detector *det0(NULL);
    if(!detShapeXML.empty())
    {
      auto shape = ShapeFactory().createShape(detShapeXML);
      det0 = new Detector("det0", id, shape, NULL);
    }
    else
    {
      det0 = new Detector("det0", id, NULL);
    }
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
    pmap.addDouble(compID, "sigma_tof", 0.3);
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
