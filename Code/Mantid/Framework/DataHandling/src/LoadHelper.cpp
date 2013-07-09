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
 * Finds the instrument name in the nexus file
 *
 * @param entry :: The Nexus entry
 * @return instrument name
 */
std::string LoadHelper::getInstrumentName(NeXus::NXEntry& entry) {

	// Old format: /entry0/IN5/name
	// New format: /entry0/instrument/name

	// Ugly way of getting Instrument Name
	// Instrument name is in: entry0/<NXinstrument>/name

	std::string instrumentName = "";

	std::vector<NeXus::NXClassInfo> v = entry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			std::string nexusInstrumentEntryName = it->nxname;
			std::string insNamePath = nexusInstrumentEntryName + "/name";
			if (!entry.isValid(insNamePath))
				throw std::runtime_error(
						"Error reading the instrument name: " + insNamePath
								+ " is not a valid path!");
			instrumentName = entry.getString(insNamePath);
			g_log.debug() << "Instrument Name: " << instrumentName
					<< " in NxPath: " << insNamePath << std::endl;
			break;
		}
	}

	return instrumentName;

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

} // namespace DataHandling
} // namespace Mantid
