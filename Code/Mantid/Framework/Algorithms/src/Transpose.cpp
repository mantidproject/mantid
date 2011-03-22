//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Transpose.h"

#include "MantidAPI/NumericAxis.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Transpose)

/// Sets documentation strings for this algorithm
void Transpose::initDocs()
{
  this->setWikiSummary("This algorithm transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.  The X-vector-values for the new workspace are taken from the axis value of the old workspace, which is generaly the spectra number but can be other values, if say the workspace has gone through [[ConvertSpectrumAxis]].  The new axis values are taken from the previous X-vector-values for the ''first specrum'' in the workspace. For this reason, use with [[Ragged Workspace|ragged workspaces]] is undefined.  The output workspace is a data point workspace, not histogram data. ");
  this->setOptionalMessage("This algorithm transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.  The X-vector-values for the new workspace are taken from the axis value of the old workspace, which is generaly the spectra number but can be other values, if say the workspace has gone through ConvertSpectrumAxis.  The new axis values are taken from the previous X-vector-values for the 'first specrum' in the workspace. For this reason, use with ragged workspaces is undefined.  The output workspace is a data point workspace, not histogram data.");
}


using namespace Kernel;
using namespace API;

void Transpose::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),"The input workspace.");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The output workspace.");
}

void Transpose::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  const int numHists = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,numBins,numHists,numHists);
  Mantid::API::Axis* inputAxis;
  try
  {
    inputAxis = inputWorkspace->getAxis(1);
  }
  catch ( Kernel::Exception::IndexError&)
  {
    g_log.error() << "Axis(1) not found on input workspace." << std::endl;
    throw std::runtime_error("Axis(1) not found on input workspace.");
  }
  Mantid::API::NumericAxis* newAxis = new NumericAxis(numBins);
  Progress prog(this,0.0,1.0,numBins);
  for ( int j=0; j < numBins; ++j )
  {
    for (int i = 0; i < numHists; ++i)
    {
      outputWorkspace->dataX(j)[i] = inputAxis->operator()(i);
      outputWorkspace->dataY(j)[i] = inputWorkspace->readY(i)[j];
      outputWorkspace->dataE(j)[i] = inputWorkspace->readE(i)[j];
    }
    double axisValue;
    if ( inputWorkspace->isHistogramData() )
    {
      axisValue = (inputWorkspace->readX(0)[j] + inputWorkspace->readX(0)[j+1])/2;
    }
    else
    {
      axisValue = inputWorkspace->readX(0)[j];
    }
    newAxis->setValue(j, axisValue);
    prog.report();
  }
  newAxis->unit() = inputWorkspace->getAxis(0)->unit();
  outputWorkspace->getAxis(0)->unit() = inputWorkspace->getAxis(1)->unit();
  outputWorkspace->replaceAxis(1, newAxis);
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid

