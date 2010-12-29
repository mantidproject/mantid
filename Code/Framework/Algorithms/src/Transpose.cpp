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

