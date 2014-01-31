/*WIKI*


Convolution of two workspaces using [[Convolution]] from CurveFitting.  Workspaces must have the same number of spectra.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ConvolveWorkspaces.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidCurveFitting/SplineWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <sstream>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvolveWorkspaces)

/// Sets documentation strings for this algorithm
void ConvolveWorkspaces::initDocs()
{
  this->setWikiSummary("Convolution of two workspaces. ");
  this->setOptionalMessage("Convolution of two workspaces.");
}

/// Constructor
ConvolveWorkspaces::ConvolveWorkspaces() : API::Algorithm(), prog(NULL)
{}

/// Virtual destructor
ConvolveWorkspaces::~ConvolveWorkspaces()
{
  delete prog;
}

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;


void ConvolveWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace1","",Direction::Input), "The name of the first input workspace.");
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace2","",Direction::Input), "The name of the second input workspace.");

  declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output), "The name of the output workspace.");



}

void ConvolveWorkspaces::exec()
{
  Workspace2D_sptr ws1 = getProperty("Workspace1");
  Workspace2D_sptr ws2 = getProperty("Workspace2");

  // Cache a few things for later use
  const size_t numHists = ws1->getNumberHistograms();
  const size_t numBins = ws1->blocksize();
  Workspace2D_sptr outputWS = boost::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D",numHists,numBins,numBins-1));

  WorkspaceFactory::Instance().initializeFromParent(ws1, outputWS, true);
  // First check that the workspace are the same size
  if ( numHists != ws2->getNumberHistograms()  )
  {
	throw std::runtime_error("Size mismatch");
  }

  prog = new Progress(this, 0.0, 1.0, numHists);
  // Now check the data itself
  PARALLEL_FOR3(ws1, ws2, outputWS)
  for ( int l = 0; l < static_cast<int>(numHists); ++l )
  {
    PARALLEL_START_INTERUPT_REGION
    prog->report();
    const MantidVec& X1 = ws1->readX(l);
    MantidVec& x = outputWS->dataX(l);
    x = X1;
    MantidVec& Yout = outputWS->dataY(l);
    Convolution conv;

    boost::shared_ptr<SplineWorkspace> res( new SplineWorkspace );
    size_t N = Yout.size();
    res->setMatrixWorkspace(ws1,l,x[0],x[N]);

    conv.addFunction(res);

    boost::shared_ptr<SplineWorkspace> fun( new SplineWorkspace );
    fun->setMatrixWorkspace(ws2,l,x[0],x[N]);

    conv.addFunction(fun);

    FunctionDomain1DView xView(&x[0],N);
    FunctionValues out(xView);
    conv.function(xView,out);

    for(size_t i=0;i<N;i++)
    {
      Yout[i] = out.getCalculated(i);

    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWS);
  return;
}


} // namespace Algorithms
} // namespace Mantid

