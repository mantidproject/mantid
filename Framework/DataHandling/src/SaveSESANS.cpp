#include "MantidDataHandling/SaveSESANS.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace DataHandling {

// Register the algorithm with the AlgorithmFactory
DECLARE_ALGORITHM(SaveSESANS)

const std::string SaveSESANS::name() const {
	return "SaveSESANS";
}

const std::string SaveSESANS::summary() const {
	return "Save a file using the SESANS format";
}

int SaveSESANS::version() const {
	return 1;
}

const std::string SaveSESANS::category() const{
	return "DataHandling\\Text";
}

/**
 * Initialise the algorithm
*/
void SaveSESANS::init(){
	declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
		"InputWorkspace", "", Kernel::Direction::Input),
		"The name of the workspace to save");
	declareProperty(Kernel::make_unique<API::FileProperty>(
		"Filename", "", API::FileProperty::Save, fileExtensions),
		"The name to use when saving the file");

	//TODO : find out good descriptions (and validators) for these properties
	declareProperty("Theta_zmax", -1, Kernel::Direction::Input);
	declareProperty("Theta_zmax_unit", "", Kernel::Direction::Input);
	declareProperty("Theta_ymax", -1, Kernel::Direction::Input);
	declareProperty("Theta_ymax_unit", "", Kernel::Direction::Input);

	declareProperty<std::string>("Orientation", "", "Orientation of the instrument");
}

/**
 * Execute the algorithm
 */
void SaveSESANS::exec(){
	API::MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

	// Check workspace has only one spectrum
	if (ws->getNumberHistograms() != 1) {
		g_log.error("This algorithm expects a workspace with exactly 1 spectrum");
		throw std::runtime_error("SaveSESANS passed workspace with incorrect number of spectra, expected 1");
	}
}

} // namespace DataHandling
} // namespace Mantid
