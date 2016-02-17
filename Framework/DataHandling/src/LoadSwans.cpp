#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/FileProperty.h"

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

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
			new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "", Direction::Output),
			"The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSwans::exec() {

	m_ws = EventWorkspace_sptr(new EventWorkspace());

	std::string filename = getPropertyValue("Filename");

	std::ifstream input(filename, std::ifstream::binary | std::ios::ate);
	long numberOfEvents = input.tellg() / 8;
	input.seekg(0);

	m_ws->initialize(128*128, 1, 1);

	std::map<uint32_t, std::vector<uint32_t>> pos_tof_map;

	while (input.is_open()) {
		if (input.eof())
			break;

		uint32_t tof = 0;
		input.read((char*) &tof, sizeof(tof));
		tof -= static_cast<uint32_t>(1e9);
		tof = static_cast<uint32_t>(tof * 0.1);
		uint32_t pos = 0;

		input.read((char*) &pos, sizeof(pos));
		if (pos < 400000) {
			std::cout << "Pos: " << pos << std::endl;
			continue;
		}
		pos -= 400000;
		pos_tof_map[pos].push_back(tof);

	}



	std::cout << "numberOfEvents = " << numberOfEvents << std::endl;

	std::cout << "Number of pixels = " << pos_tof_map.size() << std::endl;


	for (auto it = pos_tof_map.begin(); it != pos_tof_map.end(); ++it) {
		//std::cout << it->first << " => " << it->second << '\n';
		if (it->first >= 128*128)
			std::cout << "ERROR 128*128: " << it->first << std::endl;
		EventList &el = m_ws->getEventList(it->first);
		el.setSpectrumNo(it->first);
		el.setDetectorID(it->first);

		for (auto itv = it->second.begin(); itv != it->second.end(); ++itv) {
			/* std::cout << *itv; ... */
			//if (*itv > 10000 && *itv < 30000)

			el.addEventQuickly(TofEvent(*itv));
		}

	}

	unsigned int shortest_tof = 10000;
	unsigned int longest_tof = 30000;
	// Now, create a default X-vector for histogramming, with just 2 bins.
	Kernel::cow_ptr<MantidVec> axis;
	MantidVec &xRef = axis.access();
	xRef.resize(2, 0.0);

	xRef[0] = shortest_tof; // Just to make sure the bins hold it all
	xRef[1] = longest_tof;
	// Set the binning axis using this.
	m_ws->setAllX(axis);

	IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

	// Now execute the Child Algorithm. Catch and log any error, but don't stop.
	try {
		loadInst->setPropertyValue("InstrumentName", "SWANS");
		loadInst->setProperty<EventWorkspace_sptr>("Workspace", m_ws);
		loadInst->setProperty("RewriteSpectraMap",
				Mantid::Kernel::OptionalBool(true));
		loadInst->execute();
	} catch (...) {
		g_log.information("Cannot load the instrument definition.");
	}

	// Set the output workspace property
	setProperty("OutputWorkspace", m_ws);
	return;
}

} // namespace DataHandling
} // namespace Mantid
