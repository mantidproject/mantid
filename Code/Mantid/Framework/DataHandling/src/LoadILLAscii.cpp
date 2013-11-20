/*WIKI*

 Loads an ILL Ascii / Raw data file into a [[Workspace2D]] with the given name.
 To date this Loader is only compatible with non TOF instruments.

 Supported instruments : ILL D2B

 *WIKI*/

#include "MantidDataHandling/LoadILLAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadILLAsciiHelper.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadILLAscii)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLAscii::LoadILLAscii() :
		m_instrumentName(""),
		m_wavelength(0) {
	m_supportedInstruments.push_back("D2B");
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadILLAscii::~LoadILLAscii() {
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadILLAscii::confidence(Kernel::FileDescriptor & descriptor) const {
	const std::string & filePath = descriptor.filename();
	// Avoid some known file types that have different loaders
	int confidence(0);

	if (descriptor.isAscii()) {
		confidence = 10; // Low so that others may try
		ILLParser p(filePath);
		std::string instrumentName = p.getInstrumentName();

		g_log.information() << "Instrument name: " << instrumentName << "\n";

		if (std::find(m_supportedInstruments.begin(),
				m_supportedInstruments.end(), instrumentName)
				!= m_supportedInstruments.end())
			confidence = 80;
	}

	return confidence;
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLAscii::name() const {
	return "LoadILLAscii";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLAscii::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLAscii::category() const {
	return "DataHandling\\Text";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void LoadILLAscii::initDocs() {
	this->setWikiSummary("Loads ILL Ascii data.");
	this->setOptionalMessage("Loads ILL Raw data in Ascii format.");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLAscii::init() {
	declareProperty(new FileProperty("Filename", "", FileProperty::Load, ""),
			"Name of the data file to load.");
	declareProperty("OutputWorkspacePrefix", "",
			"Prefix for the workspaces created by the moving instrument.");

}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLAscii::exec() {
	// Init
	std::string filename = getPropertyValue("Filename");
	std::string prefix = getPropertyValue("OutputWorkspacePrefix");

	ILLParser p(filename);
	loadInstrumentName(p);
	p.parse();
	loadInstrumentDetails(p);

	const std::vector<std::vector<int> > &spectraList = p.getSpectraList();
	const std::vector<std::map<std::string, std::string> > &spectraHeaderList =
			p.getSpectraHeaderList();

	std::vector<std::vector<int> >::const_iterator iSpectra;
	std::vector<std::map<std::string, std::string> >::const_iterator iSpectraHeader;

	Progress progress(this, 0, 1, spectraList.size());
	for (iSpectra = spectraList.begin(), iSpectraHeader =
			spectraHeaderList.begin();
			iSpectra < spectraList.end()
					&& iSpectraHeader < spectraHeaderList.end();
			++iSpectra, ++iSpectraHeader) {

		g_log.debug() << "Reading sprectra: " << std::distance(spectraList.begin(),iSpectra) << std::endl;

		std::vector<int> thisSpectrum = *iSpectra;
		API::MatrixWorkspace_sptr thisWorkspace = WorkspaceFactory::Instance().create("Workspace2D", thisSpectrum.size(),
				2, 1);
		g_log.debug() << "2" << std::endl;
		thisWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(
				"Wavelength");
		g_log.debug() << "3" << std::endl;

		thisWorkspace->setYUnitLabel("Counts");
		loadIDF(thisWorkspace);
		// todo : need to mobe instrument



		thisWorkspace->dataX(0)[0] = m_wavelength - 0.001;
		thisWorkspace->dataX(0)[1] = m_wavelength + 0.001;




		size_t spec = 0;
		for (size_t i = 0; i < thisSpectrum.size(); ++i) {

			if (spec > 0) {
				// just copy the time binning axis to every spectra
				thisWorkspace->dataX(spec) = thisWorkspace->readX(0);
			}
			// Assign Y
			thisWorkspace->dataY(spec)[0] = thisSpectrum[i];
			// Assign Error
			thisWorkspace->dataE(spec)[0] = thisSpectrum[i] * thisSpectrum[i];

			++spec;
		}

		loadIDF(thisWorkspace); // assins data to the instrument

		// Set the output workspace property : concatenate prefix with spectrum number
		std::stringstream outWorkspaceNameStream;
		outWorkspaceNameStream << prefix << std::distance(spectraList.begin(),iSpectra);

		AnalysisDataService::Instance().addOrReplace(outWorkspaceNameStream.str(), thisWorkspace);

		progress.report();
	}

	p.showHeader();

}

/**
 * Load instrument details
 */
void LoadILLAscii::loadInstrumentDetails(ILLParser &p) {

	m_wavelength  = p.getValueFromHeader<double>("wavelength");
	g_log.debug() << "Wavelength: " << m_wavelength << std::endl;
}


void LoadILLAscii::loadInstrumentName(ILLParser &p) {

	m_instrumentName = p.getInstrumentName();
	if (m_instrumentName == "") {
		throw std::runtime_error(
				"Cannot read instrument name from the data file.");
	}
	g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

	m_wavelength  = p.getValueFromHeader<double>("wavelength");
	g_log.debug() << "Wavelength: " << m_wavelength << std::endl;
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadILLAscii::loadIDF(API::MatrixWorkspace_sptr &workspace) {

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
} // namespace DataHandling
} // namespace Mantid
