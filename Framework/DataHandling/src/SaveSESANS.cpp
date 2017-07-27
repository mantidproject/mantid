#include "MantidDataHandling/SaveSESANS.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/make_unique.h"

#include <algorithm>
#include <cmath>
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
	declareProperty("Echo_constant", -1.0, Kernel::Direction::Input);

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

	const Mantid::MantidVec wavelength = ws->dataX(0);
	const Mantid::MantidVec yValues = ws->dataY(0);
	const Mantid::MantidVec eValues = ws->dataE(0);

	const Mantid::MantidVec spinEchoLength = calculateSpinEchoLength(wavelength);
	const Mantid::MantidVec depolarisation = calculateDepolarisation(yValues, wavelength);
	const Mantid::MantidVec error = calculateError(eValues, yValues, wavelength);

	outfile << "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n";
	for (int i = 0; i < spinEchoLength.size(); i++) {
		outfile << std::to_string(spinEchoLength[i]) << " "
		     	<< std::to_string(depolarisation[i]) << " "
				<< std::to_string(error[i]) << " "
				<< std::to_string(wavelength[i]) << "\n";
	}

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

Mantid::MantidVec SaveSESANS::calculateSpinEchoLength(const Mantid::MantidVec &wavelength) {
	Mantid::MantidVec spinEchoLength;
	const double echoConstant = getProperty("EchoConstant");

	// SEL is calculated a wavelength^2 * echoConstant
	transform(wavelength.begin(), wavelength.end(), back_inserter(spinEchoLength),
		[&](double w) { return w * w * echoConstant; });
	return spinEchoLength;
}

Mantid::MantidVec SaveSESANS::calculateDepolarisation(const Mantid::MantidVec & yValues, const Mantid::MantidVec & wavelength){
	Mantid::MantidVec depolarisation;

	// Depol is calculated as ln(y) / wavelength^2
	transform(yValues.begin(), yValues.end(), wavelength.begin(),
		back_inserter(depolarisation),
		[](double y, double w) {return log(y) / (w * w); });
	return depolarisation;
}

Mantid::MantidVec SaveSESANS::calculateError(const Mantid::MantidVec & eValues, const Mantid::MantidVec & yValues, const Mantid::MantidVec & wavelength){
	Mantid::MantidVec error;

	// Error is calculated as e / (y * wavelength^2)
	for (int i = 0; i < eValues.size(); i++) {
		error.push_back(eValues[i] / (yValues[i] * wavelength[i] * wavelength[i]));
	}
	return error;
}

} // namespace DataHandling
} // namespace Mantid
