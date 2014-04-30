/*WIKI*

 Only for ILL usage.

 At the ILL the data is loaded in raw format : no units used. The X-axis represent the time channel number.

 This algorithm converts the channel number to time of flight using:
 * Spectrum or list of spectra to look for the elastic peak
 * Elastic peak channels - List of channels to look for the elastic peak

 So far this has only be tested on ILL D17.

 *WIKI*/

#include "MantidAlgorithms/ConvertEmptyToTof.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ConstraintFactory.h"

#include <cmath>

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
			new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "",
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

	MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
	MatrixWorkspace_const_sptr outputWS = this->getProperty("OutputWorkspace");
	const std::vector<int> spectraIndices = getProperty("ListOfSpectraIndices");
	const std::vector<int> channelIndices = getProperty("ListOfChannelIndices");

	g_log.information() << "Peak detection, search for peak " << std::endl;

	for (auto it = spectraIndices.begin(); it != spectraIndices.end(); ++it) {
		/* std::cout << *it; ... */



		const Mantid::MantidVec& thisSpecX = inputWS->dataX(*it);
		const Mantid::MantidVec& thisSpecY = inputWS->dataY(*it);

		g_log.debug() << "Analysing spectrum idx = " <<  *it
				<< " ; size x = " << thisSpecX.size() << " ; size y = "  << thisSpecY.size() << std::endl;

		g_log.debug() << "Contents of Y: " << std::endl;
		for (auto itY = thisSpecY.begin(); itY != thisSpecY.end(); ++itY) {
			g_log.debug() << " " << *itY;
		}
		g_log.debug() << std::endl;


		auto vmaxIt = std::max_element(thisSpecY.begin(), thisSpecY.end()); // max value
		double vmax = *vmaxIt;
		size_t imax = std::distance(thisSpecY.begin(), vmaxIt) ;   // index of max value

		//indices and values for the fwhm detection
		size_t ifwhm_min = imax;
		size_t ifwhm_max = imax;
		double vfwhm_min = vmax;
		double vfwhm_max = vmax;
		// fwhm detection
		for (; vfwhm_min > 0.5 * vmax; ifwhm_min--, vfwhm_min = thisSpecY[ifwhm_min]) {
		}
		for (; vfwhm_max > 0.5 * vmax; ifwhm_max++, vfwhm_max = thisSpecY[ifwhm_max]) {
		}
		double fwhm = thisSpecX[ifwhm_max] - thisSpecX[ifwhm_min + 1];

		//determination of the range used for the peak definition
		size_t ipeak_min = std::max(static_cast<size_t>(0),imax - static_cast<int>(2.5 * static_cast<double>(imax - ifwhm_min)));
		size_t ipeak_max = std::min(thisSpecY.size(), imax + static_cast<int>(2.5 * static_cast<double>(ifwhm_max - imax)));
		size_t i_delta_peak = ipeak_max - ipeak_min;


		//parameters for the gaussian peak fit
		double center = thisSpecX[imax];
		double sigma = fwhm;
		double height = vmax;

		g_log.debug() << "Peak before   " << center << "\t" << sigma << "\t" << height << std::endl;
		g_log.debug() << "Peak xmin/max " << thisSpecX[ipeak_min - 1] << "\t" << thisSpecX[ipeak_max + 1] << std::endl;
		bool doFit = doFitGaussianPeak(inputWS, *it, center, sigma, height,
				thisSpecX[ipeak_min - 2 * i_delta_peak],
				thisSpecX[ipeak_max + 2 * i_delta_peak]);
		if (!doFit) {
			g_log.error() << "Peak after    : fit failed" << std::endl;
		}
		g_log.debug() << "Peak after    " << center << "\t" << sigma
				<< "\t" << height << std::endl;


		g_log.debug() << center << height << 2.35 * sigma << int(ipeak_min) << int(imax)
				<< int(ipeak_max) << std::endl;

		// the simulated peak is stored in the correlation ws, row 3
//		const double& weight = pow(1 / sigma, 2);
//		for (size_t i = 0; i < this->nb_d_channel; i++) {
//			double diff = X[i] - center;
//			Y2[i] += height * exp(-0.5 * diff * diff * weight);
//		}
	}

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
bool ConvertEmptyToTof::doFitGaussianPeak(MatrixWorkspace_const_sptr dataws,
		int workspaceindex, double& center, double& sigma, double& height,
		double startX, double endX) {
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

	// 4. Fit
	API::IAlgorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
	fitalg->initialize();

	fitalg->setProperty("Function",
			boost::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
	fitalg->setProperty("InputWorkspace", dataws);
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

} // namespace Algorithms
} // namespace Mantid
