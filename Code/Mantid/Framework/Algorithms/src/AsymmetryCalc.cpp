/*WIKI*
This algorithm is used to calculate the asymmetry for a muon workspace. The asymmetry is given by:

:<math> Asymmetry = \frac{F-\alpha B}{F+\alpha B} </math>

where F is the front spectra, B is the back spectra and a is alpha.

The errors in F-aB and F+aB are calculated by adding the errors in F and B in quadrature; any errors in alpha are ignored. The errors for the asymmetry are then calculated using the fractional error method with the values for the errors in F-aB and F+aB.

The output workspace contains one set of data for the time of flight, the asymmetry and the asymmetry errors.

Note: this algorithm does not perform any grouping; the grouping must be done via the GroupDetectors algorithm or when the NeXus file is loaded auto_group must be set to true.

*WIKI*/
/*WIKI_USAGE*
'''Python'''
    OutWS = AsymmetryCalc("EmuData","1.0","0,1,2,3,4","16,17,18,19,20")

'''C++'''
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("AsymmetryCalc");
    alg->setPropertyValue("InputWorkspace", "EmuData");
    alg->setPropertyValue("OutputWorkspace", "OutWS");
    alg->setPropertyValue("Alpha", "1.0");
    alg->setPropertyValue("ForwardSpectra", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15");
    alg->setPropertyValue("BackwardSpectra", "16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31");
    alg->execute();
    Workspace* ws = FrameworkManager::Instance().getWorkspace("OutWS");

*WIKI_USAGE*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/AsymmetryCalc.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM( AsymmetryCalc)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void AsymmetryCalc::init()
{
  this->setWikiSummary("Calculates the asymmetry between two groups of detectors for a muon workspace.");
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
      Direction::Input), "Name of the input workspace");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
      Direction::Output), "The name of the workspace to be created as the output of the algorithm");

  declareProperty(new ArrayProperty<int> ("ForwardSpectra"),
      "The spectra numbers of the forward group");
  declareProperty(new ArrayProperty<int> ("BackwardSpectra"),
      "The spectra numbers of the backward group");
  declareProperty("Alpha", 1.0, "The balance parameter (default 1)", Direction::Input);
}

/** Executes the algorithm
 *
 */
void AsymmetryCalc::exec()
{
  std::vector<int> forward_list = getProperty("ForwardSpectra");
  std::vector<int> backward_list = getProperty("BackwardSpectra");
  int forward = forward_list.size() ? forward_list[0] : 1;
  int backward = backward_list.size() ? backward_list[0] : 2;
  double alpha = getProperty("Alpha");

  //Get original workspace
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  API::MatrixWorkspace_sptr tmpWS;
  if (forward_list.size() > 1 || backward_list.size() > 1)
  {
    API::IAlgorithm_sptr group = createChildAlgorithm("GroupDetectors");
    group->setProperty("InputWorkspace", inputWS);
    group->setProperty("SpectraList", backward_list);
    group->setProperty("KeepUngroupedSpectra", true);
    group->execute();
    tmpWS = group->getProperty("OutputWorkspace");

    group = createChildAlgorithm("GroupDetectors");
    group->setProperty("InputWorkspace", tmpWS);
    group->setProperty("SpectraList", forward_list);
    group->setProperty("KeepUngroupedSpectra", true);
    group->execute();
    tmpWS = group->getProperty("OutputWorkspace");

    forward = 0;
    backward = 1;
  }
  else
  {
    tmpWS = inputWS;
    // get workspace indices from spectra ids for forward and backward
    std::vector<specid_t> specIDs(2);
    specIDs[0] = forward;
    specIDs[1] = backward;
    std::vector<size_t> indices;
    tmpWS->getIndicesFromSpectra( specIDs, indices );

    // If some spectra were not found, can't continue
    if(specIDs.size() != indices.size())
      throw std::invalid_argument("Some of the spectra specified do not exist in a workspace");

    forward = static_cast<int>( indices[0] );
    backward = static_cast<int>( indices[1] );
  }

  //Create a workspace with only one spectra for forward
  API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(inputWS, 1,
                                                 inputWS->readX(0).size(),inputWS->blocksize());

  //Calculate asymmetry for each time bin
  //F-aB / F+aB
  Progress prog(this, 0.0, 1.0, tmpWS->blocksize());
  for (size_t j = 0; j < tmpWS->blocksize(); ++j)
  {
    // cal F-aB
    double numerator = tmpWS->dataY(forward)[j] - alpha * tmpWS->dataY(backward)[j];
    // cal F+aB
    double denominator = (tmpWS->dataY(forward)[j] + alpha * tmpWS->dataY(backward)[j]);

    // cal F-aB / F+aB
    if(denominator) 
    {
        outputWS->dataY(0)[j] = numerator / denominator;
    } 
    else
    {
        outputWS->dataY(0)[j] = 0.;
    }


    // Work out the error (as in 1st attachment of ticket #4188)
    double error = 1.0;
    if(denominator) {
      // cal F + a2B
      double q1 =  tmpWS->dataY(forward)[j] + alpha * alpha * tmpWS->dataY(backward)[j];
      // cal 1 + ((f-aB)/(F+aB))2 
      double q2 = 1 + numerator*numerator/(denominator*denominator);
      // cal error
      error = sqrt(q1*q2)/denominator;
    } 
    outputWS->dataE(0)[j] = error;
     
    prog.report();
  }

  //Copy the imput time bins on to the output
  outputWS->dataX(0) = inputWS->readX(0);

  // Update Y axis label 
  outputWS->setYUnitLabel("Asymmetry");

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithm
} // namespace Mantid


