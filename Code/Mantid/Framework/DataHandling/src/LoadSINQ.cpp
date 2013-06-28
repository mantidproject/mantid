/*WIKI*

 Loads a SINQ (PSI) nexus file into a [[Workspace2D]] with the given name.

 To date this algorithm only supports: FOCUS

 *WIKI*/

#include "MantidDataHandling/LoadSINQ.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"

#include <limits>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

DECLARE_HDF_FILELOADER_ALGORITHM(LoadSINQ);

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSINQ::LoadSINQ() {
	m_instrumentName = "";
	supportedInstruments.push_back("FOCUS");
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadSINQ::~LoadSINQ() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadSINQ::name() const {
	return "LoadSINQ";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int LoadSINQ::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSINQ::category() const {
	return "DataHandling";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void LoadSINQ::initDocs() {
	this->setWikiSummary("Loads PSI nexus file.");
	this->setOptionalMessage("Loads PSI nexus file.");
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadSINQ::confidence(const Kernel::HDFDescriptor & descriptor) const
{
  // Create the root Nexus class
	NXRoot root(descriptor.filename());
	NXEntry entry = root.openFirstEntry();
	std::string nexusInstrumentName;
	std::string instrumentName = getInstrumentName(entry,nexusInstrumentName);
	if (std::find(supportedInstruments.begin(), supportedInstruments.end(),
			instrumentName) != supportedInstruments.end()) {
		// FOUND
		return 80;
	}
	return 0;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSINQ::init() {
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
void LoadSINQ::exec() {

	std::string filename = getPropertyValue("Filename");
	NXRoot root(filename);
	NXEntry entry = root.openFirstEntry();
	setInstrumentName(entry);

	initWorkSpace(entry);

	loadDataIntoTheWorkSpace(entry);

	loadRunDetails(entry);
	loadExperimentDetails(entry);

	runLoadInstrument();

	setProperty("OutputWorkspace", m_localWorkspace);
}

void LoadSINQ::setInstrumentName(NeXus::NXEntry& entry) {

	m_instrumentName = getInstrumentName(entry,m_nexusInstrumentEntryName);
	if (m_instrumentName == "") {
		std::string message(
				"Cannot read the instrument name from the Nexus file!");
		g_log.error(message);
		throw std::runtime_error(message);
	}

}

std::string LoadSINQ::getInstrumentName(NeXus::NXEntry& entry, std::string &nexusInstrumentName) const {

	// format: /entry0/?????/name

	std::string instrumentName = "";

	std::vector<NXClassInfo> v = entry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
		  nexusInstrumentName = it->nxname;
		  std::string insNamePath = nexusInstrumentName + "/name";
			if ( !entry.isValid(insNamePath) )
				throw std::runtime_error("Error reading the instrument name: " + insNamePath + " is not a valid path!");
			instrumentName = entry.getString(insNamePath);
			g_log.debug() << "Instrument Name: " << instrumentName
					<< " in NxPath: " << insNamePath << std::endl;
			break;
		}
	}
	//std::replace( instrumentName.begin(), instrumentName.end(), ' ', '_'); // replace all ' ' to '_'
	long unsigned int pos = instrumentName.find(" ");
	instrumentName = instrumentName.substr(0, pos);
	return instrumentName;

}

void LoadSINQ::initWorkSpace(NeXus::NXEntry& entry) {

	// read in the data
	NXData dataGroup = entry.openNXData("merged");
	NXInt data = dataGroup.openIntData();

	m_numberOfTubes = static_cast<size_t>(data.dim0());
	m_numberOfPixelsPerTube = 1;
	m_numberOfChannels = static_cast<size_t>(data.dim1());

	// dim0 * m_numberOfPixelsPerTube is the total number of detectors
	m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

	g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << std::endl;
	g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube
			<< std::endl;
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

void LoadSINQ::loadDataIntoTheWorkSpace(NeXus::NXEntry& entry) {

	// read in the data
	NXData dataGroup = entry.openNXData("merged");
	NXInt data = dataGroup.openIntData();
	data.load();

	NXFloat timeBinning = entry.openNXFloat("merged/time_binning");
	timeBinning.load();

	size_t numberOfBins = static_cast<size_t>(timeBinning.dim0()) + 1; // boundaries
	g_log.debug() << "Number of bins: " << numberOfBins << std::endl;

	// Assign time bin to first X entry
	float* timeBinning_p = &timeBinning[0];
	std::vector<double> timeBinningTmp(numberOfBins);
	timeBinningTmp.assign(timeBinning_p, timeBinning_p + numberOfBins);
	timeBinningTmp[numberOfBins - 1] = timeBinningTmp[numberOfBins - 2]
			+ timeBinningTmp[1] - timeBinningTmp[0];
	m_localWorkspace->dataX(0).assign(timeBinningTmp.begin(),
			timeBinningTmp.end());

	Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube);
	size_t spec = 0;
	for (size_t i = 0; i < m_numberOfTubes; ++i) {
		for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
			if (spec > 0) {
				// just copy the time binning axis to every spectra
				m_localWorkspace->dataX(spec) = m_localWorkspace->readX(0);
			}
			// Assign Y
			int* data_p = &data(static_cast<int>(i), static_cast<int>(j));
			m_localWorkspace->dataY(spec).assign(data_p,
					data_p + m_numberOfChannels);

			// Assign Error
			MantidVec& E = m_localWorkspace->dataE(spec);
			std::transform(data_p, data_p + m_numberOfChannels, E.begin(),
					LoadSINQ::calculateError);

			++spec;
			progress.report();
		}
	}

	g_log.debug() << "Data loading inti WS done...." << std::endl;
}

void LoadSINQ::loadRunDetails(NXEntry & entry) {

	API::Run & runDetails = m_localWorkspace->mutableRun();

//	int runNum = entry.getInt("run_number");
//	std::string run_num = boost::lexical_cast<std::string>(runNum);
//	runDetails.addProperty("run_number", run_num);

	std::string start_time = entry.getString("start_time");
	//start_time = getDateTimeInIsoFormat(start_time);
	runDetails.addProperty("run_start", start_time);

	std::string end_time = entry.getString("end_time");
	//end_time = getDateTimeInIsoFormat(end_time);
	runDetails.addProperty("run_end", end_time);

	double wavelength = entry.getFloat(
			m_nexusInstrumentEntryName + "/monochromator/lambda");
	runDetails.addProperty<double>("wavelength", wavelength);

	double energy = entry.getFloat(
			m_nexusInstrumentEntryName + "/monochromator/energy");
	runDetails.addProperty<double>("Ei", energy, true); //overwrite

	std::string title = entry.getString("title");
	runDetails.addProperty("title", title);
	m_localWorkspace->setTitle(title);

}

/*
 * Load data about the Experiment.
 *
 * TODO: This is very incomplete. In ISIS they much more info in the nexus file than ILL.
 *
 * @param entry :: The Nexus entry
 */
void LoadSINQ::loadExperimentDetails(NXEntry & entry) {

	std::string name = boost::lexical_cast<std::string>(
			entry.getFloat("sample/name"));
	m_localWorkspace->mutableSample().setName(name);

}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadSINQ::runLoadInstrument() {

	IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

	// Now execute the Child Algorithm. Catch and log any error, but don't stop.
	try {

		// TODO: depending on the m_numberOfPixelsPerTube we might need to load a different IDF

		loadInst->setPropertyValue("InstrumentName", m_instrumentName);
		loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",
				m_localWorkspace);
		loadInst->execute();
	} catch (...) {
		g_log.information("Cannot load the instrument definition.");
	}
}

} // namespace DataHandling
} // namespace Mantid
