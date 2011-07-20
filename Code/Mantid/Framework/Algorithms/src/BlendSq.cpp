#include "MantidAlgorithms/BlendSq.h"
#include <vector>
#include <math.h>
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BlendSq)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BlendSq::BlendSq()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BlendSq::~BlendSq()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BlendSq::initDocs()
  {
    this->setWikiSummary("Blended S(Q) from multiple banks");
    this->setOptionalMessage("Total scattering S(Q) from multiple banks will be rebinned and blended");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BlendSq::init()
  {
    //declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "Input workspaces for banks to be blended.");
    //declareProperty(new ArrayProperty<>("InputWorkspaces", "", Direction::Input), "Input workspaces for banks to be blended")
    declareProperty(new ArrayProperty<std::string>("InputWorkspaces", new MandatoryValidator<std::vector<std::string> >), "The names of the input workspaces as a comma-separated list" ); 
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace for blended S(Q).");
    declareProperty(new ArrayProperty<double>("RangeLowerBounds", new MandatoryValidator<std::vector<double> >), "The lower bounds of each banks");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BlendSq::exec()
  {
	  // TODO Auto-generated execute stub
	  // 1. Get input data
	  const std::vector<std::string> inputWorkspaceNames = getProperty("InputWorkspaces");
	  int numspaces = inputWorkspaceNames.size();
	  std::vector<MatrixWorkspace_const_sptr> inputWorkspaces;
	  for (int i = 0; i < numspaces; i ++){
		  MatrixWorkspace_sptr tws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWorkspaceNames[i]));
		  inputWorkspaces.push_back(tws);
	  }

	  int size;
	  if (numspaces > 0){
              const MantidVec& wx = inputWorkspaces[0]->dataX(0);
              size = wx.size();
	  } else {
		  size = 0;
		  return;
	  }

	  
	  // 2. Determine output
	  MatrixWorkspace_sptr outWs = API::WorkspaceFactory::Instance().create(inputWorkspaces[0], 1, size, size);
	  MantidVec& bx = outWs->dataX(0);
	  MantidVec& by = outWs->dataY(0);
	  MantidVec& be = outWs->dataE(0);
	  for (int i = 0; i < size; i ++){
		  bx[i] = inputWorkspaces[0]->dataX(0)[i];
		  by[i] = 0;
		  be[i] = 0; 
	  }
	  
	  // 2. Blended
	  for (int i = 0; i < numspaces; i ++){
		  const MantidVec& ix = inputWorkspaces[i]->dataX(0);
		  const MantidVec& iy = inputWorkspaces[i]->dataY(0);
		  const MantidVec& ie = inputWorkspaces[i]->dataE(0);
		  
		  for (int j = 0; j < size; j ++){
			  by[j] += iy[j] / (ie[j]*ie[j]);
			  be[j] += 1/(ie[j]*ie[j]);
		  }
	  }
	  
	  for (int j = 0; j < size; j ++){
		  be[j] = sqrt(1/be[j]);
		  by[j] = by[j]*be[j];
	  }
	  
	  // 3. Done
	  setProperty("OutputWorkspace", outWs);
	  
	  return;
  }



} // namespace Mantid
} // namespace Algorithms

