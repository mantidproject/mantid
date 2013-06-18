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
#include <sstream>
#include <math.h>

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
      declareProperty("rho0", EMPTY_DBL(), mustBePositive,
                      "Average number density used for g(r) and RDF(r) conversions");
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

      API::MatrixWorkspace_const_sptr inputWS =  getProperty("InputWorkspace");
      if (inputWS->getNumberHistograms() != 1)
      {
        result["InputWorkspace"] = "Input workspace must have only one spectrum";
      }

      return result;
    }

		//----------------------------------------------------------------------------------------------
		/** Execute the algorithm.
		*/
		void PDFFourierTransform::exec() {
      // get input data
      API::MatrixWorkspace_const_sptr inputWS =  getProperty("InputWorkspace");
      MantidVec inputQ = inputWS->dataX(0);     //  x for input
      MantidVec inputDQ = inputWS->dataDx(0);   // dx for input
      MantidVec inputFOfQ = inputWS->dataY(0);  //  y for input
      MantidVec inputDfOfQ = inputWS->dataE(0); // dy for input
      if (inputDQ.empty())
        inputDQ.assign(inputQ.size(), 0.);

      // transform input data into Q/MomentumTransfer
      const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
      if (inputXunit == "MomentumTransfer")
      {
        // nothing to do
      }
      else if (inputXunit == "dSpacing")
      {
        // convert the x-units to Q/MomentumTransfer
        const double PI_2(2.*M_PI);
        std::transform(inputQ.begin(), inputQ.end(), inputQ.begin(),
                       std::bind1st(std::divides<double>(), PI_2));

        // reverse all of the arrays
        std::reverse(inputQ.begin(), inputQ.end());
        std::reverse(inputFOfQ.begin(), inputFOfQ.end());
        std::reverse(inputDfOfQ.begin(), inputDfOfQ.end());
      }
      else
      {
        std::stringstream msg;
        msg << "Input data x-axis with unit \"" << inputXunit
            << "\" is not supported (use \"MomentumTransfer\" or \"dSpacing\")";
        throw std::invalid_argument(msg.str());
      }
      g_log.debug() << "Input unit is " << inputXunit << "\n";

      // convert from histogram to density
      if (!inputWS->isHistogramData())
      {
        g_log.warning() << "This algorithm has not been tested on density data (only on histograms)\n";
        /* Don't do anything for now
        double deltaQ;
        for (size_t i = 0; i < inputFOfQ.size(); ++i)
        {
          deltaQ = inputQ[i+1] -inputQ[i];
          inputFOfQ[i] = inputFOfQ[i]/deltaQ;
          inputDfOfQ[i] = inputDfOfQ[i]/deltaQ; // TODO feels wrong
          inputQ[i] += .5*deltaQ;
          inputDQ[i] += .5*(inputDQ[i] + inputDQ[i+1]); // TODO running average
        }
        inputQ.pop_back();
        inputDQ.pop_back();
        */
      }

      // convert to Q[S(Q)-1]
      string soqType = getProperty("InputSofQType");
      if (soqType == S_OF_Q)
      {
        g_log.information() << "Subtracting one from all values\n";
        std::transform(inputFOfQ.begin(), inputFOfQ.end(), inputFOfQ.begin(),
                       std::bind2nd(std::minus<double>(), 1.));
        soqType = S_OF_Q_MINUS_ONE;
      }
      if (soqType == S_OF_Q_MINUS_ONE)
      {
        g_log.information() << "Multiplying all values by Q\n";
        std::transform(inputFOfQ.begin(), inputFOfQ.end(), inputQ.begin(), inputFOfQ.begin(),
                       std::multiplies<double>());
        // TODO error propogation
        soqType = Q_S_OF_Q_MINUS_ONE;
      }
      if (soqType != Q_S_OF_Q_MINUS_ONE)
      {
        // should never get here
        std::stringstream msg;
        msg << "Do not understand InputSofQType = " << soqType;
        throw std::runtime_error(msg.str());
      }

      // determine Q-range
      double qmin = getProperty("Qmin");
      double qmax = getProperty("Qmax");
      if (isEmpty(qmin))
        qmin = inputQ.front();
      if (isEmpty(qmax))
        qmax = inputQ.back();

      // check Q-range and print information
      if (qmin < inputQ.front())
      {
        g_log.information() << "Specified Qmin < range of data. Adjusting to data range.\n";
        qmin = inputQ.front();
      }
      if (qmax > inputQ.back())
      {
        g_log.information() << "Specified Qmax > range of data. Adjusting to data range.\n";
      }
      g_log.debug() << "Using Qmin = " << qmin << "Angstroms^-1 and Qmax = "
                          << qmax << "Angstroms^-1\n";
      // get pointers for the data range
      size_t qmin_index;
      size_t qmax_index;
      { // make variable scope small
        auto qmin_ptr = std::upper_bound(inputQ.begin(), inputQ.end(), qmin);
        qmin_index = std::distance(inputQ.begin(), qmin_ptr);
        if (qmin_index == 0)
          qmin_index += 1; // so there doesn't have to be a check below
        auto qmax_ptr = std::lower_bound(inputQ.begin(), inputQ.end(), qmax);
        qmax_index = std::distance(inputQ.begin(), qmax_ptr);
      }
      g_log.notice() << "Adjusting to data: Qmin = " << inputQ[qmin_index] << " Qmax = " << inputQ[qmax_index] << "\n";

      // determine r axis for result
      const double rmax = getProperty("RMax");
      double rdelta = getProperty("DeltaR");
      if (isEmpty(rdelta))
        rdelta = M_PI/qmax;
      size_t sizer = static_cast<size_t>(rmax/rdelta);

      API::MatrixWorkspace_sptr outputWS
          = WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
      outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
      Unit_sptr unit = outputWS->getAxis(0)->unit();
      boost::shared_ptr<Units::Label> label = boost::dynamic_pointer_cast<Units::Label>(unit);
      label->setLabel("AtomicDistance", "Angstrom");
      outputWS->setYUnitLabel("PDF");
      MantidVec& outputR = outputWS->dataX(0);
      for (size_t i = 0; i < sizer; i++)
      {
        outputR[i] = rdelta * static_cast<double>(1 + i);
      }
      g_log.information() << "Using rmin = " << outputR.front() << "Angstroms and rmax = "
                          << outputR.back() << "Angstroms\n";
      // always calculate G(r) then convert
      MantidVec& outputY = outputWS->dataY(0);
      MantidVec& outputE = outputWS->dataE(0);


      // do the math
      for (size_t r_index = 0; r_index < sizer; r_index ++){
        const double r = outputR[r_index];
        double fs = 0;
        double error = 0;
        for (size_t q_index = qmin_index; q_index < qmax_index; q_index++)
        {
          double q = inputQ[q_index];
          double deltaq = inputQ[q_index] - inputQ[q_index - 1];
          double sinus  = sin(q * r) * deltaq;
          fs    += sinus * inputFOfQ[q_index];
          error += q * q * (sinus*inputDfOfQ[q_index]) * (sinus*inputDfOfQ[q_index]);
          // g_log.debug() << "q[" << i << "] = " << q << "  dq = " << deltaq << "  S(q) =" << s;
          // g_log.debug() << "  d(gr) = " << temp << "  gr = " << gr << std::endl;
        }

        // put the information into the output
        outputY[r_index] = fs * 2 / M_PI;
        outputE[r_index] = error*2/M_PI; //TODO: Wrong!
      }

      // convert to the correct form of PDF
      string pdfType = getProperty("PDFType");
      double rho0 = getProperty("rho0");
      if (isEmpty(rho0))
      {
        const Kernel::Material &material = inputWS->sample().getMaterial();
        double materialDensity = material.numberDensity();

        if (!isEmpty(materialDensity) && materialDensity > 0)
          rho0 = materialDensity;
        else
          rho0 = 1.;
        // write out that it was reset if the value is coming into play
        if (pdfType == LITTLE_G_OF_R || pdfType == RDF_OF_R)
          g_log.information() << "Using rho0 = " << rho0 << "\n";
      }
      if (pdfType == BIG_G_OF_R)
      {
        // nothing to do
      }
      else if (pdfType == LITTLE_G_OF_R)
      {
        const double factor = 1./(4.*M_PI*rho0);
        for (size_t i = 0; i < outputY.size(); ++i)
        {
          outputY[i] = 1. + factor*outputY[i]/outputR[i];
          // TODO error propogation
        }
      }
      else if (pdfType == RDF_OF_R)
      {
        const double factor = 4.*M_PI*rho0;
        for (size_t i = 0; i < outputY.size(); ++i)
        {
          outputY[i] = outputR[i] * outputY[i] + factor * outputR[i] * outputR[i];
          // TODO error propogation
        }
      }
      else
      {
        // should never get here
        std::stringstream msg;
        msg << "Do not understand PDFType = " << pdfType;
        throw std::runtime_error(msg.str());
      }

      // set property
      setProperty("OutputWorkspace", outputWS);
		}

	} // namespace Mantid
} // namespace Algorithms

