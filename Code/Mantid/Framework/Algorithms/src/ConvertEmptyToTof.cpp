/*WIKI*

 Only for ILL usage.

 At the ILL the data is loaded in raw format : no units used. The X-axis represent the time channel number.

 This algorithm converts the channel number to time of flight using:
 * Spectrum or list of spectra to look for the elastic peak. Note that the spectra chosen must be at the same distance.
 * Elastic peak channels - List of channels to look for the elastic peak (e.g. range).

 So far this has only be tested on ILL D17.

 *WIKI*/

#include "MantidAlgorithms/ConvertEmptyToTof.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ConstraintFactory.h"

#include <cmath>
#include <map>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertEmptyToTof)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertEmptyToTof::ConvertEmptyToTof() {
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertEmptyToTof::~ConvertEmptyToTof() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertEmptyToTof::name() const {
	return "ConvertEmptyToTof";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int ConvertEmptyToTof::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertEmptyToTof::category() const {
	return "Transforms\\Units";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void ConvertEmptyToTof::initDocs() {
	this->setWikiSummary("Converts the channel number to time of flight.");
	this->setOptionalMessage("Converts the channel number to time of flight.");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertEmptyToTof::init() {

	auto wsValidator = boost::make_shared<CompositeValidator>();
	wsValidator->add<WorkspaceUnitValidator>("Empty");
	//wsValidator->add<HistogramValidator>();
	declareProperty(
			new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "",
					Direction::Input, wsValidator),
			"Name of the input workspace");
	declareProperty(
			new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
					Direction::Output),
			"Name of the output workspace, can be the same as the input");
	declareProperty(new Kernel::ArrayProperty<int>("ListOfSpectraIndices"),
			"A list of spectra indices as a string with ranges; e.g. 5-10,15,20-23. \n"
					"Optional: if not specified, then the Start/EndIndex fields are used alone. "
					"If specified, the range and the list are combined (without duplicating indices). For example, a range of 10 to 20 and a list '12,15,26,28' gives '10-20,26,28'.");

	declareProperty(new Kernel::ArrayProperty<int>("ListOfChannelIndices"),
			"A list of spectra indices as a string with ranges; e.g. 5-10,15,20-23. \n"
					"Optional: if not specified, then the Start/EndIndex fields are used alone. "
					"If specified, the range and the list are combined (without duplicating indices). For example, a range of 10 to 20 and a list '12,15,26,28' gives '10-20,26,28'.");

}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertEmptyToTof::exec() {

	DataObjects::Workspace2D_sptr inputWS = this->getProperty("InputWorkspace");
	MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");
	const std::vector<int> spectraIndices = getProperty("ListOfSpectraIndices");
	const std::vector<int> channelIndices = getProperty("ListOfChannelIndices");

	std::map <int, int> eppMap = findElasticPeakPositions(inputWS, spectraIndices, channelIndices);

	for (auto it = eppMap.begin(); it != eppMap.end(); ++it) {
		g_log.debug() << it->first << " -> " << it->second << std::endl;
	}

	std::map<int, double> eppTofMap = findElasticPeakTof(inputWS, eppMap);

}

/**
 * Looks for the EPP positions in the spectraIndices
 * @return map with worskpace spectra index, elastic peak position for this spectra
 */
std::map <int, int> ConvertEmptyToTof::findElasticPeakPositions(const DataObjects::Workspace2D_sptr inputWS, const std::vector<int> &spectraIndices, const std::vector<int> &channelIndices){

	std::map <int, int> eppMap;

	// make sure we not looking for channel indices outside the bounds
	assert(static_cast<size_t>(*(channelIndices.end()-1)) <  inputWS->blocksize() );

	g_log.information() << "Peak detection, search for peak " << std::endl;

	for (auto it = spectraIndices.begin(); it != spectraIndices.end(); ++it) {
		/* std::cout << *it; ... */

		int spectrumIndex = *it;
		const Mantid::MantidVec& thisSpecY = inputWS->dataY(spectrumIndex);

		int minChannelIndex =  *(channelIndices.begin());
		int maxChannelIndex =  *(channelIndices.end()-1);

		double center, sigma, height, minX, maxX;
		minX = static_cast<double>(minChannelIndex);
		maxX = static_cast<double>(maxChannelIndex);
		estimateFWHM(thisSpecY,center, sigma,height,minX,maxX);

		g_log.debug() << "Peak estimate :: center=" << center
				<< "\t sigma=" << sigma
				<< "\t height=" << height
				<< "\t minX=" << minX
				<< "\t maxX=" << maxX  << std::endl;

		bool doFit = doFitGaussianPeak(inputWS, spectrumIndex, center, sigma, height, minX, maxX);
		if (!doFit) {
			g_log.error() << "doFitGaussianPeak failed..." << std::endl;
			throw std::runtime_error("Gaussin Peak Fit failed....");
		}

		g_log.debug() << "Peak Fitting :: center=" << center
				<< "\t sigma=" << sigma
				<< "\t height=" << height
				<< "\t minX=" << minX
				<< "\t maxX=" << maxX  << std::endl;

		// round up the center to the closest int
		eppMap[spectrumIndex] = static_cast<int>(std::floor(center + 0.5) );

	}
	return eppMap;

}

/**
 * Estimated the FWHM for Gaussian peak fitting
 *
 */
void ConvertEmptyToTof::estimateFWHM(const Mantid::MantidVec& spec,
		double& center, double& sigma, double& height,
		double& minX, double& maxX ){

	//auto maxValueIt = std::max_element(spec.begin(), spec.end()); // max value
	auto maxValueIt = std::max_element(spec.begin() + static_cast<size_t>(minX), spec.begin() + static_cast<size_t>(maxX) ); // max value
	double maxValue = *maxValueIt;
	size_t maxIndex = std::distance(spec.begin(), maxValueIt); // index of max value

	//indices and values for the fwhm detection
	size_t minFwhmIndex = maxIndex;
	size_t maxFwhmIndex = maxIndex;
	double minFwhmValue = maxValue;
	double maxFwhmValue = maxValue;
	// fwhm detection
	for (; minFwhmValue > 0.5 * maxValue;
			minFwhmIndex--, minFwhmValue = spec[minFwhmIndex]) {
	}
	for (; maxFwhmValue > 0.5 * maxValue;
			maxFwhmIndex++, maxFwhmValue = spec[maxFwhmIndex]) {
	}
	//double fwhm = thisSpecX[maxFwhmIndex] - thisSpecX[minFwhmIndex + 1];
	double fwhm = static_cast<double>(maxFwhmIndex - minFwhmIndex + 1);

	//parameters for the gaussian peak fit
	center = static_cast<double>(maxIndex);
	sigma = fwhm;
	height = maxValue;

	g_log.debug() << "Peak estimate  : center=" << center << "\t sigma=" << sigma
			<< "\t h=" << height << std::endl;


	//determination of the range used for the peak definition
	size_t ipeak_min = std::max(static_cast<size_t>(0),maxIndex - static_cast<size_t>(2.5 * static_cast<double>(maxIndex - maxFwhmIndex)));
	size_t ipeak_max = std::min(spec.size(), maxIndex + static_cast<size_t>(2.5 * static_cast<double>(maxFwhmIndex - maxIndex)));
	size_t i_delta_peak = ipeak_max - ipeak_min;


	g_log.debug() << "Peak estimate xmin/max: " << ipeak_min - 1 << "\t" << ipeak_max + 1 << std::endl;


	minX = static_cast<double>(ipeak_min - 2 * i_delta_peak);
	maxX = static_cast<double>(ipeak_max + 2 * i_delta_peak);



}

/**
 * Fit peak without background i.e, with background removed
 *
 *  inspire from FitPowderDiffPeaks.cpp
 *
 *  copied from PoldiPeakDetection2.cpp
 *
 @param dataws :: input raw data for the fit
 @param workspaceindex :: indice of the row to use
 @param center :: gaussian parameter - center
 @param sigma :: gaussian parameter - width
 @param height :: gaussian parameter - height
 @param startX :: fit range - start X value
 @param endX :: fit range - end X value
 @returns A boolean status flag, true for fit success, false else
 */
bool ConvertEmptyToTof::doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws,
		int workspaceindex, double& center, double& sigma, double& height,
		double startX, double endX) {

	g_log.debug("Calling doFitGaussianPeak...");

	// 1. Estimate
	sigma = sigma * 0.5;

	// 2. Use factory to generate Gaussian
	auto temppeak = API::FunctionFactory::Instance().createFunction("Gaussian");
	auto gaussianpeak = boost::dynamic_pointer_cast<API::IPeakFunction>(
			temppeak);
	gaussianpeak->setHeight(height);
	gaussianpeak->setCentre(center);
	gaussianpeak->setFwhm(sigma);

	// 3. Constraint
	double centerleftend = center - sigma * 0.5;
	double centerrightend = center + sigma * 0.5;
	std::ostringstream os;
	os << centerleftend << " < PeakCentre < " << centerrightend;
	auto * centerbound = API::ConstraintFactory::Instance().createInitialized(
			gaussianpeak.get(), os.str(), false);
	gaussianpeak->addConstraint(centerbound);

	g_log.debug("Calling createChildAlgorithm : Fit...");
	// 4. Fit
	API::IAlgorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
	fitalg->initialize();

	fitalg->setProperty("Function",
			boost::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
	fitalg->setProperty("InputWorkspace",dataws);
	fitalg->setProperty("WorkspaceIndex", workspaceindex);
	fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
	fitalg->setProperty("CostFunction", "Least squares");
	fitalg->setProperty("MaxIterations", 1000);
	fitalg->setProperty("Output", "FitGaussianPeak");
	fitalg->setProperty("StartX", startX);
	fitalg->setProperty("EndX", endX);

	// 5.  Result
	bool successfulfit = fitalg->execute();
	if (!fitalg->isExecuted() || !successfulfit) {
		// Early return due to bad fit
		g_log.warning() << "Fitting Gaussian peak for peak around "
				<< gaussianpeak->centre() << std::endl;
		return false;
	}

	// 6. Get result
	center = gaussianpeak->centre();
	height = gaussianpeak->height();
	double fwhm = gaussianpeak->fwhm();
	if (fwhm <= 0.0) {
		return false;
	}
//	  sigma = fwhm*2;
	//  sigma = fwhm/2.35;

	return true;
}


/**
 * Finds the TOF for a given epp
 * @param eppMap : pair workspace spec index - epp
 * @return pair of workspace spec index, elastic peak tof
 */

std::map<int, double> ConvertEmptyToTof::findElasticPeakTof(const DataObjects::Workspace2D_sptr inputWS, const std::map<int, int>& eppMap){

	std::map<int, double> epTofMap;
	double l1 = getL1(inputWS);

	double wavelength;
	if (inputWS->run().hasProperty("wavelength")) {
		Kernel::Property* prop = inputWS->run().getProperty("wavelength");
		wavelength = boost::lexical_cast<double>(prop->value());
	} else {
		throw std::runtime_error("No wavelength property found in the input workspace....");
	}

	for (auto it = eppMap.begin(); it != eppMap.end(); ++it) {

		double l2 = getL2(inputWS,it->first);

		epTofMap[it->first] = (calculateTOF(l1,wavelength) + calculateTOF(l2,wavelength)) * 1e6; //microsecs

		g_log.debug() << "WS index = " << it->first << " l1 = " << l1 << " l2 = " << l2 << " TOF = " <<  epTofMap[it->first] << std::endl;
	}






//	double theoreticalElasticTOF = (m_loader.calculateTOF(m_l1,m_wavelength) + m_loader.calculateTOF(m_l2,m_wavelength))
//	        * 1e6; //microsecs
//
//	      // Calculate the real tof (t1+t2) put it in tof array
//	      std::vector<double> detectorTofBins(m_numberOfChannels + 1);
//	      for (size_t i = 0; i < m_numberOfChannels + 1; ++i)
//	      {
//	        detectorTofBins[i] = theoreticalElasticTOF
//	          + m_channelWidth
//	          * static_cast<double>(static_cast<int>(i)
//	          - calculatedDetectorElasticPeakPosition)
//	          - m_channelWidth / 2; // to make sure the bin is in the middle of the elastic peak
//
//	      }
//	      //g_log.debug() << "Detector TOF bins: ";
//	      //for (auto i : detectorTofBins) g_log.debug() << i << " ";
//	      //g_log.debug() << "\n";
//
//	      g_log.information() << "T1+T2 : Theoretical = " << theoreticalElasticTOF;
//	      g_log.information() << " ::  Calculated bin = ["
//	        << detectorTofBins[calculatedDetectorElasticPeakPosition] << ","
//	        << detectorTofBins[calculatedDetectorElasticPeakPosition + 1] << "]"
//	        << std::endl;

	return epTofMap;

}


double ConvertEmptyToTof::getL1(const API::MatrixWorkspace_sptr& workspace) {
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	Geometry::IComponent_const_sptr sample = instrument->getSample();
	double l1 = instrument->getSource()->getDistance(*sample);
	return l1;
}

double ConvertEmptyToTof::getL2(const API::MatrixWorkspace_sptr& workspace, int detId) {
	// Get a pointer to the instrument contained in the workspace
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	// Get the distance between the source and the sample (assume in metres)
	Geometry::IComponent_const_sptr sample = instrument->getSample();
	// Get the sample-detector distance for this detector (in metres)
	double l2 = workspace->getDetector(detId)->getPos().distance(
			sample->getPos());
	return l2;
}

double ConvertEmptyToTof::calculateTOF(double distance,double wavelength) {
	if (wavelength <= 0) {
		throw std::runtime_error("Wavelenght is <= 0");
	}

	double velocity = PhysicalConstants::h
			/ (PhysicalConstants::NeutronMass * wavelength * 1e-10); //m/s

	return distance / velocity;
}


} // namespace Algorithms
} // namespace Mantid
