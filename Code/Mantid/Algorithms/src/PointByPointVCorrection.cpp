//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PointByPointVCorrection.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cfloat>
#include <cmath>

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(PointByPointVCorrection)

using namespace Kernel;
using namespace API;

/// Default constructor
PointByPointVCorrection::PointByPointVCorrection() : Algorithm()
{}

// Destructor
PointByPointVCorrection::~PointByPointVCorrection() {}

void PointByPointVCorrection::init()
{
  declareProperty(new WorkspaceProperty<>("InputW1","",Direction::Input),
                  "Name of the Sample workspace" );
  declareProperty(new WorkspaceProperty<>("InputW2","",Direction::Input),
                  "Name of the Vanadium workspace" );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                  "Name of the output workspace");
}

void PointByPointVCorrection::exec()
{
  // Get the input workspace and output workspace
  MatrixWorkspace_const_sptr inputWS1 = getProperty("InputW1");
  MatrixWorkspace_const_sptr inputWS2 = getProperty("InputW2");
  MatrixWorkspace_sptr outputWS= getProperty("OutputWorkspace");

  // Check that everything is OK.
  check_validity(inputWS1,inputWS2,outputWS);

  // Now do the normalisation
  const int size=inputWS1->readX(0).size();
  const int nHist=inputWS1->getNumberHistograms();
  Progress prog(this,0.0,1.0,nHist);

  PARALLEL_FOR3(inputWS1,inputWS2,outputWS)
  for (int i=0;i<nHist;i++) //Looping on all histograms
  {
		PARALLEL_START_INTERUPT_REGION

    const MantidVec& X=inputWS1->readX(i);

    MantidVec& Xresult=outputWS->dataX(i); //Copy the Xs
    Xresult=X;

    const MantidVec& Y1=inputWS1->readY(i);
    const MantidVec& Y2=inputWS2->readY(i);
    const MantidVec& E1=inputWS1->readE(i);
    const MantidVec& E2=inputWS2->readE(i);
    MantidVec& resultY=outputWS->dataY(i);
    MantidVec& resultE=outputWS->dataE(i);


    // Work on the Y data
    MantidVec binwidths(size); //MantidVec for bin widths
    MantidVec errors(size-1); //MantidVec for temporary errors
    std::adjacent_difference(X.begin(),X.end(),binwidths.begin()); //Calculate the binwidths
    std::transform(binwidths.begin()+1,binwidths.end(),Y2.begin(),resultY.begin(),VectorHelper::DividesNonNull<double>());
    std::transform(Y1.begin(),Y1.end(),resultY.begin(),resultY.begin(),std::multiplies<double>()); // Now resultY contains the A_i=s_i/v_i*Dlam_i

    // Calculate the errors squared related to A_i at this point
    double r = 0.0;
    for (int j=0;j<size-1;j++)
    {
      r=0;
      if (std::abs(Y1[j])>1e-7)
        r+=std::pow(E1[j]/Y1[j],2);
      if (std::abs(Y2[j])>1e-7)
        r+=std::pow(E2[j]/Y2[j],2);
      errors[j]=r; // This are the errors^2 of S_i/v_i*Dlam_i
      if (errors[j]>DBL_MAX || errors[j]<-DBL_MAX)
        errors[j]=0;
    }


    // Calculate the normaliser
    double factor1=std::accumulate(Y1.begin(),Y1.end(),0.0);
    double factor2=std::accumulate(resultY.begin(),resultY.end(),0.0);
    double factor=factor1/factor2;

    // Now propagate the error bars due to the normaliser
    double error2_factor1=std::inner_product(E1.begin(),E1.end(),E1.begin(),0.0);
    double error2_factor2=0;
    double test;
    for (int j=0;j<size-1;j++)
    {
      test=std::abs(std::pow(resultY[j],2));
      if (test>DBL_MAX)
        test=0;
      error2_factor2+=errors[j]*test/factor2/factor2;
    }
    double error2_factor=(error2_factor1/factor1/factor1+error2_factor2);

    //Calculate the normalized Y values
    std::transform(resultY.begin(),resultY.end(),resultY.begin(),std::bind2nd(std::multiplies<double>(),factor)); // Now result is s_i/v_i*Dlam_i*(sum_i s_i)/(sum_i S_i/v_i*Dlam_i)

    //Finally get the normalized errors
    for (int j=0;j<size-1;j++)
      resultE[j]=resultY[j]*sqrt(errors[j]+error2_factor);

    // Check that any masking matches, print a warning if not
    check_masks(inputWS1,inputWS2,i);

    prog.report();
		PARALLEL_END_INTERUPT_REGION
  }
	PARALLEL_CHECK_INTERUPT_REGION

  outputWS->setYUnitLabel("Counts normalised to a vanadium");
  outputWS->isDistribution(false);
}

/** Checks that the axes of the input workspaces match and creates the output workspace if necessary
 *  @param w1  The first input workspace
 *  @param w2  The second input workspace
 *  @param out Pointer to the output workspace
 */
void PointByPointVCorrection::check_validity(API::MatrixWorkspace_const_sptr& w1,API::MatrixWorkspace_const_sptr& w2,
                                             API::MatrixWorkspace_sptr& out)
{
  // First check that the instrument matches for both input workspaces
  // Only check base instrument because pointer comparison would fail if parameterized (see "new" in getInstrument)
  if ( w1->getBaseInstrument() != w2->getBaseInstrument() )
  {
    g_log.error("The input workspaces have different instrument definitions");
    throw std::runtime_error("The input workspaces have different instrument definitions");
  }
  // Check that the two workspaces are the same size
  if ( w1->size() != w2->size() )
  {
    g_log.error("The input workspaces are not the same size");
    throw std::runtime_error("The input workspaces are not the same size");
  }
  // Now check that the bins match
  if ( ! WorkspaceHelpers::matchingBins(w1,w2) )
  {
    g_log.error("The input workspaces have different binning");
    throw std::runtime_error("The input workspaces have different binning");
  }
  const Mantid::API::Axis* const axis1=w1->getAxis(1);
  const Mantid::API::Axis* const axis2=w2->getAxis(1);
  if (!((*axis1)==(*axis2))) // Spectra axis are different, so division does not make any sense
  {
    g_log.error("The two workspaces InputW1 and InputW2 have different spectra list");
    throw std::runtime_error("The two workspaces InputW1 and InputW2 have different spectra list");
  }

  if (out!=w1 && out!=w2) // Create a new workspace only if it is different from of the input ones.
  {
    out=API::WorkspaceFactory::Instance().create(w1);
    setProperty("OutputWorkspace",out);
  }
  else if (out==w2)
  {
    g_log.warning("Any masking in the output workspaces will be taken from the vanadium workspace (InputW2)");
  }
}

/** Checks whether the two input workspaces have the same bin masking.
 *  Prints a warning if not.
 *  @param w1 The first (sample) input workspace
 *  @param w2 The second (vanadium) input workspace
 *  @param index The workspace index to check
 */
void PointByPointVCorrection::check_masks(const API::MatrixWorkspace_const_sptr& w1,
		  const API::MatrixWorkspace_const_sptr& w2, const int& index) const
{
  static bool warned = false;
  if ( !warned )
  {
    const bool w1masked = w1->hasMaskedBins(index);
    const bool w2masked = w2->hasMaskedBins(index);
    if ( (w1masked && w2masked && ( w1->maskedBins(index) != w2->maskedBins(index) ))
         || (w1masked && !w2masked) || (!w1masked && w2masked) )
    {
      g_log.warning("The input workspaces do not have matching bin masking");
      warned = true;
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
