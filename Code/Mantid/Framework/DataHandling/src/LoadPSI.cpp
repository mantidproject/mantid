/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidDataHandling/LoadPSI.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadPSI)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadPSI::LoadPSI() :
		m_instrumentName("") {
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadPSI::~LoadPSI() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadPSI::name() const {
	return "LoadPSI";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int LoadPSI::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadPSI::category() const {
	return "DataHandling";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void LoadPSI::initDocs() {
	this->setWikiSummary("Loads PSI nexus file.");
	this->setOptionalMessage("Loads PSI nexus file.");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadPSI::init() {
	std::vector<std::string> exts;
	exts.push_back(".nxs");
	exts.push_back(".hdf");
	declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
			"The name of the Nexus file to load");
	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadPSI::exec() {

	NXEntry entry = openNexusFile();
	setInstrumentName(entry);




}

NXEntry LoadPSI::openNexusFile() {
	std::string filename = getPropertyValue("Filename");
	NXRoot root(filename);
	return root.openFirstEntry();
}

void LoadPSI::setInstrumentName(NeXus::NXEntry& entry) {

	// format: /entry0/FOCUS/name

	std::vector<NXClassInfo> v = entry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			std::string nexusInstrumentEntryName = it->nxname;
			std::string insNamePath = nexusInstrumentEntryName + "/name";
			m_instrumentName = entry.getString(insNamePath);
			if (m_instrumentName == "") {
				std::string message(
						"Cannot read the instrument name from the Nexus file!");
				g_log.error(message);
				throw std::runtime_error(message);
			} else {
				g_log.debug() << "Instrument Name: " << m_instrumentName
						<< " in NxPath: " << insNamePath << std::endl;
				break;
			}

		}
	}

}

void LoadPSI::initWorkSpace(NeXus::NXEntry& entry) {

	// read in the data
	NXData dataGroup = entry.openNXData("merged/counts");
	NXInt data = dataGroup.openIntData();

	m_numberOfTubes = static_cast<size_t>(data.dim0());
	m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
	m_numberOfChannels = static_cast<size_t>(data.dim2());

	// dim0 * m_numberOfPixelsPerTube is the total number of detectors
	m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

	g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << std::endl;
	g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << std::endl;
	g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << std::endl;

	// Now create the output workspace
	// Might need to get this value from the number of monitors in the Nexus file
	// params:
	// workspace type,
	// total number of spectra + (number of monitors = 0),
	// bin boundaries = m_numberOfChannels + 1
	// Z/time dimension
	m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",
			m_numberOfHistograms, m_numberOfChannels + 1, m_numberOfChannels);
	m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(
			"TOF");
	m_localWorkspace->setYUnitLabel("Counts");

}

} // namespace DataHandling
} // namespace Mantid
