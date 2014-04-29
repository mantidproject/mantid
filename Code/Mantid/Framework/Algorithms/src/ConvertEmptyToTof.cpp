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
	wsValidator->add < WorkspaceUnitValidator > ("Empty");
	wsValidator->add<HistogramValidator>();
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

	MatrixWorkspace_const_sptr  inputWS = this->getProperty("InputWorkspace");
	MatrixWorkspace_const_sptr  outputWS = this->getProperty("OutputWorkspace");
	const std::vector<int> spectraIndices = getProperty("ListOfSpectraIndices");
	const std::vector<int> channelIndices = getProperty("ListOfChannelIndices");

}

} // namespace Algorithms
} // namespace Mantid
