#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/FileProperty.h"

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSwans)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSwans::LoadSwans() {
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadSwans::~LoadSwans() {
}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadSwans::name() const {
	return "LoadSwans";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadSwans::version() const {
	return 1;
}

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSwans::category() const {
	return "DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadSwans::summary() const {
	return "Loads SNS SWANS Data";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSwans::init() {
	declareProperty(new FileProperty("Filename", "", FileProperty::Load, {
			".dat", ".txt" }),
			"The name of the text file to read, including its full or "
					"relative path. The file extension must be .txt or .dat.");

	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSwans::exec() {
	// TODO Auto-generated execute stub
	// Retrieve the filename from the properties
	m_filename = getPropertyValue("Filename");


	m_ws = EventWorkspace_sptr(new EventWorkspace());

	return;
}

} // namespace DataHandling
} // namespace Mantid
