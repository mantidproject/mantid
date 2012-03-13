#include "MantidAlgorithms/PDFFT.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"

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
	auto uv = boost::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer");

	// Set up input data type
	std::vector<std::string> input_options;
	input_options.push_back("S(Q)");
	input_options.push_back("S(Q)-1");
	// Set up output data type
	gtype1 = "G(r)=4pi*r[rho(r)-rho_0]";
	std::vector<std::string> outputGoption;
	outputGoption.push_back(gtype1);

	declareProperty(new WorkspaceProperty<> ("InputWorkspace", "",
			Direction::Input, uv), "An input workspace S(d).");
	declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "",
			Direction::Output), "An output workspace G(r)");
	declareProperty("InputSofQType", "S(Q)", boost::make_shared<StringListValidator>(input_options),
	    "Select the input Workspace type.  It can be S(Q) or S(Q)-1");
  declareProperty(new Kernel::PropertyWithValue<double>("RMax", 20, Direction::Input),
      "Maximum r value of output G(r).");
	// declareProperty("RMax", 20.0);
  declareProperty(new Kernel::PropertyWithValue<double>("DeltaR", -0.00, Direction::Input),
      "Step of r in G(r). ");
	declareProperty(new Kernel::PropertyWithValue<double>("Qmin", 0.0, Direction::Input),
	    "Staring value Q of S(Q) for Fourier Transform");
	declareProperty(new Kernel::PropertyWithValue<double>("Qmax", 50.0, Direction::Input),
	    "Ending value of Q of S(Q) for Fourier Transform");
	declareProperty("PDFType", "GofR", boost::make_shared<StringListValidator>(outputGoption),
	    "Select the output PDF type");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDFFT::exec() {

	// Accept d-space S(d)
	//
	// 1. Generate a Workspace for G
	const double rmax = getProperty("RMax");
	const double deltarc = getProperty("DeltaR");
	double qmax = getProperty("Qmax");
	double qmin = getProperty("Qmin");
  std::string typeSofQ = getProperty("InputSofQType");
  //std::string typeGofR = getProperty("PDFType");

  // b) Process input, including defaults
  double deltar;
  if (deltarc <= 0){
    deltar = M_PI/qmax;
  } else {
    deltar = deltarc;
  }
  int sizer = static_cast<int>(rmax/deltar);

  bool sofq = true;
  if (typeSofQ == "S(Q)-1"){
    sofq = false;
    g_log.information() << "Input is S(Q)-1" << std::endl;
  } else {
    g_log.information() << "Input is S(Q)" << std::endl;
  }

	// 2. Set up G(r) dataX(0)
	Gspace
			= WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
	Gspace->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
	Unit_sptr unit = Gspace->getAxis(0)->unit();
	boost::shared_ptr<Units::Label> label = boost::dynamic_pointer_cast<Units::Label>(unit);
	label->setLabel("AtomicDistance", "Angstrom");
	// Gspace->getAxis(0)->unit()->setLabel("caption", "label");
	Gspace->setYUnitLabel("PDF");
	MantidVec& vr = Gspace->dataX(0);
	MantidVec& vg = Gspace->dataY(0);
	MantidVec& vge = Gspace->dataE(0);
	for (int i = 0; i < sizer; i++) {
		vr[i] = deltar * (1 + i);
	}

	Sspace = getProperty("InputWorkspace");

	// 3. Check input workgroup, esp. the UNIT
	std::string strunit;
	Unit_sptr& iunit = Sspace->getAxis(0)->unit();
	if (iunit->unitID() == "dSpacing") {
	  strunit = "d";
	} else if (iunit->unitID() == "MomentumTransfer") {
	  strunit = "Q";
	} else {
			g_log.error() << "Unit " << iunit->unitID() << " is not supported"
					<< std::endl;
			throw std::invalid_argument("Unit of input Workspace is not supported");
	}

	g_log.information() << "Range of Q for F.T. : (" << qmin << ", " << qmax << ")\n";

	// 4. Check datamax, datamin and do Fourier transform
	const MantidVec& inputx = Sspace->readX(0);
	int sizesq = static_cast<int>(inputx.size());
	double error;

	if (strunit == "d") {
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
	      vg[i] = CalculateGrFromD(vr[i], error, dmin, dmax, sofq);
	      vge[i] = error;
		}

	} else if (strunit == "Q"){
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
			vg[i] = CalculateGrFromQ(vr[i], error, qmin, qmax, sofq);
			vge[i] = error;
		}

	} // ENDIF unit

	// 3. TODO Calculate rho(r)????

	// 3.2 Calculate QS(Q)
	MatrixWorkspace_sptr QSspace = WorkspaceFactory::Instance().create(
			"Workspace2D", 1, sizesq, sizesq);
	const MantidVec& vecq = Sspace->readX(0);
	const MantidVec& vecs = Sspace->readY(0);
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
	setProperty("OutputWorkspace", Gspace);

	return;
}


/**
 *  Fourier transform to a specific r value in G(r)
 *  @param r:: atomic distance vlaue
 *  @param egr: error of G(r)
 *  @param qmin: mininum value of Q
 *  @param qmax: maximum value of Q
 *  @param sofq: true if input is S(Q), false if input is S(Q)-1
 */
double PDFFT::CalculateGrFromD(double r, double& egr, double qmin, double qmax, bool sofq) {

	double gr = 0;

	const MantidVec& vd = Sspace->readX(0);
	const MantidVec& vs = Sspace->readY(0);
	const MantidVec& ve = Sspace->readE(0);

	double temp, d, s, deltad;
	double error = 0;
	double dmin = 2.0 * M_PI / qmax;
	double dmax = 2.0 * M_PI / qmin;
	for (size_t i = 1; i < vd.size(); i++) {
		d = vd[i];
		if (d >= dmin && d <= dmax) {
		  if (sofq){
		    s = vs[i]-1;
		  } else {
		    s = vs[i];
		  }
			deltad = vd[i] - vd[i - 1];
			temp = (s) * sin(2 * M_PI * r / d) * 4 * M_PI * M_PI / (d * d * d)
					* deltad;
			gr += temp;
			error += ve[i] * ve[i];
		}
	}

	gr = gr * 2 / M_PI;
	egr = sqrt(error); //TODO: Wrong!

	return gr;
}

double PDFFT::CalculateGrFromQ(double r, double& egr, double qmin, double qmax, bool sofq) {

	const MantidVec& vq = Sspace->readX(0);
	const MantidVec& vs = Sspace->readY(0);
	const MantidVec& ve = Sspace->readE(0);

	double sinus, q, s, deltaq, fs, error;

	fs = 0;
	error = 0;
	for (size_t i = 1; i < vq.size(); i++) {
		q = vq[i];
		if (q >= qmin && q <= qmax) {
		  if (sofq){
		    s = vs[i]-1;
		  } else {
		    s = vs[i];
		  }
			deltaq = vq[i] - vq[i - 1];
			sinus  = sin(q * r) * q * deltaq;
			fs    += sinus * (s);
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

