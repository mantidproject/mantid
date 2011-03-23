#include "MantidAlgorithms/PDFFT.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PDFFT)
  
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
    declareProperty(new WorkspaceProperty<>("SWorkspace","",Direction::Input), "An input workspace S(d).");
    declareProperty(new WorkspaceProperty<>("GrWorkspace","",Direction::Output), "An output workspace G(r)");
    declareProperty(new Kernel::PropertyWithValue<double>("RMax", 20));
    declareProperty(new Kernel::PropertyWithValue<double>("DeltaR", 0.1));
    declareProperty(new Kernel::PropertyWithValue<std::string>("Unit", "Q"));
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
	  const std::string unit = getProperty("Unit");

	  int sizer = rmax/deltar;

	  // 2. Set up G(r)
	  Gspace = WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
	  MantidVec& vr = Gspace->dataX(0);
	  MantidVec& vg = Gspace->dataY(0);
	  MantidVec& vge = Gspace->dataE(0);
	  for (int i = 0; i < sizer; i ++){
		  vr[i] = deltar*(1+i);
	  }

	  Sspace = getProperty("SWorkspace");

	  if (unit != "Q" && unit != "d"){
		  g_log.information() << "Unit " << unit << " is not supported (Q or d)" << std::endl;
	  }

	  // 3. Calculate G(r)
	  for (int i = 0; i < sizer; i ++){
		  double error;
		  if (unit == "Q"){
			  vg[i] = CalculateGrFromQ(vr[i], error);
			  vge[i] = error;
		  } else if (unit == "d"){
			  vg[i] = CalculateGrFromD(vr[i], error);
			  vge[i] = error;
		  }
	  }


	  // 3. TODO Calculate rho(r)????

	  // 4. Set property
	  setProperty("GrWorkspace", Gspace);

	  return;
  }

  double PDFFT::CalculateGrFromD(double r, double& egr){

      double gr = 0;
      double PI = 3.1415926545;

      const MantidVec& vd = Sspace->dataX(0);
      const MantidVec& vs = Sspace->dataY(0);
      const MantidVec& ve = Sspace->dataE(0);

      double temp, d, s, deltad;
      double error = 0;
      for (size_t i = 1; i < vd.size(); i ++){
          d = vd[i];
          s = vs[i];
          deltad = vd[i]-vd[i-1];
          temp = (s-1)*sin(2*PI*r/d)*4*PI*PI/(d*d*d)*deltad;
          gr += temp;
          error += ve[i]*ve[i];
      }

      gr = gr*2/PI;
      egr = sqrt(error); //TODO: Wrong!

      return gr;
  }

  double PDFFT::CalculateGrFromQ(double r, double& egr){

      double gr = 0;
      double PI = 3.1415926545;

      const MantidVec& vq = Sspace->dataX(0);
      const MantidVec& vs = Sspace->dataY(0);
      const MantidVec& ve = Sspace->dataE(0);

      double temp, q, s, deltaq;
      double error = 0;
      for (size_t i = 1; i < vq.size(); i ++){
          q = vq[i];
          s = vs[i];
          deltaq = vq[i]-vq[i-1];
          temp = q*(s-1)*sin(q*r)*deltaq;
          gr += temp;
          error += ve[i]*ve[i];
      }

      gr = gr*2/PI;
      egr = sqrt(error); //TODO: Wrong!

      return gr;
  }

} // namespace Mantid
} // namespace Algorithms

