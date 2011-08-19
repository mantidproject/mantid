#include "MantidAlgorithms/PDFFT.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM( PDFFT)

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDFFT::PDFFT() {
	// TODO Auto-generated constructor stub
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDFFT::~PDFFT() {
	// TODO Auto-generated destructor stub
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void PDFFT::initDocs() {
	this->setWikiSummary("Fourier Transform for PDF Q-Space (VZ)");
	this->setOptionalMessage("Data must be in Q-space (VZ).");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDFFT::init() {
	API::WorkspaceUnitValidator<>* uv = new API::WorkspaceUnitValidator<>(
			"MomentumTransfer");
	// CompositeValidator<> *wsValidator = new CompositeValidator<>;
	// wsValidator->add(new WorkspaceUnitValidator<>("MomentumTransfer"));
	// declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, wsValidator), "An input workspace S(d).");
	declareProperty(new WorkspaceProperty<> ("InputWorkspace", "",
			Direction::Input, uv), "An input workspace S(d).");
	declareProperty(new WorkspaceProperty<> ("OutputGorRWorkspace", "",
			Direction::Output), "An output workspace G(r)");
	declareProperty(new WorkspaceProperty<> ("OutputQSQm1Workspace", "",
			Direction::Output), "An output workspace for Q(S(Q)-1))");
  declareProperty(new Kernel::PropertyWithValue<double>("RMax", 20, Direction::Input),
      "Maximum r value of output G(r).");
	// declareProperty("RMax", 20.0);
  declareProperty(new Kernel::PropertyWithValue<double>("DeltaR", 0.01, Direction::Input),
      "Step of r in G(r). ");
	declareProperty(new Kernel::PropertyWithValue<double>("Qmin", 0.0, Direction::Input),
	    "Staring value Q of S(Q) for Fourier Transform");
	declareProperty(new Kernel::PropertyWithValue<double>("Qmax", 50.0, Direction::Input),
	    "Ending value of Q of S(Q) for Fourier Transform");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDFFT::exec() {
	// TODO Auto-generated execute stub

	// Accept d-space S(d)
	//
	// 1. Generate a Workspace for G
	const double rmax = getProperty("RMax");
	const double deltar = getProperty("DeltaR");
	double qmax = getProperty("Qmax");
	double qmin = getProperty("Qmin");
  int sizer = static_cast<int>(rmax/deltar);

	// 2. Set up G(r) dataX(0)
	Gspace
			= WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
	MantidVec& vr = Gspace->dataX(0);
	MantidVec& vg = Gspace->dataY(0);
	MantidVec& vge = Gspace->dataE(0);
	for (int i = 0; i < sizer; i++) {
		vr[i] = deltar * (1 + i);
	}

	Sspace = getProperty("InputWorkspace");

	// 3. Check input workgroup, esp. the UNIT
	std::string unit;
	Unit_sptr& iunit = Sspace->getAxis(0)->unit();
	if (iunit->unitID() == "dSpacing") {
	  unit = "d";
	} else if (iunit->unitID() == "MomentumTransfer") {
	  unit = "Q";
	} else {
			g_log.error() << "Unit " << iunit->unitID() << " is not supported"
					<< std::endl;
			throw std::invalid_argument("Unit of input Workspace is not supported");
	}

	g_log.information() << "Range of Q for F.T. : (" << qmin << ", " << qmax << ")\n";

	// 4. Check datamax, datamin and do Fourier transform
	const MantidVec& inputx = Sspace->dataX(0);
	int sizesq = static_cast<int>(inputx.size());
	double error;

	if (unit == "d") {
	  // d-Spacing
	  g_log.information()<< "Fourier Transform in d-Space" << std::endl;

	  double datadmax = 2 * M_PI / inputx[inputx.size() - 1];
		double datadmin = 2 * M_PI / inputx[0];
		double dmin = 2*M_PI/qmax;
		double dmax = 2*M_PI/qmin;

	  if (dmin < datadmin) {
	    g_log.notice() << "User input dmin = " << dmin << "is out of range.  Using Min(d) = " << datadmin << "instead\n";
	    dmin = datadmin;
	  }
	  if (dmax > datadmax) {
	    g_log.notice() << "User input dmax = " << dmax << "is out of range.  Using Max(d) = " << datadmax << "instead\n";
	    dmax = datadmax;
	  }

		for (int i = 0; i < sizer; i ++){
	      vg[i] = CalculateGrFromD(vr[i], error, dmin, dmax);
	      vge[i] = error;
		}

	} else if (unit == "Q"){
	  // Q-spacing
	  g_log.information()<< "Fourier Transform in Q-Space" << std::endl;

		double dataqmin = inputx[0];
		double dataqmax = inputx[inputx.size() - 1];

		if (qmin < dataqmin) {
		  g_log.notice() << "User input qmin = " << qmin << "is out of range.  Using Min(Q) = " << dataqmin << "instead\n";
		  qmin = dataqmin;
		}
		if (qmax > dataqmax) {
		  g_log.notice() << "User input qmax = " << qmax << "is out of range.  Using Max(Q) = " << dataqmax << "instead\n";
		  qmax = dataqmax;
		}

		for (int i = 0; i < sizer; i ++){
			vg[i] = CalculateGrFromQ(vr[i], error, qmin, qmax);
			vge[i] = error;
		}

	} // ENDIF unit

	// 3. TODO Calculate rho(r)????

	// 3.2 Calculate QS(Q)
	MatrixWorkspace_sptr QSspace = WorkspaceFactory::Instance().create(
			"Workspace2D", 1, sizesq, sizesq);
	const MantidVec& vecq = Sspace->dataX(0);
	const MantidVec& vecs = Sspace->dataY(0);
	// const MantidVec& vece = Sspace->dataE(0);
	MantidVec& qsqq = QSspace->dataX(0);
	MantidVec& qsqs = QSspace->dataY(0);
	MantidVec& qsqe = QSspace->dataE(0);
	for (int i = 0; i < sizesq; i ++){
		qsqq[i] = vecq[i];
		qsqs[i] = vecq[i]*(vecs[i]-1);
		qsqe[i] = 0.0;
	}

	// 4. Set property
	setProperty("OutputGorRWorkspace", Gspace);
	setProperty("OutputQSQm1Workspace", QSspace);

	return;
}

double PDFFT::CalculateGrFromD(double r, double& egr, double qmin, double qmax) {

	double gr = 0;

	const MantidVec& vd = Sspace->dataX(0);
	const MantidVec& vs = Sspace->dataY(0);
	const MantidVec& ve = Sspace->dataE(0);

	double temp, d, s, deltad;
	double error = 0;
	double dmin = 2.0 * M_PI / qmax;
	double dmax = 2.0 * M_PI / qmin;
	for (size_t i = 1; i < vd.size(); i++) {
		d = vd[i];
		if (d >= dmin && d <= dmax) {
			s = vs[i];
			deltad = vd[i] - vd[i - 1];
			temp = (s - 1) * sin(2 * M_PI * r / d) * 4 * M_PI * M_PI / (d * d * d)
					* deltad;
			gr += temp;
			error += ve[i] * ve[i];
		}
	}

	gr = gr * 2 / M_PI;
	egr = sqrt(error); //TODO: Wrong!

	return gr;
}

double PDFFT::CalculateGrFromQ(double r, double& egr, double qmin, double qmax) {

	const MantidVec& vq = Sspace->dataX(0);
	const MantidVec& vs = Sspace->dataY(0);
	const MantidVec& ve = Sspace->dataE(0);

	double sinus, q, s, deltaq, fs, error;

	fs = 0;
	error = 0;
	for (size_t i = 1; i < vq.size(); i++) {
		q = vq[i];
		if (q >= qmin && q <= qmax) {
			s = vs[i];
			deltaq = vq[i] - vq[i - 1];
			sinus  = sin(q * r) * q * deltaq;
			fs    += sinus * (s - 1.0);
			error += (sinus*ve[i]) * (sinus*ve[i]);
		  // g_log.debug() << "q[" << i << "] = " << q << "  dq = " << deltaq << "  S(q) =" << s;
		  // g_log.debug() << "  d(gr) = " << temp << "  gr = " << gr << std::endl;
		}
	}

	// Summarize
	double gr = fs * 2 / M_PI;
	egr = error*2/M_PI; //TODO: Wrong!

	return gr;
}

} // namespace Mantid
} // namespace Algorithms

