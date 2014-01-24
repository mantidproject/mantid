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
#include <sstream>

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
	// Add here supported instruments by this loader
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

	std::string filename = getPropertyValue("Filename");

	// Parses ascii file and fills the data scructures
	ILLParser illAsciiParser(filename);
	loadInstrumentName(illAsciiParser);
	illAsciiParser.parse();
	loadExperimentDetails(illAsciiParser);

	// get local references to the parsed file
	const std::vector<std::vector<int> > &spectraList = illAsciiParser.getSpectraList();
	const std::vector<std::map<std::string, std::string> > &spectraHeaderList = illAsciiParser.getSpectraHeaderList();

	// list containing all parsed scans. 1 scan => 1 ws
	std::vector<API::MatrixWorkspace_sptr> workspaceList;
	workspaceList.reserve(spectraList.size());

	// iterate parsed file
	std::vector<std::vector<int> >::const_iterator iSpectra;
	std::vector<std::map<std::string, std::string> >::const_iterator iSpectraHeader;

	Progress progress(this, 0, 1, spectraList.size());
	for (iSpectra = spectraList.begin(), iSpectraHeader = spectraHeaderList.begin();
			iSpectra < spectraList.end() && iSpectraHeader < spectraHeaderList.end();
			++iSpectra, ++iSpectraHeader) {

		g_log.debug() << "Reading Spectrum: " << std::distance(spectraList.begin(), iSpectra) << std::endl;

		std::vector<int> thisSpectrum = *iSpectra;
		API::MatrixWorkspace_sptr thisWorkspace = WorkspaceFactory::Instance().create("Workspace2D",
				thisSpectrum.size(), 2, 1);

		thisWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
		thisWorkspace->setYUnitLabel("Counts");
		// only reads instrument
		loadIDF(thisWorkspace);

		double currentPositionAngle = illAsciiParser.getValue<double>("angles*1000", *iSpectraHeader) / 1000;
		moveDetector(thisWorkspace, currentPositionAngle);

		//
		loadsDataIntoTheWS(thisWorkspace, thisSpectrum);
		loadIDF(thisWorkspace); // assigns data to the instrument

		workspaceList.push_back(thisWorkspace);

		// just to see the list of WS in MantidPlot
		// TODO: delete this at the end!
		std::stringstream outWorkspaceNameStream;
		outWorkspaceNameStream << "test" << std::distance(spectraList.begin(), iSpectra);
		AnalysisDataService::Instance().addOrReplace(outWorkspaceNameStream.str(), thisWorkspace);
		// End here

		progress.report();
	}

	//p.showHeader();

	// Merge the workspace list into a single WS with a virtual instrument
	// TODO : Not done yet!
	MatrixWorkspace_sptr outWorkspace = mergeWorkspaces(workspaceList);
	setProperty("OutputWorkspace",outWorkspace);


}

/**
 * Load instrument details
 */
void LoadILLAscii::loadExperimentDetails(ILLParser &p) {

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

		Geometry::IComponent_const_sptr component = instrument->getComponentByName(componentName);
		Geometry::ICompAssembly_const_sptr componentAssembly =
				boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);

		if (componentAssembly) {
			// Get a vector of children (recursively)
			std::vector<Geometry::IComponent_const_sptr> children;
			componentAssembly->getChildren(children, false);

			for (unsigned int i = 0; i < children.size(); ++i) {
				std::string tubeName = children.at(i)->getName();

				Geometry::IComponent_const_sptr tube = instrument->getComponentByName(tubeName);

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
				Geometry::ComponentHelper::moveComponent(*tube, pmap, newPos, Geometry::ComponentHelper::Absolute);

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

/*
 * Changes the assembly
 * Copies componentName nTimes
 *
 */
//void duplicateCompAssembly(Geometry::CompAssembly *assembly,const std::string &componentName, int ntimes) {
//
//	for (int i = 0; i < assembly->nelements(); i++) {
//
//			boost::shared_ptr<Geometry::IComponent> it = (*assembly)[i];
//			Geometry::CompAssembly* children = dynamic_cast<Geometry::CompAssembly*>(it.get());
//			if (children) {
//				if (children->getName() == componentName) {
//					std::cout << "right component " << std::endl;
//					for (int i =0; i < ntimes -1; ++i){
//						std::ostringstream newComponentName;
//						newComponentName << componentName << "_"<< i;
//						children->addCopy(children,  newComponentName.str() );
//					}
//					return;
//				}
//				else {
//					duplicateCompAssembly(children,componentName,ntimes);
//				}
//			}
//		}
//
//}

//void printInstrument(Geometry::CompAssembly *assembly) {
//	for (int i = 0; i < assembly->nelements(); i++) {
//		std::cout << "**** Element " << i << " of " << assembly->nelements() << std::endl;
//
//		boost::shared_ptr<Geometry::IComponent> it = (*assembly)[i];
//
//
//		//it->printSelf(std::cout);
//
//		Geometry::CompAssembly* children = dynamic_cast<Geometry::CompAssembly*>(it.get());
//
//		std::cout << "Element number = " << i << " : ";
//		if (children) {
//			std::cout <<  " with name: " << children->getName() << std::endl;
//			std::cout << "Children :******** " << std::endl;
//			printInstrument(children);
//		} else {
//			std::cout << " with name: " << it->getName() << std::endl;
//		}
//	}
//}


//void printWorkspace(const API::MatrixWorkspace_sptr &workspace) {
//	Geometry::Instrument_const_sptr instrument = workspace->getInstrument();
//
//	//duplicateCompAssembly(const_cast < Geometry::Instrument* >( instrument.get()), "bank_uniq", 3 );
//	printInstrument( const_cast < Geometry::Instrument* > ( instrument.get() ) );
//
//}

/* Duplicates the componentName in the given assembly
 *
 * Both components must have the same structure!
 *
 *
 */
void LoadILLAscii::addCompAssemblyToReferenceInstrument(Geometry::CompAssembly *refAssembly,
		Geometry::CompAssembly *fromAssembly, const std::string &componentName) {

	assert(refAssembly->nelements() == fromAssembly->nelements());

	g_log.debug() << "refAssembly: " << refAssembly->getName()  <<
					" :: fromAssembly: " << fromAssembly->getName() << std::endl;

	for (int i = 0; i < refAssembly->nelements(); i++) {

		g_log.debug() << "refAssembly_" << i  << " : " << (*refAssembly)[i]->getName()  <<
				" :: fromAssembly_" << i  << " : "  << (*refAssembly)[i]->getName() << std::endl;


		boost::shared_ptr<Geometry::IComponent> itRefAssembly = (*refAssembly)[i];
		boost::shared_ptr<Geometry::IComponent> itFromAssembly = (*fromAssembly)[i];

		Geometry::CompAssembly* childrenRefInst = dynamic_cast<Geometry::CompAssembly*>(itRefAssembly.get());
		Geometry::CompAssembly* childrenFromInst = dynamic_cast<Geometry::CompAssembly*>(itFromAssembly.get());

		if (childrenFromInst && childrenRefInst) {
			g_log.debug() << "\tchildrenRefInst: " << childrenRefInst->getName()  <<
					" :: childrenFromInst: " << childrenFromInst->getName() << std::endl;

			if (childrenFromInst->getName() == componentName) {
				g_log.debug() << "Add Copy: " << componentName << std::endl;

				// TODO:

				// Copy from childrenFromInst to ref Inst
				// code below same as:
				int childrenSize = childrenRefInst->addCopy(childrenFromInst);
//				Geometry::IComponent* newcomp = childrenFromInst->clone();
//				newcomp->setParent(childrenRefInst);
//				childrenRefInst->add(newcomp);

				g_log.debug() << "Before = " << refAssembly->nelements() << " afer = " << childrenSize << std::endl;
				//

				return;
			} else {
				addCompAssemblyToReferenceInstrument(childrenRefInst,
						childrenFromInst, componentName);
			}
		}
	}
}




/**
 * Merge all workspaces and create a virtual new instrument.
 * @return new workspace with all the scans and a new virtual instrument
 *
 * TODO:
 * See : CompAssembly::printTree
 *
 * 1. duplicate object assembly?
 *    E.g. bank_uniq
 *    Problem is CompAssembly::addCopy() does not accept parametrized CompAssembly.
 *
 *
 *
 * Options:
 *
 * 1. Make detectorId writable
 *    - copy thisWorkspace->getDetector(i) and then set the detector
 *
 *
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

//		// Fully copy the instrument
//		auto sourceBaseInstrument = boost::shared_ptr<Geometry::Instrument>(refWorkspace->getInstrument()->baseInstrument()->clone());
//		outWorkspace->setInstrument( sourceBaseInstrument );
//		// OR copy base instrument => ParameterMap is empty!
//		// outWorkspace->setInstrument(refWorkspace->getInstrument()->baseInstrument());
//
//		// outInstrument: The instrument that will be modified
//		Geometry::Instrument_const_sptr outInstrument = outWorkspace->getInstrument();
//
//		std::size_t numberOfDetectorsPerScan = outInstrument->getNumberDetectors(true);
//		g_log.debug() << "***** NumberOfDetectorsPerScan: " << numberOfDetectorsPerScan << std::endl;
//
//		//DEBUG: print outParameterMap
//		boost::shared_ptr<Geometry::ParameterMap> outParameterMap = outInstrument->getParameterMap();
//		// g_log.debug() << outParameterMap->asString() << std::endl;
//		//DEBUG: print intrument tree
//		//outInstrument->printTree(g_log.debug());
//
//
//		auto it = workspaceList.begin();
//		for (++it; it < workspaceList.end(); ++it) { // jumps the first
//			std::size_t pos = std::distance(workspaceList.begin(),it);
//			API::MatrixWorkspace_sptr thisWorkspace = *it;
//
//			g_log.debug() << "Merging the workspace: " << pos << std::endl;
//
//			// current position
//			Geometry::Instrument_const_sptr thisInstrument = thisWorkspace->getInstrument();
//
//
//			long newIdOffset = numberOfDetectorsPerScan*pos; // this must me summed to the current IDs
//
//			// FUN STARTS HERE!
//			//addCompAssemblyToReferenceInstrument( const_cast < Geometry::Instrument* >( thisInstrument.get()),"bank_uniq");
//			addCompAssemblyToReferenceInstrument(const_cast < Geometry::Instrument* >(outInstrument.get()),
//					const_cast < Geometry::Instrument* >(thisInstrument.get()),"bank_uniq");
//
//
//
//
//
//			// Debug:
//			/*
//			outInstrument->printTree(g_log.debug());
//			boost::shared_ptr<Geometry::ParameterMap> thisParameterMap = thisInstrument->getParameterMap();
//			g_log.debug() << outParameterMap->asString() << std::endl;
//			*/
//
//		}
		return outWorkspace;
	}
	else{
		throw std::runtime_error("Error: No workspaces were found to be merged!");
	}

}

} // namespace DataHandling
} // namespace Mantid
