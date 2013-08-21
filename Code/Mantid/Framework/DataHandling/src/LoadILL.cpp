/*WIKI* 

 Loads an ILL nexus file into a [[Workspace2D]] with the given name.

 To date this algorithm only supports: IN5


 *WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadHelper.h"

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

    DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILL);
;

/**
 * tostring operator to print the contents of NXClassInfo
 *
 * TODO : This has to go somewhere else
 */
std::ostream& operator<<(std::ostream &strm, const NXClassInfo &c) {
	return strm << "NXClassInfo :: nxname: " << c.nxname << " , nxclass: "
			<< c.nxclass;
}

/// Sets documentation strings for this algorithm
void LoadILL::initDocs() {
	this->setWikiSummary("Loads a ILL nexus file. ");
	this->setOptionalMessage("Loads a ILL nexus file.");
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadILL::confidence(Kernel::NexusDescriptor & descriptor) const {

	// fields existent only at the ILL
	if (descriptor.pathExists("/entry0/wavelength")
			&& descriptor.pathExists("/entry0/experiment_identifier")
			&& descriptor.pathExists("/entry0/mode")) {
		return 80;
	} else {
		return 0;
	}
}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------

LoadILL::LoadILL() :
		API::IFileLoader<Kernel::NexusDescriptor>() {

	m_instrumentName = "";
	m_wavelength = 0;
	m_channelWidth = 0;
	m_numberOfChannels = 0;
	m_numberOfHistograms = 0;
	m_supportedInstruments.push_back("IN4");
	m_supportedInstruments.push_back("IN5");
	m_supportedInstruments.push_back("IN6");

}

/**
 * Initialise the algorithm
 */
void LoadILL::init() {
	declareProperty(
			new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
			"Name of the SPE file to load");
	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"The name to use for the output workspace");

}

/**
 * Execute the algorithm
 */
void LoadILL::exec() {
	// Retrieve filename
	m_filename = getPropertyValue("Filename");
	// open the root node
	NXRoot root(m_filename);
	// find the first entry
	NXEntry entry = root.openFirstEntry();

	loadInstrumentDetails(entry);
	loadTimeDetails(entry);
	initWorkSpace(entry);

	runLoadInstrument(); // just to get IDF contents
	initInstrumentSpecific();

	loadDataIntoTheWorkSpace(entry);

	loadRunDetails(entry);
	loadExperimentDetails(entry);

	// load the instrument from the IDF if it exists
	runLoadInstrument();

	// Set the output workspace property
	setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 *
 */
void LoadILL::loadInstrumentDetails(NeXus::NXEntry& firstEntry) {

	m_instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);

	if (m_instrumentPath == "") {
		throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
	}
	m_instrumentName = m_loader.getStringFromNexusPath(firstEntry,
			m_instrumentPath + "/name");
	g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param entry :: The Nexus entry
 *
 */
void LoadILL::initWorkSpace(NeXus::NXEntry& entry) {

	// read in the data
	NXData dataGroup = entry.openNXData("data");
	NXInt data = dataGroup.openIntData();

	m_numberOfTubes = static_cast<size_t>(data.dim0());
	m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
	m_numberOfChannels = static_cast<size_t>(data.dim2());

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

/**
 * Function to do specific instrument stuff
 *
 */
void LoadILL::initInstrumentSpecific() {
	m_l1 = m_loader.getL1(m_localWorkspace);
	// this will be mainly for IN5 (flat PSD detector)
	m_l2 = m_loader.getInstrumentProperty(m_localWorkspace,"l2");
	if (m_l2 == EMPTY_DBL()) {
		g_log.debug("Calculating L2 from the IDF.");
		m_l2 = m_loader.getL2(m_localWorkspace);
	}
}

/**
 * Load the time details from the nexus file.
 * @param entry :: The Nexus entry
 */
void LoadILL::loadTimeDetails(NeXus::NXEntry& entry) {

	m_wavelength = entry.getFloat("wavelength");

	// Monitor can be monitor (IN5) or monitor1 (IN6)
	std::string monitorName;
	if (entry.containsGroup("monitor"))
		monitorName = "monitor";
	else if (entry.containsGroup("monitor1"))
		monitorName = "monitor1";
	else {
		std::string message("Cannot find monitor[1] in the Nexus file!");
		g_log.error(message);
		throw std::runtime_error(message);
	}

	m_monitorElasticPeakPosition = entry.getInt(monitorName + "/elasticpeak");

	NXFloat time_of_flight_data = entry.openNXFloat(
			monitorName + "/time_of_flight");
	time_of_flight_data.load();

	// The entry "monitor/time_of_flight", has 3 fields:
	// channel width , number of channels, Time of flight delay
	m_channelWidth = time_of_flight_data[0];
//	m_timeOfFlightDelay = time_of_flight_data[2];

	g_log.debug("Nexus Data:");
	g_log.debug() << " ChannelWidth: " << m_channelWidth << std::endl;
	g_log.debug() << " Wavelength: " << m_wavelength << std::endl;
	g_log.debug() << " ElasticPeakPosition: " << m_monitorElasticPeakPosition
			<< std::endl;

}



/**
 * Load information about the run.
 * People from ISIs have this... Let's do the same for the ILL.
 * TODO: They also have a lot of info in XML format!
 *
 * @param entry :: The Nexus entry
 */
void LoadILL::loadRunDetails(NXEntry & entry) {

	API::Run & runDetails = m_localWorkspace->mutableRun();

	int runNum = entry.getInt("run_number");
	std::string run_num = boost::lexical_cast<std::string>(runNum);
	runDetails.addProperty("run_number", run_num);

	std::string start_time = entry.getString("start_time");
	start_time = m_loader.dateTimeInIsoFormat(start_time);
	runDetails.addProperty("run_start", start_time);

	std::string end_time = entry.getString("end_time");
	end_time = m_loader.dateTimeInIsoFormat(end_time);
	runDetails.addProperty("run_end", end_time);

	//m_wavelength = entry.getFloat("wavelength");
	std::string wavelength = boost::lexical_cast<std::string>(m_wavelength);
	//runDetails.addProperty<double>("wavelength", m_wavelength);
	runDetails.addProperty("wavelength", wavelength);
	double ei = m_loader.calculateEnergy(m_wavelength);
	runDetails.addProperty<double>("Ei", ei, true); //overwrite
	//std::string ei_str = boost::lexical_cast<std::string>(ei);
	//runDetails.addProperty("Ei", ei_str);

	std::string duration = boost::lexical_cast<std::string>(
			entry.getFloat("duration"));
	runDetails.addProperty("duration", duration);

	std::string preset = boost::lexical_cast<std::string>(
			entry.getFloat("preset"));
	runDetails.addProperty("preset", duration);

	std::string mode = entry.getString("mode");
	runDetails.addProperty("mode", mode);

	std::string title = entry.getString("title");
	runDetails.addProperty("title", title);
	m_localWorkspace->setTitle(title);

	// Below : This should belong to sample ???
	std::string experiment_identifier = entry.getString(
			"experiment_identifier");
	runDetails.addProperty("experiment_identifier", experiment_identifier);
	m_localWorkspace->mutableSample().setName(experiment_identifier);

	std::string temperature = boost::lexical_cast<std::string>(
			entry.getFloat("sample/temperature"));
	runDetails.addProperty("temperature", temperature);
}

/*
 * Load data about the Experiment.
 *
 * TODO: This is very incomplete. In ISIS they much more info in the nexus file than ILL.
 *
 * @param entry :: The Nexus entry
 */
void LoadILL::loadExperimentDetails(NXEntry & entry) {

	// TODO: Do the rest
	// Pick out the geometry information

	std::string description = boost::lexical_cast<std::string>(
			entry.getFloat("sample/description"));

	m_localWorkspace->mutableSample().setName(description);

//	m_localWorkspace->mutableSample().setThickness(static_cast<double> (isis_raw->spb.e_thick));
//	m_localWorkspace->mutableSample().setHeight(static_cast<double> (isis_raw->spb.e_height));
//	m_localWorkspace->mutableSample().setWidth(static_cast<double> (isis_raw->spb.e_width));

}

/**
 * Gets the experimental Elastic Peak Position in the dectector
 * as the value parsed from the nexus file might be wrong.
 *
 * It gets a few spectra in the equatorial line of the detector,
 * sum them up and finds the maximum = the Elastic peak
 *
 *
 * @param data :: spectra data
 * @return detector Elastic Peak Position
 */
int LoadILL::getDetectorElasticPeakPosition(const NeXus::NXInt &data) {

	std::vector<int> listOfFoundEPP;
	// j = index in the equatorial line (256/2=128)
	// both index 127 and 128 are in the equatorial line
	size_t j = m_numberOfPixelsPerTube / 2;

	// ignore the first tubes and the last ones to avoid the beamstop
	//get limits in the m_numberOfTubes
	size_t tubesToRemove = m_numberOfTubes / 7;

	std::vector<int> cumulatedSumOfSpectras(m_numberOfChannels, 0);
	for (size_t i = tubesToRemove; i < m_numberOfTubes - tubesToRemove; i++) {
		int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
		std::vector<int> thisSpectrum(data_p, data_p + m_numberOfChannels);
		// sum spectras
		std::transform(thisSpectrum.begin(), thisSpectrum.end(),
				cumulatedSumOfSpectras.begin(), cumulatedSumOfSpectras.begin(),
				std::plus<int>());
	}
	auto it = std::max_element(cumulatedSumOfSpectras.begin(),
			cumulatedSumOfSpectras.end());

	int calculatedDetectorElasticPeakPosition;
	if (it == cumulatedSumOfSpectras.end()) {
		g_log.warning()
				<< "No Elastic peak position found! Assuming the EPP in the Nexus file: "
				<< m_monitorElasticPeakPosition << std::endl;
		calculatedDetectorElasticPeakPosition = m_monitorElasticPeakPosition;

	} else {
		//calculatedDetectorElasticPeakPosition = *it;
		calculatedDetectorElasticPeakPosition = static_cast<int>(std::distance(
				cumulatedSumOfSpectras.begin(), it));

		if (calculatedDetectorElasticPeakPosition == 0) {
			g_log.warning()
					<< "Elastic peak position is ZERO Assuming the EPP in the Nexus file: "
					<< m_monitorElasticPeakPosition << std::endl;
			calculatedDetectorElasticPeakPosition =
					m_monitorElasticPeakPosition;

		} else {
			g_log.debug() << "Calculated Detector EPP: "
					<< calculatedDetectorElasticPeakPosition;
			g_log.debug() << " :: Read EPP from the nexus file: "
					<< m_monitorElasticPeakPosition << std::endl;
		}
	}
	return calculatedDetectorElasticPeakPosition;

}
/**
 * Loads all the spectra into the workspace, including that from the monitor
 *
 * @param entry :: The Nexus entry
 */
void LoadILL::loadDataIntoTheWorkSpace(NeXus::NXEntry& entry) {

	// read in the data
	NXData dataGroup = entry.openNXData("data");
	NXInt data = dataGroup.openIntData();
	// load the counts from the file into memory
	data.load();
	/*
	 * Detector: Find real elastic peak in the detector.
	 * Looks for a few elastic peaks on the equatorial line of the detector.
	 */
	int calculatedDetectorElasticPeakPosition = getDetectorElasticPeakPosition(
			data);

	double theoreticalElasticTOF = (m_loader.calculateTOF(m_l1,m_wavelength) + m_loader.calculateTOF(m_l2,m_wavelength))
			* 1e6; //microsecs

	// Calculate the real tof (t1+t2) put it in tof array
	std::vector<double> detectorTofBins(m_numberOfChannels + 1);
	for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
		detectorTofBins[i] = theoreticalElasticTOF
				+ m_channelWidth
						* static_cast<double>(static_cast<int>(i)
								- calculatedDetectorElasticPeakPosition)
				- m_channelWidth / 2; // to make sure the bin is in the middle of the elastic peak

	}
	//g_log.debug() << "Detector TOF bins: ";
	//for (auto i : detectorTofBins) g_log.debug() << i << " ";
	//g_log.debug() << "\n";

	g_log.information() << "T1+T2 : Theoretical = " << theoreticalElasticTOF;
	g_log.information() << " ::  Calculated bin = ["
			<< detectorTofBins[calculatedDetectorElasticPeakPosition] << ","
			<< detectorTofBins[calculatedDetectorElasticPeakPosition + 1] << "]"
			<< std::endl;

	// Assign calculated bins to first X axis
	m_localWorkspace->dataX(0).assign(detectorTofBins.begin(),
			detectorTofBins.end());

	Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube);
	size_t spec = 0;
	for (size_t i = 0; i < m_numberOfTubes; ++i) {
		for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
			if (spec > 0) {
				// just copy the time binning axis to every spectra
				m_localWorkspace->dataX(spec) = m_localWorkspace->readX(0);
			}
			// Assign Y
			int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
			m_localWorkspace->dataY(spec).assign(data_p,
					data_p + m_numberOfChannels);

			// Assign Error
			MantidVec& E = m_localWorkspace->dataE(spec);
			std::transform(data_p, data_p + m_numberOfChannels, E.begin(),
					LoadILL::calculateError);

			++spec;
			progress.report();
		}
	}
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadILL::runLoadInstrument() {

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
