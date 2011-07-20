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
	declareProperty(new WorkspaceProperty<> ("OutputWorkspace0", "",
			Direction::Output), "An output workspace G(r)");
	declareProperty(new WorkspaceProperty<> ("OutputWorkspace1", "",
			Direction::Output), "An output workspace for Q(S(Q)-1))");
  declareProperty(new Kernel::PropertyWithValue<double>("RMax", 20, Direction::Input),
      "Maximum r value of output G(r).");
	// declareProperty("RMax", 20.0);
  declareProperty(new Kernel::PropertyWithValue<double>("DeltaR", 0.1, Direction::Input),
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

	// 2. Set up G(r)
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

	if (unit != "Q" && unit != "d") {
		g_log.information() << "Unit " << unit << " is not supported (Q or d)"
				<< std::endl;
	}

	// 4. Check qmax, qmin
	double dataqmax, dataqmin;
	const MantidVec& inputx = Sspace->dataX(0);
	int sizesq = static_cast<int>(inputx.size());
	if (unit == "d") {
		dataqmax = 2 * M_PI / inputx[inputx.size() - 1];
		dataqmin = 2 * M_PI / inputx[0];
	} else {
		dataqmin = inputx[0];
		dataqmax = inputx[inputx.size() - 1];
	}
	if (qmin < dataqmin) {
		qmin = dataqmin;
	}
	if (qmax > dataqmax) {
		qmax = dataqmax;
	}

	// 5. Calculate G(r)
  g_log.debug() << "Unit = " << unit << "  Size = " << sizer << std::endl;
	for (int i = 0; i < sizer; i++) {
		double error;
		if (unit == "Q") {
			vg[i] = CalculateGrFromQ(vr[i], error, qmin, qmax);
			vge[i] = error;

		} else if (unit == "d") {
			vg[i] = CalculateGrFromD(vr[i], error, qmin, qmax);
			vge[i] = error;
		}
	}

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
	setProperty("OutputWorkspace0", Gspace);
	setProperty("OutputWorkspace1", QSspace);

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

	double gr = 0;
	double PI = 3.1415926545;

	const MantidVec& vq = Sspace->dataX(0);
	const MantidVec& vs = Sspace->dataY(0);
	const MantidVec& ve = Sspace->dataE(0);

	double temp, q, s, deltaq;
	double error = 0;

	// g_log.information()<< "r = " << r << "  size(q) = " << vq.size() << std::endl;

	for (size_t i = 1; i < vq.size(); i++) {
		q = vq[i];
		if (q >= qmin && q <= qmax) {
			s = vs[i];
			deltaq = vq[i] - vq[i - 1];
			temp = q * (s - 1) * sin(q * r) * deltaq;
			gr += temp;
			error += ve[i] * ve[i];
#if 0
			if (printout) {
				g_log.information() << "q[" << i << "] = " << q << "  dq = " << deltaq << "  S(q) =" << s;
				g_log.information() << "  d(gr) = " << temp << "  gr = " << gr << std::endl;
			}
#endif
		}
	}

	// Extrapolate to zero
	double dq = fabs(vq[1]-vq[0]);
	size_t num = size_t(qmin/dq);
	g_log.information() << "Extrapolate:  qmin = " << qmin << "  num = " << num << "  dq = " << dq << std::endl;
	for (size_t i = 0; i < num; i ++){
    q = double(i)*dq;
    s = 0.0;
    deltaq = dq;
    temp = q * (s - 1) * sin(q * r) * deltaq;
    gr += temp;
    error += 0;
	}

	// Summarize
	gr = gr * 2 / PI;
	egr = sqrt(error); //TODO: Wrong!

	return gr;
}

} // namespace Mantid
} // namespace Algorithms

