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

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILL)

//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadILL)

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
 * tostring operator to print the contents of NXClassInfo
 */
std::ostream& operator<<(std::ostream &strm, const NXClassInfo &c) {
	return strm << "NXClassInfo :: nxname: " << c.nxname << " , nxclass: "
			<< c.nxclass;
}

/**
 * Execute the algorithm
 */
void LoadILL::exec() {
	// Retrieve filename and try to open the file
	m_filename = getPropertyValue("Filename");

	MatrixWorkspace_sptr workspace;

	// open the root node
	NXRoot root(m_filename);
	// find the first entry
	NXEntry entry = root.openFirstEntry();

	NXData dataGroup = loadNexusFile(entry);

	loadData(dataGroup, workspace);

	// load the instrument from the IDF if it exists
	runLoadInstrument(workspace);

	// Correct the data
	correctData(workspace);

	// Set the output workspace property
	setProperty("OutputWorkspace", workspace);
}

/**
 * Load the nexus details
 * @param entry :: first NXEntry entry in the nexus file
 */
NeXus::NXData LoadILL::loadNexusFile(NXEntry& entry) {

	// Ugly way of getting Instrument Name
	// Instrument name is in: entry0/<NXinstrument>/name
	std::vector<NXClassInfo> v = entry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			std::string insNamePath = it->nxname + "/name";
			m_instrumentName = entry.getString(insNamePath);
			g_log.debug() << "Instrument Name: " << m_instrumentName
					<< std::endl;
			break;
		}
	}
	if (m_instrumentName == "") {
		g_log.error();
		throw std::runtime_error(
				"Cannot read the instrument name from the Nexus file!");
	}

	title = entry.getString("title");
	wavelength = entry.getFloat("wavelength");
	elasticPeakPosition = entry.getInt("monitor/elasticpeak");

	NXDiskChopper chopper =
			entry.openNXInstrument(m_instrumentName).openNXDiskChopper("FO");
	NXFloat f0ChopperSpeedRPM = chopper.openRotationSpeed();
	f0ChopperSpeedRPM.load();

	// Angular velocity [deg/s] and period [s]
	double f0ChopperAngularVelocity = 360. * (f0ChopperSpeedRPM[0] / 60.);
	double pickupOffsetAngle = 90.; // Pick-up to windows offset angle [deg.]
	// Time from pickup to the window opening [mu-sec]
	timePickupToOpening = pickupOffsetAngle / f0ChopperAngularVelocity * 1e6;

	NXFloat time_of_flight_data = entry.openNXFloat("monitor/time_of_flight");
	time_of_flight_data.load();

	// channel width , number of channels, Time of flight delay
	channelWidth = time_of_flight_data[0];
	timeOfFlightDelay = time_of_flight_data[2];

	g_log.debug("Nexus Data:");
	g_log.debug() << "NchannelWidth: " << channelWidth << std::endl;
	g_log.debug() << "wavelength: " << wavelength << std::endl;
	g_log.debug() << "elasticPeakPosition: " << elasticPeakPosition
			<< std::endl;
	g_log.debug() << "timeOfFlightDelay: " << timeOfFlightDelay << std::endl;
	g_log.debug() << "timePickupToOpening: " << timePickupToOpening
			<< std::endl;

	// read in the data
	NXData dataGroup = entry.openNXData("data");
	return dataGroup;
}

/**
 * Load the counts.
 * @param data :: The dataset with the counts (signal=1).
 * @param workspace :: The workspace to write the data in.
 */
void LoadILL::loadData(NeXus::NXData& dataGroup,
		API::MatrixWorkspace_sptr& workspace) {
	NXInt data = dataGroup.openIntData();

	size_t dim0 = static_cast<size_t>(data.dim0());
	size_t dim1 = static_cast<size_t>(data.dim1());
	// dim0 * dim1 is the total number of detectors
	size_t nhist = dim0 * dim1;
	// dim2 is the number of time bins
	size_t nbins = data.dim2();

	// Now create the output workspace
	workspace = WorkspaceFactory::Instance().create("Workspace2D", nhist,
			nbins + 1, nbins);
	workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
	workspace->setYUnitLabel("Counts");
	workspace->setTitle(title);

	// put the time of flight data into the x vector
	std::vector<double> tof(nbins + 1);
	// Calculate the time of flight from channel number and channel width
	// Neutron velocity: v = (6.626e-34 m² kg / s) / (1.675e-27 kg) (\lambda m)
	// t = distance / v

	//TODO: get this from parameters file: 0.8656
//	double distanceSourceSample = 2.10855; //meters
//	double distanceSampleDetector = 4;
//
//	double tElastSampleDetector = (distanceSampleDetector)
//			/ (6.626e-34 / (1.675e-27 * wavelength * 1e-10));
//	double tElastSourceSample = (distanceSourceSample)
//			/ (6.626e-34 / (1.675e-27 * wavelength * 1e-10));

	//distance must be t1+t2: 865.6mm + 4m ????
	//Mantid uses t1 (source in IDF) for calculations
	// double telast = wavelength * (distanceSampleDetector + 0.8656) / 3956.035;

//	// convert seconds to microseconds
//	tElastSampleDetector *= 1e6;
//	tElastSourceSample *= 1e6;
//
//	g_log.debug("Theoretical Values:");
//	g_log.debug() << "tElastSourceSample: " << tElastSourceSample
//			<< " microsecs" << std::endl;
//	g_log.debug() << "tElastSampleDetector: " << tElastSampleDetector
//			<< " microsecs" << std::endl;

	double tElastChannel = static_cast<double>(elasticPeakPosition)
			* channelWidth;
	double tPickupToDetectorElastic = timeOfFlightDelay + tElastChannel;

	g_log.debug("Calculated Values:");
	g_log.debug() << "tElastChannel: " << tElastChannel << std::endl;
	g_log.debug() << "timePickupToDetectorElastic: " << tPickupToDetectorElastic
			<< std::endl;

	// load the counts from the file into memory
	data.load();

	// have to find the real elastic peak
	// Select a h = middle detector
	// sweap across theta and find the average position

	for (size_t j = 0; j <= nbins; ++j) {
		// EPP channel number for which the elastic peak coincides
		tof[j] = tPickupToDetectorElastic - timePickupToOpening
				+ channelWidth
						* static_cast<double>(static_cast<int>(j)
								- elasticPeakPosition);

		// This WORKS
		/*tof[j] = channelWidth
		 * static_cast<double>(static_cast<int>(j) - elasticPeakPosition)
		 + tElastSourceSample + tElastSampleDetector;*/

	}
	workspace->dataX(0).assign(tof.begin(), tof.end());

	Progress progress(this, 0, 1, dim0 * dim1);

	size_t spec = 0;
	for (size_t i = 0; i < dim0; ++i)
		for (size_t j = 0; j < dim1; ++j) {
			if (spec > 0) {
				workspace->dataX(spec) = workspace->readX(0);
			}
			int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
			workspace->dataY(spec).assign(data_p, data_p + nbins);
			++spec;
			progress.report();
		}
}

/**
 * Apply specific ILL corrections to the data: top/bottom detector correction + find the fitted Elastic Peak Position
 *
 * TODO: This has be implemented in a separate algorithm
 * TODO: Still incomplete
 *
 *
 */
void LoadILL::correctData(API::MatrixWorkspace_sptr& workspace) {
	detid_t minId = 0;
	detid_t maxId = 0;
	workspace->getInstrument()->getMinMaxDetectorIDs(minId, maxId);

	size_t start = static_cast<size_t>(floor(static_cast<double>(minId + 256) / 2.0));

	// Move along the equatorial line of the detector
	// todo: I have to get these values from somewhere
	// theres 384 detectors let's use 200
	for (size_t i = start; i < 200*256 ; i=i+256 ) {

		const MantidVec& y = workspace->readY(i);
		// Dummy log to avoid warning: unused variable
		g_log.debug() << y.size();


	}
//
//
//	 const int numberOfSpectra = workspace->getNumberHistograms();
//	 for (int i = 0; i < numberOfSpectra; ++i)
//	 {
//	   MantidVec& x = workspace->readX(i); // Getting a reference (including the &) is more efficient
//	   MantidVec& y = workspace->readY(i);
//	   MantidVec& e = workspace->readE(i);
//	   // ... do something to the data ...
//	 }

	size_t testSpec = 48082;
	const MantidVec& x = workspace->readX(testSpec); // Getting a reference (including the &) is more efficient
	const MantidVec& y = workspace->readY(testSpec);
	const MantidVec& e = workspace->readE(testSpec);

	std::cout << "x size: " << x.size() << std::endl;
	std::cout << "x Max: " << *std::max_element(x.begin(), x.end()) << std::endl;
	std::cout << "x Min: " << *std::min_element(x.begin(), x.end()) << std::endl;

	std::cout << "y size: " << y.size() << std::endl;
	std::cout << "y Max: " << *std::max_element(y.begin(), y.end()) << std::endl;
	std::cout << "y Min: " << *std::min_element(y.begin(), y.end()) << std::endl;

	std::cout << "e size: " << e.size() << std::endl;
	std::cout << "e Max: " << *std::max_element(e.begin(), e.end()) << std::endl;
	std::cout << "e Min: " << *std::min_element(e.begin(), e.end()) << std::endl;

}

/**This method does a quick file check by checking the no.of bytes read nread params and header buffer
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

/**checks the file by opening it and reading few lines 
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
void LoadILL::runLoadInstrument(API::MatrixWorkspace_sptr workspace) {

	IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

	// Now execute the Child Algorithm. Catch and log any error, but don't stop.
	try {
		loadInst->setPropertyValue("InstrumentName", m_instrumentName);
		loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
		loadInst->execute();
	} catch (...) {
		g_log.information("Cannot load the instrument definition.");
	}
}

}

// namespace DataHandling
}// namespace Mantid
