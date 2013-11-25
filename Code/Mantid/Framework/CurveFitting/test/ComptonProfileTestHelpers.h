#ifndef COMPTONPROFILETESTHELPERS_H_
#define COMPTONPROFILETESTHELPERS_H_

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

// Define helper functions to create test workspaces with appropriate instruments set up
namespace ComptonProfileTestHelpers
{
  struct ones
  {
    double operator()(const double, size_t) { return 1.0; } // don't care about Y values, just use 1.0 everywhere
  };

  static Mantid::API::MatrixWorkspace_sptr createSingleSpectrumTestWorkspace(const double x0, const double x1, const double dx)
  {
    using Mantid::Kernel::V3D;
    using namespace Mantid::Geometry;

    int nhist(1);
    bool isHist(false);

    auto ws2d = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(ones(), nhist, x0,x1,dx,isHist);
    // Requires an instrument.
    auto inst = boost::make_shared<Instrument>();
    ws2d->setInstrument(inst);

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
    const Mantid::detid_t id(1);
    auto *det0 = new Detector("det0",id,NULL);
    double r(0.55), theta(66.5993), phi(0.0);
    V3D detPos;
    detPos.spherical(r, theta, phi);
    det0->setPos(detPos);
    inst->add(det0);
    inst->markAsDetector(det0);

    // Parameters
    ParameterMap & pmap = ws2d->instrumentParameters();
    pmap.addDouble(det0, "sigma_l1", 0.021);
    pmap.addDouble(det0, "sigma_l2", 0.023);
    pmap.addDouble(det0, "sigma_theta", 0.028);
    pmap.addDouble(det0, "efixed", 4908);
    pmap.addDouble(det0, "t0", -0.32);
    pmap.addDouble(det0, "hwhm_analyser_lorentz", 24);
    pmap.addDouble(det0, "sigma_analyser_gauss", 73);

    // Link workspace with detector
    auto *spec0 = ws2d->getSpectrum(0);
    spec0->setSpectrumNo(1);
    spec0->clearDetectorIDs();
    spec0->addDetectorID(id);

    return ws2d;

  }
}


#endif /* COMPTONPROFILETESTHELPERS_H_ */
