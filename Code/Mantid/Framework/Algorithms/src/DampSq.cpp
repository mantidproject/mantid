#include "MantidAlgorithms/DampSq.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DampSq)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  const double PI = 3.14159265;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DampSq::DampSq()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DampSq::~DampSq()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DampSq::initDocs()
  {
    this->setWikiSummary("Damping S(Q) before Fourier transform");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DampSq::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace for S(Q)");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace for S(Q).");
    declareProperty(new Kernel::PropertyWithValue<int>("Mode", 3));
    declareProperty(new Kernel::PropertyWithValue<double>("QMax", 30));
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DampSq::exec()
  {
    // TODO Auto-generated execute stub

	  // 1. Generate new workspace
	  API::MatrixWorkspace_const_sptr isqspace = getProperty("InputWorkspace");
	  API::MatrixWorkspace_sptr osqspace = WorkspaceFactory::Instance().create(isqspace, 1, isqspace->size(), isqspace->size());

	  int mode = getProperty("Mode");
	  double qmax = getProperty("QMax");

	  if (mode < 1 || mode > 4){
		  g_log.error("Damp mode can only be 1, 2, 3, or 4");
		  return;
	  }

	  // 2. Get access to all
	  const MantidVec& iQVec = isqspace->dataX(0);
	  const MantidVec& iSVec = isqspace->dataY(0);
	  const MantidVec& iEVec = isqspace->dataE(0);

	  MantidVec& oQVec = osqspace->dataX(0);
	  MantidVec& oSVec = osqspace->dataY(0);
	  MantidVec& oEVec = osqspace->dataE(0);

	  // 3. Calculation
	  double dqmax = qmax - iQVec[0];

	  double damp;
	  for (unsigned int i = 0; i < iQVec.size(); i ++){
		  // a) calculate damp coefficient
		  switch (mode){
		  case 1:
			  damp = dampcoeff1(iQVec[i], qmax, dqmax);
			  break;
		  case 2:
			  damp = dampcoeff2(iQVec[i], qmax, dqmax);;
			  break;
		  case 3:
			  damp = dampcoeff3(iQVec[i], qmax, dqmax);;
			  break;
		  case 4:
			  damp = dampcoeff4(iQVec[i], qmax, dqmax);;
			  break;
		  default:
			  damp = 0;
			  break;
		  }
		  // b) calculate new S(q)
		  oQVec[i] = iQVec[i];
		  oSVec[i] = 1 + damp*(iSVec[i]-1);
		  oEVec[i] = damp*iEVec[i];
	  }  // i

	  // 4. Over
	  setProperty("OutputWorkspace", osqspace);

	  return;
  }

  double DampSq::dampcoeff1(double q, double qmax, double dqmax){
	  double deltaq = qmax-q;
	  double t = PI*deltaq/dqmax;
	  double damp = sin(t)/t;
	  return damp;
  }

  double DampSq::dampcoeff2(double q, double qmax, double dqmax){
	  double deltaq = qmax-q;
	  double t1 = deltaq/dqmax;
	  double t2 = PI*t1;
	  double damp = sin(t2)/t2*exp(-4*t1*t1);
	  return damp;
  }

  double DampSq::dampcoeff3(double q, double qmax, double dqmax){
    UNUSED_ARG(q)
    UNUSED_ARG(qmax)
    UNUSED_ARG(dqmax)
	  double damp=1;
	  return damp;
  }
  double DampSq::dampcoeff4(double q, double qmax, double dqmax){
	  double deltaq = qmax-q;
	  double t = cos(0.5*PI*deltaq/dqmax);
	  double damp = t*t;
	  return damp;
  }



} // namespace Mantid
} // namespace Algorithms

