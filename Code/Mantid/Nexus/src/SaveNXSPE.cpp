#include "MantidNexus/SaveNXSPE.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/WorkspaceValidators.h"


#include "Poco/File.h"
#include "Poco/Path.h"

namespace Mantid {
namespace NeXus {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM( SaveNXSPE)

using namespace Kernel;
using namespace API;

SaveNXSPE::SaveNXSPE() :
	API::Algorithm() {
}

/**
 * Initialise the algorithm
 */
void SaveNXSPE::init() {

	std::vector<std::string> exts;
	exts.push_back(".nxspe");

	declareProperty(new API::FileProperty("Filename", "", FileProperty::Save, exts),
			"The name of the NXSPE file to write, as a full or relative path");

	CompositeValidator<> * wsValidator = new CompositeValidator<> ;
	wsValidator->add(new API::WorkspaceUnitValidator<>("DeltaE"));
	wsValidator->add(new API::CommonBinsValidator<>);
	wsValidator->add(new API::HistogramValidator<>);

	declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace",
			"", Direction::Input, wsValidator),
			"Name of the workspace to be saved.");
}

/**
 * Execute the algorithm
 */
void SaveNXSPE::exec() {
	using namespace Mantid::API;
	// Retrieve the input workspace
	const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

	// Do the full check for common binning
	if (!WorkspaceHelpers::commonBoundaries(inputWS)) {
		g_log.error("The input workspace must have common bins");
		throw std::invalid_argument("The input workspace must have common bins");
	}

	const int nHist = inputWS->getNumberHistograms();
	this->nBins = inputWS->blocksize();

	// Retrieve the filename from the properties
	this->filename = getPropertyValue("Filename");

	// Create the file.
	::NeXus::File nxFile(this->filename, NXACC_CREATE5);

	// Make the top level entry (and open it)
	nxFile.makeGroup(inputWS->getName(), "NXentry", true);

	// Create NXSPE_info
	nxFile.makeGroup("NXSPE_info", "NXcollection", true);

	//TODO: Get and write Ei as "fixed_energy"
	//TODO: Get and write psi as "psi"
	//TODO: Get and write ki/kf as "ki_over_kf_scaling" = 1/0

	nxFile.closeGroup(); // NXSPE_info

	// NXinstrument
	nxFile.makeGroup("instrument", "NXinstrument", true);

	// NXfermi_chopper
	nxFile.makeGroup("fermi", "NXfermi_chopper", true);
	//TODO: Get and write Ei as "fixed_energy"
	nxFile.closeGroup(); // NXfermi_chopper

	nxFile.closeGroup(); // NXinstrument

	// NXsample
	nxFile.makeGroup("sample", "NXsample", true);
	// TODO: Write sample info
	nxFile.closeGroup(); // NXsample

	// Energy bins
    // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
    const MantidVec& X = inputWS->readX(0);
	nxFile.writeData("energy", X);

	// TODO: Data Array
	// TODO: Error Array
	// Polar Angles
	// Polar Angle Width
	// Azimuthal Angle
	// Azimuthal Angle Width

	nxFile.closeGroup(); // Top level NXentry
}

} // namespace NeXus
} // namespace Mantid
