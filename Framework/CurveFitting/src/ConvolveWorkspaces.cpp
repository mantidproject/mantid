//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ConvolveWorkspaces.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidCurveFitting/TabulatedFunction.h"
#include <sstream>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

namespace Mantid {
namespace CurveFitting {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvolveWorkspaces)

/// Constructor
ConvolveWorkspaces::ConvolveWorkspaces() : API::Algorithm(), prog(NULL) {}

/// Virtual destructor
ConvolveWorkspaces::~ConvolveWorkspaces() { delete prog; }

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;

void ConvolveWorkspaces::init() {
  declareProperty(
      new WorkspaceProperty<Workspace2D>("Workspace1", "", Direction::Input),
      "The name of the first input workspace.");
  declareProperty(
      new WorkspaceProperty<Workspace2D>("Workspace2", "", Direction::Input),
      "The name of the second input workspace.");

  declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "",
                                                     Direction::Output),
                  "The name of the output workspace.");
}

void ConvolveWorkspaces::exec() {
  std::string ws1name = getProperty("Workspace1");
  std::string ws2name = getProperty("Workspace2");
  Workspace2D_sptr ws1 = getProperty("Workspace1");
  Workspace2D_sptr ws2 = getProperty("Workspace2");

  // Cache a few things for later use
  const size_t numHists = ws1->getNumberHistograms();
  const size_t numBins = ws1->blocksize();
  Workspace2D_sptr outputWS = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create(ws1, numHists, numBins + 1, numBins));

  // First check that the workspace are the same size
  if (numHists != ws2->getNumberHistograms()) {
    throw std::runtime_error("Size mismatch");
  }

  prog = new Progress(this, 0.0, 1.0, numHists);
  // Now convolve the histograms
  PARALLEL_FOR3(ws1, ws2, outputWS)
  for (int l = 0; l < static_cast<int>(numHists); ++l) {
    PARALLEL_START_INTERUPT_REGION
    prog->report();
    const MantidVec &X1 = ws1->readX(l);
    MantidVec &x = outputWS->dataX(l);
    x = X1;
    MantidVec &Yout = outputWS->dataY(l);
    Convolution conv;

    auto res = boost::make_shared<TabulatedFunction>();
    res->setAttributeValue("Workspace", ws1name);
    res->setAttributeValue("WorkspaceIndex", l);

    conv.addFunction(res);

    auto fun = boost::make_shared<TabulatedFunction>();
    fun->setAttributeValue("Workspace", ws2name);
    fun->setAttributeValue("WorkspaceIndex", l);

    conv.addFunction(fun);
    size_t N = Yout.size();
    FunctionDomain1DView xView(&x[0], N);
    FunctionValues out(xView);
    conv.function(xView, out);

    for (size_t i = 0; i < N; i++) {
      Yout[i] = out.getCalculated(i);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);
  return;
}

} // namespace Algorithms
} // namespace Mantid
