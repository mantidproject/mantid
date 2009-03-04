//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PointByPointVCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cfloat>

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
  Vector binwidths(inputWS1->readX(0).size());
  const int nHist=inputWS1->getNumberHistograms();
  for (int i=0;i<nHist;i++) //Looping on all histograms
  {
	  const Vector& X=inputWS1->readX(i);
	  Vector& Xresult=outputWS->dataX(i);
	  Xresult=X;
	  const Vector& Y1=inputWS1->readY(i);
	  const Vector& Y2=inputWS2->readY(i);
	  Vector& result=outputWS->dataY(i);
	  std::adjacent_difference(X.begin(),X.end(),binwidths.begin()); //Calculate the binwidths
	  std::transform(Y2.begin(),Y2.end(),binwidths.begin()+1,result.begin(),std::divides<double>());
	  std::replace_if(result.begin(),result.end(),std::bind2nd(std::equal_to<double>(),0),DBL_MAX);
	  std::transform(Y1.begin(),Y1.end(),result.begin(),result.begin(),std::divides<double>());
	  double sum=std::accumulate(Y1.begin(),Y1.end(),0.0);
	  sum/=std::accumulate(result.begin(),result.end(),0.0);
	  std::transform(result.begin(),result.end(),result.begin(),std::bind2nd(std::multiplies<double>(),sum));
  }
  binwidths.clear();
  outputWS->setYUnit("Counts normalised to a vanadium");
}

void PointByPointVCorrection::check_validity(MatrixWorkspace_sptr& w1,MatrixWorkspace_sptr& w2,MatrixWorkspace_sptr& out)
{
	  if (w1->YUnit()!=w2->YUnit()) //Unit not the same
	  {
		  g_log.error("InputW1 and InputW2 have different Y units");
		  throw std::runtime_error("InputW1 and InputW2 have different Y units");
	  }
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
