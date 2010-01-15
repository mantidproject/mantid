#include "MantidAlgorithms/MultiplyRange.h"

namespace Mantid
{
namespace Algorithms
{

// Algorithm must be declared
DECLARE_ALGORITHM(MultiplyRange)

using namespace Kernel;
using namespace API;

void MultiplyRange::init()
{
  // Declare an input workspace property.
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  // Declare an output workspace property.
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  // StartBin
  declareProperty("StartBin", 0, "Bin number to start from");
  // EndBin
  declareProperty("EndBin", 0, "Bin number to finish at");
  // factor
  declareProperty("Factor", 0.0, "Multiplier");
}

/** Executes the algorithm
 */
void MultiplyRange::exec()
{
  g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl;

  // Get the input workspace
  MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
  StartBin = getProperty("StartBin");
  EndBin = getProperty("EndBin");
  Factor = getProperty("Factor");

  // make output Workspace the same type and size as the input one
  MatrixWorkspace_sptr outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create(inputW));

  // Get the count of histograms in the input workspace
  int histogramCount = inputW->getNumberHistograms();
  // Loop over spectra
  for (int i = 0; i < histogramCount; ++i)
  {
    outputW->dataX(i) = inputW->dataX(i);

    // Retrieve the data into a vector
    const MantidVec& YValues = inputW->readY(i);
    const MantidVec& EValues = inputW->readE(i);
    MantidVec& newY = outputW->dataY(i);
    MantidVec& newE = outputW->dataE(i);

    int j=0;
    // Iterate over i-th spectrum and modify the data
    for(j=0;j<inputW->blocksize();j++)
    {
      if (j >= StartBin && j <= EndBin)
      {
        newY[j] = YValues[j]*Factor;
        newE[j] = EValues[j]*Factor;
      }
      else
      {
        newY[j] = YValues[j];
        newE[j] = EValues[j];
      }
    }

  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputW);
}

}
}

