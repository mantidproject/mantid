#include "MantidAlgorithms/WeightedMean.h"

namespace Mantid
{
namespace Algorithms
{

// Algorithm must be declared
DECLARE_ALGORITHM(WeightedMean)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
*/
void WeightedMean::init()
{
  // Declare a 2D input workspace property.
  declareProperty(new WorkspaceProperty<>("InputWorkspace1","",Direction::Input));

  // Declare a 2D input workspace property.
  declareProperty(new WorkspaceProperty<>("InputWorkspace2","",Direction::Input));

  // Declare a 2D output workspace property.
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

/** Executes the algorithm
 */
void WeightedMean::exec()
{
  g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl;

  // Get the input workspace2
  MatrixWorkspace_const_sptr inputW1 = getProperty("InputWorkspace1");
  MatrixWorkspace_const_sptr inputW2 = getProperty("InputWorkspace2");

  // make output Workspace the same type and size as the input one
  MatrixWorkspace_sptr outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create(inputW1));

  // Get the count of histograms in the input workspace
  int histogramCount = inputW1->getNumberHistograms();
  // Loop over spectra
  for (int i = 0; i < histogramCount; ++i)
  {
    // Retrieve the data into a vector
    const MantidVec& XValues1 = inputW1->readX(i);
    const MantidVec& YValues1 = inputW1->readY(i);
    const MantidVec& EValues1 = inputW1->readE(i);
    const MantidVec& XValues2 = inputW2->readX(i);
    const MantidVec& YValues2 = inputW2->readY(i);
    const MantidVec& EValues2 = inputW2->readE(i);
    MantidVec& newX = outputW->dataX(i);
    MantidVec& newY = outputW->dataY(i);
    MantidVec& newE = outputW->dataE(i);

    int j=0;
    double err1,err2;
    // Iterate over i-th spectrum and modify the data
    for(j=0;j<inputW1->blocksize();j++)
    {
      newX[j] = XValues1[j];
      if (EValues1[j] > 0.0 && EValues2[j] > 0.0)
      {
        err1=EValues1[j]*EValues1[j];
        err2=EValues2[j]*EValues2[j];
        newY[j] = (YValues1[j]/err1)+(YValues2[j]/err2);
        newE[j] = 1.0/((1.0/err1)+(1.0/err2));
        newY[j] *= newE[j];
        newE[j]=sqrt(newE[j]); 
      }
      else if (EValues1[j] > 0.0 && EValues2[j] <= 0.0)
      {
        newY[j]=YValues1[j];
        newE[j]=EValues1[j];
      }
      else if (EValues1[j] <= 0.0 && EValues2[j] > 0.0)
      {
        newY[j]=YValues2[j];
        newE[j]=EValues2[j];
      }
      else
      {
        newY[j]=0.0;
        newE[j]=0.0;
      }
    }
    newX[j] = XValues1[j];

    }

    // Assign it to the output workspace property
    setProperty("OutputWorkspace",outputW);
}

} // namespace Algorithms
} // namespace Mantid
