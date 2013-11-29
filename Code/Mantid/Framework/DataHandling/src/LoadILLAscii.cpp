/*WIKI*

 Loads an ILL Ascii / Raw data file into a [[Workspace2D]] with the given name.
 To date this Loader is only compatible with non TOF instruments.

 Supported instruments : ILL D2B

 *WIKI*/

#include "MantidDataHandling/LoadILLAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadILLAsciiHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/System.h"

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <iterator>     // std::distance

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadILLAscii)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLAscii::LoadILLAscii() :
		m_instrumentName(""), m_wavelength(0) {
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
	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"Name to use for the output workspace");

}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLAscii::exec() {
	// Init
	std::string filename = getPropertyValue("Filename");
	ILLParser p(filename);
	loadInstrumentName(p);
	p.parse();
	loadInstrumentDetails(p);

	// get local references to the parsed file
	const std::vector<std::vector<int> > &spectraList = p.getSpectraList();
	const std::vector<std::map<std::string, std::string> > &spectraHeaderList =
			p.getSpectraHeaderList();

	std::vector<API::MatrixWorkspace_sptr> workspaceList;
	workspaceList.reserve(spectraList.size());

	// iterate parsed file
	std::vector<std::vector<int> >::const_iterator iSpectra;
	std::vector<std::map<std::string, std::string> >::const_iterator iSpectraHeader;

	Progress progress(this, 0, 1, spectraList.size());
	for (iSpectra = spectraList.begin(), iSpectraHeader =
			spectraHeaderList.begin();
			iSpectra < spectraList.end()
					&& iSpectraHeader < spectraHeaderList.end();
			++iSpectra, ++iSpectraHeader) {

		g_log.debug() << "Reading Spectrum: "
				<< std::distance(spectraList.begin(), iSpectra) << std::endl;

		std::vector<int> thisSpectrum = *iSpectra;
		API::MatrixWorkspace_sptr thisWorkspace =
				WorkspaceFactory::Instance().create("Workspace2D",
						thisSpectrum.size(), 2, 1);

		thisWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(
				"Wavelength");
		thisWorkspace->setYUnitLabel("Counts");
		loadIDF(thisWorkspace);

		double currentPositionAngle = p.getValue<double>("angles*1000",
				*iSpectraHeader) / 1000;
		moveDetector(thisWorkspace, currentPositionAngle);

		//
		loadsDataIntoTheWS(thisWorkspace, thisSpectrum);
		loadIDF(thisWorkspace); // assigns data to the instrument

		workspaceList.push_back(thisWorkspace);

		// JUUST TO SEE the WS in mantiplot
		std::stringstream outWorkspaceNameStream;
		outWorkspaceNameStream << "test"
				<< std::distance(spectraList.begin(), iSpectra);
		AnalysisDataService::Instance().addOrReplace(
				outWorkspaceNameStream.str(), thisWorkspace);

		progress.report();
	}

	//p.showHeader();

	// TODO: Merge workspaces
	MatrixWorkspace_sptr outWorkspace = mergeWorkspaces(workspaceList);

	// TODO : Correct (this is like this to work!)
	setProperty("OutputWorkspace",outWorkspace);
			//WorkspaceFactory::Instance().create("Workspace2D", 128, 2, 1));

}

/**
 * Load instrument details
 */
void LoadILLAscii::loadInstrumentDetails(ILLParser &p) {

	m_wavelength = p.getValueFromHeader<double>("wavelength");
	g_log.debug() << "Wavelength: " << m_wavelength << std::endl;
}

void LoadILLAscii::loadInstrumentName(ILLParser &p) {

	m_instrumentName = p.getInstrumentName();
	if (m_instrumentName == "") {
		throw std::runtime_error(
				"Cannot read instrument name from the data file.");
	}
	g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

	m_wavelength = p.getValueFromHeader<double>("wavelength");
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

/**
 * Loads the scan into the workspace
 */
void LoadILLAscii::loadsDataIntoTheWS(API::MatrixWorkspace_sptr &thisWorkspace,
		const std::vector<int> &thisSpectrum) {

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

	loadIDF(thisWorkspace); // assigns data to the instrument

}

/**
 *
 * @param angle :: the theta angle read from the data file
 */
void LoadILLAscii::moveDetector(API::MatrixWorkspace_sptr &ws, double angle) {

	// todo: put this as a constant somewhere?
	const std::string componentName("bank_uniq");

	try {
		// current position
		Geometry::Instrument_const_sptr instrument = ws->getInstrument();

		Geometry::IComponent_const_sptr component =
				instrument->getComponentByName(componentName);
		Geometry::ICompAssembly_const_sptr componentAssembly =
				boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
						component);

		if (componentAssembly) {
			// Get a vector of children (recursively)
			std::vector<Geometry::IComponent_const_sptr> children;
			componentAssembly->getChildren(children, false);

			for (unsigned int i = 0; i < children.size(); ++i) {
				std::string tubeName = children.at(i)->getName();

				Geometry::IComponent_const_sptr tube =
						instrument->getComponentByName(tubeName);

				// position - set all detector distance to constant l2
				double r, theta, phi, refTheta, newTheta;
				V3D oldPos = tube->getPos();
				oldPos.getSpherical(r, theta, phi);

				if (i == 0) {
					// the theta for the first tube is the reference
					refTheta = theta;
				}

				newTheta = theta - refTheta + angle;

				V3D newPos;
				newPos.spherical(r, newTheta, phi);

				//g_log.debug() << tube->getName() << " : t = " << theta << " ==> t = " << newTheta << "\n";
				Geometry::ParameterMap& pmap = ws->instrumentParameters();
				Geometry::ComponentHelper::moveComponent(*tube, pmap, newPos,
						Geometry::ComponentHelper::Absolute);

			}
		}
	} catch (Mantid::Kernel::Exception::NotFoundError&) {
		throw std::runtime_error(
				"Error when trying to move the detector : NotFoundError");
	} catch (std::runtime_error &) {
		throw std::runtime_error(
				"Error when trying to move the detector : runtime_error");
	}

}

/**
 * Merge all workspaces and create a virtual new instrument.
 * @return new workspace with all data and the new virtual instrument
 */
MatrixWorkspace_sptr LoadILLAscii::mergeWorkspaces(
		std::vector<API::MatrixWorkspace_sptr> &workspaceList) {



	if (workspaceList.size() > 0) {



		// 1st workspace will be the reference
		API::MatrixWorkspace_sptr &refWorkspace = workspaceList[0];
		size_t numberOfHistograms = refWorkspace->getNumberHistograms() * workspaceList.size();
		size_t xWidth = refWorkspace->blocksize()+1;
		size_t yWidth = refWorkspace->blocksize();
		MatrixWorkspace_sptr outWorkspace = WorkspaceFactory::Instance().create(
				"Workspace2D", numberOfHistograms, xWidth, yWidth);
		outWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
		outWorkspace->setYUnitLabel("Counts");

		// This is not corect : i copies also teh detectors!
		// copy base instrument
		outWorkspace->setInstrument(refWorkspace->getInstrument()->baseInstrument()); // if baseInstrument the ParameterMap is empty!

		// let's modify the instrument!
		Geometry::Instrument_const_sptr outInstrument = outWorkspace->getInstrument();

		std::size_t numberOfDetectorsPerScan = outInstrument->getNumberDetectors(true);
		g_log.debug() << "numberOfDetectorsPerScan: " << numberOfDetectorsPerScan << std::endl;





		auto it = workspaceList.begin();
		for (++it; it < workspaceList.end(); ++it) { // just the first
			std::size_t pos = std::distance(workspaceList.begin(),it);
			API::MatrixWorkspace_sptr thisWorkspace = *it;

			g_log.debug() << "Merging the workspace: " << pos << std::endl;

			// current position
			Geometry::Instrument_const_sptr thisInstrument = thisWorkspace->getInstrument();
			boost::shared_ptr<Geometry::ParameterMap> outParameterMap = thisInstrument->getParameterMap();

			g_log.debug() << outParameterMap->asString() << std::endl;

			//outInstrument->printTree(g_log.debug());

			//g_log.debug() << "outParameterMap Map size: " << outParameterMap->size() << std::endl;


			long newIdOffset = numberOfDetectorsPerScan*pos; // this must me summed to the current IDs


		}
		return outWorkspace;
	}
	else{
		throw std::runtime_error("Error: No workspaces were found to be merged!");
	}

}

} // namespace DataHandling
} // namespace Mantid
