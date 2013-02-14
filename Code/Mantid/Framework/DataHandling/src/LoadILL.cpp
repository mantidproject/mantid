/*WIKI* 

 Loads an ILL nexus file into a [[Workspace2D]] with the given name.


 *WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"
//#include "MantidAPI/NumericAxis.h"
//#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument.h"

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

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILL)

//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadILL)

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

//---------------------------------------------------
// Private member functions
//---------------------------------------------------

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

	setInstrumentName(entry);

	loadTimeDetails(entry);

	initWorkSpace(entry);
	loadDataIntoTheWorkSpace(entry);

	loadRunDetails(entry);
	loadExperimentDetails(entry);

	// load the instrument from the IDF if it exists
	runLoadInstrument();

	// Set the output workspace property
	setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Sets the member variable to instrument name
 *
 * @param entry :: The Nexus entry
 *
 */
void LoadILL::setInstrumentName(NeXus::NXEntry& entry) {

	// Old format: /entry0/IN5/name
	// New format: /entry0/instrument/name

	// Ugly way of getting Instrument Name
	// Instrument name is in: entry0/<NXinstrument>/name
	std::vector<NXClassInfo> v = entry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			m_nexusInstrumentEntryName = it->nxname;
			std::string insNamePath = m_nexusInstrumentEntryName + "/name";
			m_instrumentName = entry.getString(insNamePath);
			g_log.debug() << "Instrument Name: " << m_instrumentName
					<< " in NxPath: " << insNamePath << std::endl;
			break;
		}
	}
	if (m_instrumentName == "") {
		g_log.error();
		throw std::runtime_error(
				"Cannot read the instrument name from the Nexus file!");
	}

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
	// TODO : I added here m_numberOfHistograms+1 : +1 for the monitor!
	// Might need to get this value from the number of monitors in the Nexus file
	// params:
	// workspace type,
	// total number of spectra + number of monitors,
	// bin boundaries = m_numberOfChannels + 1
	// Z/time dimension
	m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",
			m_numberOfHistograms + 1, m_numberOfChannels + 1,
			m_numberOfChannels);
	m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(
			"TOF");
	m_localWorkspace->setYUnitLabel("Counts");

}

/**
 *  Get Monitor Data from the Nexus file
 *
 *   @param entry :: The Nexus entry
 *   @return vector with the monitor spectra
 *
 *   TODO: For now only for 1 monitor, do it generic in the future
 *
 *   @param entry :: The Nexus entry
 *
 */
std::vector<int> LoadILL::getMonitorData(NeXus::NXEntry& entry) {

	NXData dataMon = entry.openNXData("monitor/data");
	NXInt data = dataMon.openIntData();
	size_t dim0 = static_cast<size_t>(data.dim2());
	data.load();

	std::vector<int> v;
	for (size_t i = 0; i < dim0; ++i) {
		//std::cout << "pos = " << i << " :: Value : ";
		int* data_p = &data(0, 0, static_cast<int>(i));
		v.push_back(*data_p);
		//std::cout << *data_p << std::endl;
	}
	g_log.debug() << "Monitor data loaded size() = " << v.size() << std::endl;
	// vector is small. no much overhead in returning a copy of it
	return v;
}

/**
 * Load member variables with the time details of IN5
 *
 * @param entry :: first NXEntry entry in the nexus file
 *
 */
void LoadILL::loadTimeDetails(NeXus::NXEntry& entry) {

	m_wavelength = entry.getFloat("wavelength");
	m_monitorElasticPeakPosition = entry.getInt("monitor/elasticpeak");

	NXDiskChopper chopper =
			entry.openNXInstrument(m_nexusInstrumentEntryName).openNXDiskChopper(
					"FO");

	NXFloat f0ChopperSpeedRPM = chopper.openRotationSpeed();
	f0ChopperSpeedRPM.load();

	// Angular velocity [deg/s] and period [s]
	double f0ChopperAngularVelocity = 360. * (f0ChopperSpeedRPM[0] / 60.);
	double pickupOffsetAngle = 90.; // Pick-up to windows offset angle [deg.]
	// Time from pickup to the window opening [mu-sec]
	m_timePickupToOpening = pickupOffsetAngle / f0ChopperAngularVelocity * 1e6;

	NXFloat time_of_flight_data = entry.openNXFloat("monitor/time_of_flight");
	time_of_flight_data.load();

	// The entry "monitor/time_of_flight", has 3 fields:
	// channel width , number of channels, Time of flight delay
	m_channelWidth = time_of_flight_data[0];
	m_timeOfFlightDelay = time_of_flight_data[2];

	g_log.debug("Nexus Data:");
	g_log.debug() << " ChannelWidth: " << m_channelWidth << std::endl;
	g_log.debug() << " Wavelength: " << m_wavelength << std::endl;
	g_log.debug() << " ElasticPeakPosition: " << m_monitorElasticPeakPosition
			<< std::endl;
	g_log.debug() << " timeOfFlightDelay: " << m_timeOfFlightDelay << std::endl;
	g_log.debug() << " timePickupToOpening: " << m_timePickupToOpening
			<< std::endl;

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
std::string LoadILL::getDateTimeInIsoFormat(std::string dateToParse) {
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

/*
 * Find a peak position in a array
 *
 * Adapted from : http://www.billauer.co.il/peakdet.html
 *
 * @param :: v - vector
 * @param :: delta - A point is considered a maximum peak if it has the maximal
 *         value, and was preceded (to the left) by a value lower by delta.
 *
 * @return a vector with positions for maximum values
 *
 */
template<class T>
std::vector<int> LoadILL::peakSearchPosition(const std::vector<T> &v,
		int delta = 5) {

	std::vector<int> maxPositions;
	std::vector<int> minPositions;

	int mn = std::numeric_limits<T>::max();
	int mx = std::numeric_limits<T>::min();
	int mnpos = -1, mxpos = -1;

	bool lookformax = true;

	for (unsigned int i = 0; i < v.size(); i++) {
		T thisV = v[i];
		if (thisV > mx) {
			mx = thisV;
			mxpos = i;
		}
		if (thisV < mn) {
			mn = thisV;
			mnpos = i;
		}

		if (lookformax) {
			if (thisV < mx - delta) {
				maxPositions.push_back(mxpos);
				mn = thisV;
				mnpos = i;
				lookformax = false;
			}
		} else {
			if (thisV > mn + delta) {
				minPositions.push_back(mnpos);
				mx = thisV;
				mxpos = i;
				lookformax = true;
			}
		}
	}

	return maxPositions;
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
	start_time = getDateTimeInIsoFormat(start_time);
	runDetails.addProperty("run_start", start_time);

	std::string end_time = entry.getString("end_time");
	end_time = getDateTimeInIsoFormat(end_time);
	runDetails.addProperty("run_end", end_time);

	//m_wavelength = entry.getFloat("wavelength");
	std::string wavelength = boost::lexical_cast<std::string>(m_wavelength);
	runDetails.addProperty("wavelength", wavelength);

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

/*
 * This function calculates the mode in a vector
 *
 * Adapted from : http://www.hdelossantos.com/2010/10/16/mean-median-and-mode-in-c/
 *
 * @param data :: input vector
 * @return a new vector with the mode (can be more than one value!!!)
 *
 */
template<typename T> std::vector<T> LoadILL::mode(std::vector<T> &data) {
	std::vector<T> tmp_vector;

	if (data.size() > 0) {
		std::sort(data.begin(), data.end());
		std::vector<int>::iterator i;
		i = data.begin();

		int highest_mode = 0;
		int highest_mode_count = 0;
		int current_mode = 0;
		int current_count = 0;

		// Iterate through the vector
		while (i != data.end()) {
			int tmp = *i;

			if (current_count == 0) {
				current_mode = tmp;
			}

			if (tmp == current_mode) {
				current_count++;
			} else if (tmp > current_mode) {

				// Check if the current mode is greater than the highest
				if (current_count > highest_mode_count) {
					// Make the current mode the highest
					highest_mode = current_mode;
					highest_mode_count = current_count;

					// Clear the vector
					tmp_vector.clear();

					// Add the highest value to the vector
					tmp_vector.push_back(highest_mode);

					// Set current to tmp
					current_mode = tmp;
					current_count = 1;
				}

				// In case multiple modes
				else if (current_count == highest_mode_count) {
					// Set the highest mode to current
					highest_mode = current_mode;
					highest_mode_count = current_count;

					// Add the current mode to the vector
					tmp_vector.push_back(current_mode);

					current_mode = tmp;
					current_count = 1;
				}

				else {
					// Set tmp to current
					current_mode = tmp;
					current_count = 1;
				}
			} else {
				// Shouldn't need to do anything if tmp < current_mode
			}

			i++;
		}
	}

	return tmp_vector;
}

/**
 * Loads monitor data from the nexus file and returns the
 * index position of the peak
 * In case there are 2 peaks (which should be impossible in the monitor)
 * returns the highest one.
 *
 * @param monitorData :: A vector with the contents of the monitor spectrum
 * @return the monitor Peak Position
 */
int LoadILL::getMonitorElasticPeakPosition(
		const std::vector<int> &monitorData) {

	std::vector<int> peakPositions = peakSearchPosition<int>(monitorData);
	auto it = std::max_element(peakPositions.begin(), peakPositions.end());
	int monitorPeakPosition = *it;
	return monitorPeakPosition;
}

/**
 * Gets the experimental Elastic Peak Position in the dectector
 * as the value parsed from the nexus file might be wrong.
 *
 * It gets a few spectra in the equatorial line of the detector,
 * calculates the peak position and puts all in a vector.
 *
 * In case there are several peaks in a spectrum, it considers
 * the highest one.
 *
 * @param :: spectra data
 * @return detector Elastic Peak Position
 */
int LoadILL::getDetectorElasticPeakPosition(const NeXus::NXInt &data) {

	std::vector<int> listOfFoundEPP;
	// j = index in the equatorial line (256/2=128)
	// both index 127 and 128 are in the equatorial line
	size_t j = m_numberOfPixelsPerTube / 2;
	// ignore the first tubes and the last ones to avoid the beamstop
	//for (size_t i = 20; i < m_numberOfTubes - 50; i = i + 4) {
	for (size_t i = 1; i < m_numberOfTubes-30; i++) {
		int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
		std::vector<int> thisSpectrum(data_p, data_p + m_numberOfChannels);
		std::vector<int> peakPositions = peakSearchPosition<int>(thisSpectrum);
		if (peakPositions.size() > 0) {
			int it = *std::max_element(peakPositions.begin(),
					peakPositions.end());
			listOfFoundEPP.push_back(it);
		}
//		g_log.debug() << "Tube (" << i << ") : Spectra (" << 256 * i - 128 << ") : ";
//		for (auto v : peakPositions)
//			g_log.debug() << v << " ";
//		g_log.debug() << "\n";

	}

	int calculatedDetectorElasticPeakPosition;
	if (listOfFoundEPP.size() <= 0 || mode(listOfFoundEPP).size() <= 0) {
		g_log.warning()
				<< "No Elastic peak position found! Assuming the EPP in the Nexus file: "
				<< m_monitorElasticPeakPosition << std::endl;
		calculatedDetectorElasticPeakPosition = m_monitorElasticPeakPosition;
	} else {
		calculatedDetectorElasticPeakPosition = mode(listOfFoundEPP)[0]; // in case of various, pick the first [0]
		g_log.debug() << "Calculated Detector EPP: "
				<< calculatedDetectorElasticPeakPosition;
		g_log.debug() << " :: Read EPP from the nexus file: "
				<< m_monitorElasticPeakPosition << std::endl;
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

	// get Monitor data
	std::vector<int> monitorData = getMonitorData(entry);
	int monitorPeakPosition = getMonitorElasticPeakPosition(monitorData);

	/**
	 * Theoretical calculations for cross checking
	 *
	 * Neutron velocity: v = (6.626e-34 m² kg / s) / (1.675e-27 kg) (\\lambda m)
	 * t = distance / v
	 * TODO: delete this when in production
	 */
	double distanceSourceSample = 2.10855; //meters
	double distanceSourceMonitor = 0.85560; //meters
	double distanceSampleDetector = 4; //meters - equatorial line of the detector

	double tElastSampleDetector = (distanceSampleDetector)
			/ (6.626e-34 / (1.675e-27 * m_wavelength * 1e-10));
	double tElastSourceSample = (distanceSourceSample)
			/ (6.626e-34 / (1.675e-27 * m_wavelength * 1e-10));
	double tElastSourceMonitor = (distanceSourceMonitor)
			/ (6.626e-34 / (1.675e-27 * m_wavelength * 1e-10));

	// convert seconds to microseconds
	tElastSampleDetector *= 1e6;
	tElastSourceSample *= 1e6;
	tElastSourceMonitor *= 1e6;

	g_log.debug("------------ Theoretical TOFs (microsecs) -----------------");
	g_log.debug() << "t Sample -> Detector: " << tElastSampleDetector
			<< std::endl;
	g_log.debug() << "t Source -> Monitor:  " << tElastSourceMonitor
			<< std::endl;
	g_log.debug() << "t Source -> Sample:   " << tElastSourceSample
			<< std::endl;
	g_log.debug("-----------------------------------------------------------");

	/*
	 * Monitor
	 * Match the theoretical monitor EPP with the read EPP
	 * TODO:
	 * I hate this but the EPP in  the monitor has to be the theoretical value
	 * Let's make time start at zero and find a ficticious channel witdh
	 * TODO: This tElastSourceMonitor must be calculated from a parameter and not an hardcoded value
	 * TODO: Monitor info doesn't look useful for ILL... delete it.
	 */
	std::vector<double> monitorTofBins(monitorData.size() + 1);
	double monitorFakeChannelWidth = tElastSourceMonitor / monitorPeakPosition;
	for (size_t i = 1; i < monitorData.size() + 1; ++i) {
		monitorTofBins[i] = static_cast<double>(i) * monitorFakeChannelWidth;
	}
	// assign the calculated tof bins to the 0 spectra X axis
	m_localWorkspace->dataX(0).assign(monitorTofBins.begin(),
			monitorTofBins.end());
	m_localWorkspace->dataY(0).assign(monitorData.begin(), monitorData.end());

	/*
	 * Detector: Find real elastic peak in the detector.
	 * Looks for a few elastic peaks on the equatorial line of the detector.
	 */
	int calculatedDetectorElasticPeakPosition = getDetectorElasticPeakPosition(
			data);

	double tElastChannel = static_cast<double>(m_monitorElasticPeakPosition)
			* m_channelWidth;
	double tPickupToDetectorElastic = m_timeOfFlightDelay + tElastChannel;

	// Calculate the real tof from the source and put it in tof array
	std::vector<double> detectorTofBins(m_numberOfChannels + 1);
	for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
		detectorTofBins[i] = tPickupToDetectorElastic
				- m_timePickupToOpening
				+ m_channelWidth
						* static_cast<double>(static_cast<int>(i)
								- calculatedDetectorElasticPeakPosition);

	}
	//g_log.debug() << "Detector TOF bins: ";
	//for (auto i : detectorTofBins) g_log.debug() << i << " ";
	//g_log.debug() << "\n";

	g_log.information() << "T1+T2 : Theoretical = "
			<< tElastSourceSample + tElastSampleDetector;
	g_log.information() << " ::  Calculated bin = ["
			<< detectorTofBins[calculatedDetectorElasticPeakPosition]
			<< "," << detectorTofBins[calculatedDetectorElasticPeakPosition +1]
			<< "]" << std::endl;


	// assign the calculated tof bins to the 0 spectra X axis
	m_localWorkspace->dataX(1).assign(detectorTofBins.begin(),
			detectorTofBins.end());

	//TODO: start in 1 as 0 is the monitor. Change this!
	Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube);
	size_t spec = 1;
	for (size_t i = 0; i < m_numberOfTubes; ++i){
		for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
			if (spec > 1) {
				// just copy the time binning axis to every spectra
				m_localWorkspace->dataX(spec) = m_localWorkspace->readX(1);
			}
			int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
			m_localWorkspace->dataY(spec).assign(data_p,
					data_p + m_numberOfChannels);
			++spec;
			progress.report();
		}
	}
}

/**
 * This method does a quick file check by checking the no.of bytes read nread params and header buffer
 *  @param filePath- path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadILL::quickFileCheck(const std::string& filePath, size_t nread,
		const file_header& header) {
	std::string extn = extension(filePath);
	bool bnexs(false);
	(!extn.compare("nxs") || !extn.compare("nx5")) ? bnexs = true : bnexs =
																false;
	/*
	 * HDF files have magic cookie in the first 4 bytes
	 */
	if (((nread >= sizeof(unsigned))
			&& (ntohl(header.four_bytes) == g_hdf_cookie)) || bnexs) {
		//hdf
		return true;
	} else if ((nread >= sizeof(g_hdf5_signature))
			&& (!memcmp(header.full_hdr, g_hdf5_signature,
					sizeof(g_hdf5_signature)))) {
		//hdf5
		return true;
	}
	return false;
}

/**
 * Checks the file by opening it and reading few lines
 * @param filePath :: name of the file inluding its path
 * @return an integer value how much this algorithm can load the file 
 */
int LoadILL::fileCheck(const std::string& filePath) {
	// Create the root Nexus class
	NXRoot root(filePath);
	NXEntry entry = root.openFirstEntry();
	if (entry.containsGroup("IN5"))
		return 80;

	return 0;
}

/**
 * Run the Child Algorithm LoadInstrument.
 * @param workspace :: The workspace to assign the loaded instrument to.
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
