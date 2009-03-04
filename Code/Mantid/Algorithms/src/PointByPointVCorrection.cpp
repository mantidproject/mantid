//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PointByPointVCorrection.h"
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
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D_const_sptr;

// Get a reference to the logger
Logger& PointByPointVCorrection::g_log = Logger::get("PointByPointVCorrection");

/// Default constructor
PointByPointVCorrection::PointByPointVCorrection() : Algorithm()
{}

// Destructor
PointByPointVCorrection::~PointByPointVCorrection() {}

void PointByPointVCorrection::init()
{
  declareProperty(new WorkspaceProperty<>("InputW1","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("InputW2","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void PointByPointVCorrection::exec()
{
  // Get the input workspace and output workspace
  MatrixWorkspace_sptr inputWS1 = getProperty("InputW1");
  MatrixWorkspace_sptr inputWS2 = getProperty("InputW2");
  MatrixWorkspace_sptr outputWS= getProperty("OutputWorkspace");

  // Check that everything is OK.
  check_validity(inputWS1,inputWS2,outputWS);

  // Now do the normalisation
  typedef std::vector<double> Vector;
  const int size=inputWS1->readX(0).size();
  Vector binwidths(size); //Vector for bin widths
  Vector errors(size-1); //Vector for temporary errors

  double r=0;

  const int nHist=inputWS1->getNumberHistograms();

  for (int i=0;i<nHist;i++) //Looping on all histograms
  {
	  const Vector& X=inputWS1->readX(i);

	  Vector& Xresult=outputWS->dataX(i); //Copy the Xs
	  Xresult=X;

	  const Vector& Y1=inputWS1->readY(i);
	  const Vector& Y2=inputWS2->readY(i);
	  const Vector& E1=inputWS1->readE(i);
	  const Vector& E2=inputWS2->readE(i);
	  Vector& resultY=outputWS->dataY(i);
	  Vector& resultE=outputWS->dataE(i);


	  // Work on the Y data

	  std::adjacent_difference(X.begin(),X.end(),binwidths.begin()); //Calculate the binwidths
	  std::transform(Y2.begin(),Y2.end(),binwidths.begin()+1,resultY.begin(),std::divides<double>());
	  std::replace_if(resultY.begin(),resultY.end(),std::bind2nd(std::equal_to<double>(),0),DBL_MAX); //Make sure we are not dividing by zero
	  std::transform(Y1.begin(),Y1.end(),resultY.begin(),resultY.begin(),std::divides<double>()); // Now resultY contains the s_i/v_i*Dlam_i

	  // Calculate the errors squared related to resultY at this point
	  for (int j=0;j<size-1;j++)
	  {
		  r=std::pow(resultY[j],2)*(std::pow(E1[j]/Y1[j],2)+std::pow(E2[j]/Y2[j],2)); //No errors on the Dlam_i
		  if (r!=r) //Make sure it is a finite number
			  r=0;
		  errors[j]=r; // This are the errors^2 of S_i/v_i*Dlam_i
	  }

	  // Calculate the normaliser
	  double factor1=std::accumulate(Y1.begin(),Y1.end(),0.0);
	  double factor2=std::accumulate(resultY.begin(),resultY.end(),0.0);
	  double factor=factor1/factor2;

	  // Now propagate the error bars due to the normaliser
	  double error2_factor1=std::inner_product(E1.begin(),E1.end(),E1.begin(),0.0);
	  double error2_factor2=std::accumulate(errors.begin(),errors.end(),0.0);
	  double error2_factor=std::pow(factor,2)*(error2_factor1/factor1/factor1+error2_factor2/factor2/factor2);

	  //Calculate the normalized Y values
	  std::transform(resultY.begin(),resultY.end(),resultY.begin(),std::bind2nd(std::multiplies<double>(),factor)); // Now result is s_i/v_i*Dlam_i*(sum_i s_i)/(sum_i S_i/v_i*Dlam_i)

	  //Finally get the normalized errors
	  for (int j=0;j<size-1;j++)
		resultE[j]=resultY[j]*sqrt(errors[j]+error2_factor);
  }

  binwidths.clear();
  errors.clear();
  outputWS->setYUnit("Counts normalised to a vanadium");
}

void PointByPointVCorrection::check_validity(MatrixWorkspace_sptr& w1,MatrixWorkspace_sptr& w2,MatrixWorkspace_sptr& out)
{
//	  if (w1->YUnit()!=w2->YUnit()) //Unit not the same
//	  {
//		  g_log.error("InputW1 and InputW2 have different Y units");
//		  throw std::runtime_error("InputW1 and InputW2 have different Y units");
//	  }
	  Mantid::API::Axis* axis1=w1->getAxis(1);
	  Mantid::API::Axis* axis2=w2->getAxis(1);
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
}



} // namespace Algorithm
} // namespace Mantid
