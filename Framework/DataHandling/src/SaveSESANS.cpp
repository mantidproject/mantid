#include "MantidDataHandling/SaveSESANS.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/make_unique.h"

#include <fstream>
#include <iomanip>
#include <string>

namespace Mantid {
namespace DataHandling {

// Register the algorithm with the AlgorithmFactory
DECLARE_ALGORITHM(SaveSESANS)

/// Get the algorithm's name
const std::string SaveSESANS::name() const {
	return "SaveSESANS";
}

/// Get a summary of the algorithm
const std::string SaveSESANS::summary() const {
	return "Save a file using the SESANS format";
}

/// Get the version number of the algorithm
int SaveSESANS::version() const {
	return 1;
}

/// Get the algorithm's category
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
	declareProperty("Theta_zmax", -1.0, Kernel::Direction::Input);
	declareProperty("Theta_zmax_unit", "radians", Kernel::Direction::Input);
	declareProperty("Theta_ymax", -1.0, Kernel::Direction::Input);
	declareProperty("Theta_ymax_unit", "radians", Kernel::Direction::Input);

	declareProperty<std::string>("Orientation", "", "Orientation of the instrument");
}

/**
 * Execute the algorithm
 */
void SaveSESANS::exec(){
	API::MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");

	// Check workspace has only one spectrum
	if (ws->getNumberHistograms() != 1) {
		g_log.error("This algorithm expects a workspace with exactly 1 spectrum");
		throw std::runtime_error("SaveSESANS passed workspace with incorrect number of spectra, expected 1");
	}

	std::ofstream outfile;
	outfile.open(getPropertyValue("Filename"));
	writeHeaders(outfile, ws);
	outfile << "\n" << "BEGIN_DATA" << "\n";
	outfile.close();
}

/**
 * Write header values to the output file
 * @param outfile ofstream to the output file
 * @param ws The workspace to save
 */
void SaveSESANS::writeHeaders(std::ofstream &outfile, API::MatrixWorkspace_const_sptr & ws){
	const API::Sample &sample = ws->sample();

	writeHeader(outfile, "FileFormatVersion", "1.0");
	writeHeader(outfile, "DataFileTitle", ws->getTitle());
	writeHeader(outfile, "Sample", sample.getName());
	writeHeader(outfile, "Thickness", std::to_string(sample.getThickness()));
	writeHeader(outfile, "Thickness_unit", "mm");
	writeHeader(outfile, "Theta_zmax", getPropertyValue("Theta_zmax"));
	writeHeader(outfile, "Theta_zmax_unit", getPropertyValue("Theta_zmax_unit"));
	writeHeader(outfile, "Theta_ymax", getPropertyValue("Theta_ymax"));
	writeHeader(outfile, "Theta_ymax_unit", getPropertyValue("Theta_ymax_unit"));
	writeHeader(outfile, "Orientation", "Z");
	writeHeader(outfile, "SpinEchoLength_unit", "A");
	writeHeader(outfile, "Depolarisation_unit", "A-2 cm-1");
	writeHeader(outfile, "Wavelength_unit", "A");
}

/**
 * Write a single header to the output file
 * @param outfile ofstream to the output file
 * @param name The name of the attribute being written
 * @param value The attribute's value
*/
void SaveSESANS::writeHeader(std::ofstream &outfile, const std::string & name, const std::string & value){
	outfile << std::setfill(' ') << std::setw(MAX_HDR_LENGTH) << std::left << name << value << "\n";
}



} // namespace DataHandling
} // namespace Mantid
