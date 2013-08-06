/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

LoadHelper::LoadHelper() :
		g_log(Kernel::Logger::get("Algorithm")) {
}

LoadHelper::~LoadHelper() {
}


/**
 * Finds the path for the instrument name in the nexus file
 * Usually of the form: entry0/<NXinstrument class>/name
 */
std::string LoadHelper::findInstrumentNexusPath(
		const NeXus::NXEntry &firstEntry) {
	std::string insNamePath = "";
	std::vector<NeXus::NXClassInfo> v = firstEntry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			insNamePath = it->nxname;
			break;
		}
	}
	return insNamePath;
}

std::string LoadHelper::getStringFromNexusPath(const NeXus::NXEntry &firstEntry,
		const std::string &nexusPath) {
	return firstEntry.getString(nexusPath);
}

double LoadHelper::getDoubleFromNexusPath(const NeXus::NXEntry &firstEntry,
		const std::string &nexusPath) {
	return firstEntry.getFloat(nexusPath);
}

/**
 * Gets the time binning from a Nexus float array
 * Adds an extra bin at the end
 */
std::vector<double> LoadHelper::getTimeBinningFromNexusPath(
		const NeXus::NXEntry &firstEntry, const std::string &nexusPath) {

	NeXus::NXFloat timeBinningNexus = firstEntry.openNXFloat(nexusPath);
	timeBinningNexus.load();

	size_t numberOfBins = static_cast<size_t>(timeBinningNexus.dim0()) + 1; // boundaries

	float* timeBinning_p = &timeBinningNexus[0];
	std::vector<double> timeBinning(numberOfBins);
	timeBinning.assign(timeBinning_p, timeBinning_p + numberOfBins);
	// calculate the extra bin at the end
	timeBinning[numberOfBins - 1] = timeBinning[numberOfBins - 2]
			+ timeBinning[1] - timeBinning[0];

	return timeBinning;
}
/**
 * Calculate Neutron Energy from wavelength: \f$ E = h^2 / 2m\lambda ^2 \f$
 *  @param wavelength :: wavelength in \f$ \AA \f$
 *  @return tof in seconds
 */
double LoadHelper::calculateEnergy(double wavelength) {
	double e = (PhysicalConstants::h * PhysicalConstants::h)
			/ (2 * PhysicalConstants::NeutronMass * wavelength * wavelength
					* 1e-20) / PhysicalConstants::meV;
	return e;
}

/**
 * Calculate TOF from distance
 *  @param distance :: distance in meters
 *  @return tof in seconds
 */
double LoadHelper::calculateTOF(double distance,double wavelength) {
	if (wavelength <= 0) {
		g_log.error("Wavelenght is <= 0");
		throw std::runtime_error("Wavelenght is <= 0");
	}

	double velocity = PhysicalConstants::h
			/ (PhysicalConstants::NeutronMass * wavelength * 1e-10); //m/s

	return distance / velocity;
}

double LoadHelper::getL1(const API::MatrixWorkspace_sptr& workspace) {
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	Geometry::IObjComponent_const_sptr sample = instrument->getSample();
	double l1 = instrument->getSource()->getDistance(*sample);
	return l1;
}

double LoadHelper::getL2(const API::MatrixWorkspace_sptr& workspace, int detId) {
	// Get a pointer to the instrument contained in the workspace
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	// Get the distance between the source and the sample (assume in metres)
	Geometry::IObjComponent_const_sptr sample = instrument->getSample();
	// Get the sample-detector distance for this detector (in metres)
	double l2 = workspace->getDetector(detId)->getPos().distance(
			sample->getPos());
	return l2;
}

/*
 * Get instrument property as double
 * @s - input property name
 *
 */
double LoadHelper::getInstrumentProperty(const API::MatrixWorkspace_sptr& workspace, std::string s) {
	std::vector<std::string> prop =
			workspace->getInstrument()->getStringParameter(s);
	if (prop.empty()) {
		g_log.debug("Property <" + s + "> doesn't exist!");
		return EMPTY_DBL();
	} else {
		g_log.debug() << "Property <" + s + "> = " << prop[0] << std::endl;
		return boost::lexical_cast<double>(prop[0]);
	}
}

/**
 * Parses the date as formatted at the ILL:
 * 29-Jun-12 11:27:26
 * and converts it to the ISO format used in Mantid:
 * ISO8601 format string: "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"
 *
 *  @param dateToParse :: date as string
 *  @return date as required in Mantid
 */
std::string LoadHelper::dateTimeInIsoFormat(std::string dateToParse) {
	namespace bt = boost::posix_time;
	// parsing format
	const std::locale format = std::locale(std::locale::classic(),
			new bt::time_input_facet("%d-%b-%y %H:%M:%S"));

	bt::ptime pt;
	std::istringstream is(dateToParse);
	is.imbue(format);
	is >> pt;

	if (pt != bt::ptime()) {
		// Converts to ISO
		std::string s = bt::to_iso_extended_string(pt);
		return s;
	} else {
		return "";
	}
}

} // namespace DataHandling
} // namespace Mantid
