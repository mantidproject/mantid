#include "MantidAlgorithms/PDFFT.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  //DECLARE_ALGORITHM(PDFFT)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PDFFT::PDFFT()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PDFFT::~PDFFT()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PDFFT::initDocs()
  {
    this->setWikiSummary("Fourier Transform for PDF Q-Space (VZ)");
    this->setOptionalMessage("Data must be in Q-space (VZ).");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PDFFT::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace S(d).");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace G(r)");
    declareProperty(new Kernel::PropertyWithValue<double>("RMax", 20));
    declareProperty(new Kernel::PropertyWithValue<double>("DeltaR", 0.1));
    declareProperty(new Kernel::PropertyWithValue<double>("Qmin", 0.1));
    declareProperty(new Kernel::PropertyWithValue<double>("Qmax", 2.6));
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PDFFT::exec()
  {
    // TODO Auto-generated execute stub

	  // Accept d-space S(d)
	  //
	  // 1. Generate a Workspace for G
	  const double rmax = getProperty("RMax");
	  const double deltar = getProperty("DeltaR");
	  const double qmax = getProperty("Qmax");
	  const double qmin = getProperty("Qmin");

	  int sizer = rmax/deltar;

	  // 2. Set up G(r)
	  Gspace = WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
	  MantidVec& vr = Gspace->dataX(0);
	  MantidVec& vg = Gspace->dataY(0);
	  MantidVec& vge = Gspace->dataE(0);
	  for (int i = 0; i < sizer; i ++){
		  vr[i] = deltar*(1+i);
	  }

	  Sspace = getProperty("InputWorkspace");

	  std::string unit;
	  Unit_sptr& iunit = Sspace->getAxis(0)->unit();
	  if (iunit->unitID() == "dSpacing"){
		  unit = "d";
	  } else if (iunit->unitID() == "MomentumTransfer"){
		  unit = "Q";
	  } else {
		  g_log.error() << "Unit " << iunit->unitID() << " is not supported" << std::endl;
	  }

	  if (unit != "Q" && unit != "d"){
		  g_log.information() << "Unit " << unit << " is not supported (Q or d)" << std::endl;
	  }

	  // 3. Calculate G(r)
	  // g_log.information() << "Unit = " << unit << "  Size = " << sizer << std::endl;
#if 0
	  printout = true;
#endif
	  for (int i = 0; i < sizer; i ++){
		  double error;
		  if (unit == "Q"){
			  vg[i] = CalculateGrFromQ(vr[i], error, qmin, qmax);
#if 0
			  if (printout)
				  printout = false;
#endif
			  vge[i] = error;

		  } else if (unit == "d"){
			  vg[i] = CalculateGrFromD(vr[i], error, qmin, qmax);
			  vge[i] = error;
		  }
	  }


	  // 3. TODO Calculate rho(r)????

	  // 4. Set property
	  setProperty("OutputWorkspace", Gspace);

	  return;
  }

  double PDFFT::CalculateGrFromD(double r, double& egr, double qmin, double qmax){

      double gr = 0;
      double PI = 3.1415926545;

      const MantidVec& vd = Sspace->dataX(0);
      const MantidVec& vs = Sspace->dataY(0);
      const MantidVec& ve = Sspace->dataE(0);

      double temp, d, s, deltad;
      double error = 0;
      double dmin = 2.0*PI/qmax;
      double dmax = 2.0*PI/qmin;
      for (size_t i = 1; i < vd.size(); i ++){
          d = vd[i];
          if (d >= dmin && d <= dmax){
        	  s = vs[i];
        	  deltad = vd[i]-vd[i-1];
        	  temp = (s-1)*sin(2*PI*r/d)*4*PI*PI/(d*d*d)*deltad;
        	  gr += temp;
        	  error += ve[i]*ve[i];
          }
      }

      gr = gr*2/PI;
      egr = sqrt(error); //TODO: Wrong!

      return gr;
  }

  double PDFFT::CalculateGrFromQ(double r, double& egr, double qmin, double qmax){

      double gr = 0;
      double PI = 3.1415926545;

      const MantidVec& vq = Sspace->dataX(0);
      const MantidVec& vs = Sspace->dataY(0);
      const MantidVec& ve = Sspace->dataE(0);

      double temp, q, s, deltaq;
      double error = 0;

      // g_log.information()<< "r = " << r << "  size(q) = " << vq.size() << std::endl;

      for (size_t i = 1; i < vq.size(); i ++){
          q = vq[i];
          if (q>=qmin && q<= qmax)
          {
        	  s = vs[i];
        	  deltaq = vq[i]-vq[i-1];
        	  temp = q*(s-1)*sin(q*r)*deltaq;
        	  gr += temp;
        	  error += ve[i]*ve[i];
#if 0
        	  if (printout){
        	  g_log.information() << "q[" << i << "] = " << q << "  dq = " << deltaq << "  S(q) =" << s;
        	  g_log.information() << "  d(gr) = " << temp << "  gr = " << gr << std::endl;
        	  }
#endif
          }
      }

      gr = gr*2/PI;
      egr = sqrt(error); //TODO: Wrong!

      return gr;
  }

} // namespace Mantid
} // namespace Algorithms

