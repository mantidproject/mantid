/*WIKI*

The algorithm PDFFourierTransform imports S(Q) (or S(d)) or S(Q)-1 (or S(d)-1) in a Workspace object, does Fourier transform to it, 
and then store the resulted PDF (paired distribution function) in another Workspace object. 

The input Workspace can have the unit in d-space of Q-space.  The algorithm itself is able to identify the unit.  The allowed unit are MomentumTransfer and d-spacing. 

=== Output Option 1: G(r) ===

:<math> G(r) = 4\pi r[\rho(r)-\rho_0] = \frac{2}{\pi} \int_{0}^{\infty} Q[S(Q)-1]sin(Qr)dQ </math>

and in this algorithm, it is implemented as

:<math> G(r) =  \frac{2}{\pi} \sum_{Q_{min}}^{Q_{max}} Q[S(Q)-1]sin(Qr)\Delta Q </math>
In Algorithm's input parameter "PDFType", it is noted as "G(r) = 4pi*r[rho(r)-rho_0]".  
And r is from 0 to RMax with step size DeltaR.

*WIKI*/

#include "MantidAlgorithms/PDFFourierTransform.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid 
{
	namespace Algorithms 
	{

    using std::string;

    // Register the algorithm into the AlgorithmFactory
		DECLARE_ALGORITHM( PDFFourierTransform)

		using namespace Mantid::Kernel;
		using namespace Mantid::API;

    namespace { // anonymous namespace
    /// Crystalline PDF
    const string BIG_G_OF_R("G(r)");
    /// Liquids PDF
    const string LITTLE_G_OF_R("g(r)");
    /// Radial distribution function
    const string RDF_OF_R("RDF(r)");

    /// Normalized intensity
    const string S_OF_Q("S(Q)");
    /// Asymptotes to zero
    const string S_OF_Q_MINUS_ONE("S(Q)-1");
    /// Kernel of the Fourier transform
    const string Q_S_OF_Q_MINUS_ONE("Q[S(Q)-1]");
    }

    //----------------------------------------------------------------------------------------------
		/** Constructor
		*/
		PDFFourierTransform::PDFFourierTransform() 
		{
		}

		//----------------------------------------------------------------------------------------------
		/** Destructor
		*/
		PDFFourierTransform::~PDFFourierTransform() 
		{
		}

    const std::string PDFFourierTransform::name() const
    {
      return "PDFFourierTransform";
    }

    int PDFFourierTransform::version() const
    {
      return 1;
    }

    const std::string PDFFourierTransform::category() const
    {
      return "Diffraction";
    }

    //----------------------------------------------------------------------------------------------
		/// Sets documentation strings for this algorithm
		void PDFFourierTransform::initDocs() 
		{
			this->setWikiSummary("PDFFourierTransform() does Fourier transform from S(Q) to G(r), which is paired distribution function (PDF). G(r) will be stored in another named workspace.");
			this->setOptionalMessage("Fourier transform from S(Q) to G(r), which is paired distribution function (PDF). G(r) will be stored in another named workspace.");
		}

		//----------------------------------------------------------------------------------------------
		/** Initialize the algorithm's properties.
		*/
		void PDFFourierTransform::init() {
			auto uv = boost::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer");

      declareProperty(new WorkspaceProperty<> ("InputWorkspace", "", Direction::Input, uv),
                      S_OF_Q + ", " + S_OF_Q_MINUS_ONE + ", or " + Q_S_OF_Q_MINUS_ONE);
			declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "",
        Direction::Output), "Result paired-distribution function");

      // Set up input data type
      std::vector<std::string> inputTypes;
      inputTypes.push_back(S_OF_Q);
      inputTypes.push_back(S_OF_Q_MINUS_ONE);
      inputTypes.push_back(Q_S_OF_Q_MINUS_ONE);
      declareProperty("InputSofQType", S_OF_Q, boost::make_shared<StringListValidator>(inputTypes),
        "To identify whether input function");

      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);

      declareProperty("Rmax", 20., mustBePositive, "Maximum r for G(r) to calculate.");

      declareProperty("DeltaR", EMPTY_DBL(), mustBePositive,
                      "Step size of r of G(r) to calculate.  Default = <math>\\frac{\\pi}{Q_{max}}</math>.");
      declareProperty("Qmin", EMPTY_DBL(), mustBePositive,
                      "Minimum Q in S(Q) to calculate in Fourier transform.");
      declareProperty("Qmax", EMPTY_DBL(), mustBePositive,
                      "Maximum Q in S(Q) to calculate in Fourier transform.  It is the cut-off of Q in summation.");

      // Set up output data type
      std::vector<std::string> outputTypes;
      outputTypes.push_back(BIG_G_OF_R);
      outputTypes.push_back(LITTLE_G_OF_R);
      outputTypes.push_back(RDF_OF_R);
      declareProperty("PDFType", BIG_G_OF_R, boost::make_shared<StringListValidator>(outputTypes),
				"Type of output PDF including G(r)");
		}

    std::map<string, string> PDFFourierTransform::validateInputs()
    {
      std::map<string, string> result;

      double Qmin = getProperty("Qmin");
      double Qmax = getProperty("Qmax");
      if ((!isEmpty(Qmin)) && (!isEmpty(Qmax)))
        if (Qmax <= Qmin)
          result["Qmax"] = "Must be greater than Qmin";

      return result;
    }

		//----------------------------------------------------------------------------------------------
		/** Execute the algorithm.
		*/
		void PDFFourierTransform::exec() {
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
			}
			else
			{
				deltar = deltarc;
			}
			int sizer = static_cast<int>(rmax/deltar);

			bool sofq = true;
			if (typeSofQ == "S(Q)-1")
			{
				sofq = false;
				g_log.information() << "Input is S(Q)-1" << std::endl;
			} 
			else 
			{
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
			for (int i = 0; i < sizer; i++) 
			{
				vr[i] = deltar * (1 + i);
			}

			Sspace = getProperty("InputWorkspace");

			// 3. Check input workgroup, esp. the UNIT
			std::string strunit;
			Unit_sptr& iunit = Sspace->getAxis(0)->unit();
			if (iunit->unitID() == "dSpacing") 
			{
				strunit = "d";
			} 
			else if (iunit->unitID() == "MomentumTransfer") 
			{
				strunit = "Q";
			} 
			else 
			{
				g_log.error() << "Unit " << iunit->unitID() << " is not supported"
					<< std::endl;
				throw std::invalid_argument("Unit of input Workspace is not supported");
			}

			g_log.information() << "Range of Q for F.T. : (" << qmin << ", " << qmax << ")\n";

			// 4. Check datamax, datamin and do Fourier transform
			const MantidVec& inputx = Sspace->readX(0);
			double error;

			if (strunit == "d") {
				// d-Spacing
				g_log.information()<< "Fourier Transform in d-Space" << std::endl;

				double datadmax = 2 * M_PI / inputx[inputx.size() - 1];
				double datadmin = 2 * M_PI / inputx[0];
				double dmin = 2*M_PI/qmax;
				double dmax = 2*M_PI/qmin;

				if (dmin < datadmin)
				{
					g_log.notice() << "User input dmin = " << dmin << "is out of range.  Using Min(d) = " << datadmin << "instead\n";
					dmin = datadmin;
				}
				if (dmax > datadmax) 
				{
					g_log.notice() << "User input dmax = " << dmax << "is out of range.  Using Max(d) = " << datadmax << "instead\n";
					dmax = datadmax;
				}

				for (int i = 0; i < sizer; i ++)
				{
					vg[i] = CalculateGrFromD(vr[i], error, dmin, dmax, sofq);
					vge[i] = error;
				}

			} 
			else if (strunit == "Q")
			{
				// Q-spacing
				g_log.information()<< "Fourier Transform in Q-Space" << std::endl;

				double dataqmin = inputx[0];
				double dataqmax = inputx[inputx.size() - 1];

				if (qmin < dataqmin) 
				{
					g_log.notice() << "User input qmin = " << qmin << "is out of range.  Using Min(Q) = " << dataqmin << "instead\n";
					qmin = dataqmin;
				}
				if (qmax > dataqmax) 
				{
					g_log.notice() << "User input qmax = " << qmax << "is out of range.  Using Max(Q) = " << dataqmax << "instead\n";
					qmax = dataqmax;
				}

				for (int i = 0; i < sizer; i ++){
					vg[i] = CalculateGrFromQ(vr[i], error, qmin, qmax, sofq);
					vge[i] = error;
				}

			} // ENDIF unit

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
		double PDFFourierTransform::CalculateGrFromD(double r, double& egr, double qmin, double qmax, bool sofq) 
		{

			double gr = 0;

			const MantidVec& vd = Sspace->readX(0);
			const MantidVec& vs = Sspace->readY(0);
			const MantidVec& ve = Sspace->readE(0);

			double temp, s, deltad;
			double error = 0;
			double dmin = 2.0 * M_PI / qmax;
			double dmax = 2.0 * M_PI / qmin;
			for (size_t i = 1; i < vd.size(); i++)
			{
				double d = vd[i];
				if (d >= dmin && d <= dmax) 
				{
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

		double PDFFourierTransform::CalculateGrFromQ(double r, double& egr, double qmin, double qmax, bool sofq) 
		{

			const MantidVec& vq = Sspace->readX(0);
			const MantidVec& vs = Sspace->readY(0);
			const MantidVec& ve = Sspace->readE(0);

			double sinus, q, s, deltaq, fs, error;

			fs = 0;
			error = 0;
			for (size_t i = 1; i < vq.size(); i++) 
			{
				q = vq[i];
				if (q >= qmin && q <= qmax) 
				{
					if (sofq)
					{
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

