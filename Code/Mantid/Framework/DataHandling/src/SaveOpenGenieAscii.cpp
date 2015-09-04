//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <exception>

namespace Mantid {
namespace DatHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveOpenGenieAscii)

/// Empty constructor
SaveOpenGenieAscii::SaveOpenGenieAscii() : Mantid::API::Algorithm() {}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */

void SaveOpenGenieAscii::init() {
  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "",
                                   Kernel::Direction::Input),
      "The name of the workspace containing the data you wish to save");

  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save),
      "The filename to use for the saved data");

  declareProperty(
      "Append", false,
      "If true and Filename already exists, append, else overwrite");

  declareProperty(
      "Bank", 1,
      "The bank number to include in the file header for the first spectrum, "
      "i.e., the starting bank number. "
      "This will increment for each spectrum or group member. ");

  declareProperty("ColumnHeader", true,
                  "If true, put column headers into file. ");
}

//////////////////////////////////////////////////////////////////////////

void SaveOpenGenieAscii::exec() {
  // Process properties

  // Retrieve the input workspace
  MatrixWorkspace_sptr ws = getProperty("InputWorkspace");
  int nSpectra = static_cast<int>(ws->getNumberHistograms());
  int nBins = static_cast<int>(ws->blocksize());

  bool writeHeader = getProperty("ColumnHeader");

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  // Output string variables
  std::string comment = " ";
  std::string comstr = " "; // spaces between each number
  std::string errstr = "E";
  std::string errstr2 = "";
  int InLine = 10;
  std::string GXR = "GXRealarray";
  std::string banknum = "1";
  std::string fourspc = "    ";
  std::string twospc = " ";

  if (nBins == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");

  // File
  std::ofstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  int i = 0;

  //// e axis
  // Write the column captions
  if (writeHeader) {
    file << twospc << comment << "'e'" << std::endl;
    file << fourspc << GXR << std::endl;
    file << fourspc << banknum << std::endl;
    file << fourspc << ws->getNumberHistograms() << std::endl;
  }

  // Writing Data
  bool isHistogram = ws->isHistogramData();
  Progress progress(this, 0, 1, nBins);

  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        file << fourspc;
      }
      file << (ws->readE(0)[bin]) << comment;
      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        file << std::endl << fourspc;
      }
    }
  }
  file << std::endl;

  ////// x axis
  // Write the column captions
  if (writeHeader) {
    file << twospc << comment << "'x'\n";
    file << fourspc << GXR << "\n";
    file << fourspc << banknum << std::endl;
    file << fourspc << ws->getNumberHistograms() << std::endl;
  }

  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        file << fourspc;
      }
      file << (ws->readX(0)[bin]) << comment;
      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        file << std::endl << fourspc;
      }
    }
  }
  file << std::endl;

  ////// y axis
  if (writeHeader) {
    file << comment << "'y'\n";
    file << fourspc << GXR << "\n";
    file << fourspc << banknum << std::endl;
    file << fourspc << ws->getNumberHistograms() << std::endl;
  }

  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        file << fourspc;
      }
      file << (ws->readY(0)[bin]) << comment;
      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        file << std::endl << fourspc;
      }
    }
  }
  file << std::endl;

  progress.report();
};




// up to here @shahroz

//----------------------------------------------------------------------------------------------
/** Determine the focused position for the supplied spectrum. The position
 * (l1, l2, tth) is returned via the references passed in.
 */
/*
void SaveOpenGenieAscii::getFocusedPos(MatrixWorkspace_const_sptr wksp, const
int spectrum,
                   double &l1, double &l2, double &tth, double &difc) {
  Geometry::Instrument_const_sptr instrument = wksp->getInstrument();
  if (instrument == NULL) {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if (source == NULL || sample == NULL) {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  l1 = source->getDistance(*sample);
  Geometry::IDetector_const_sptr det = wksp->getDetector(spectrum);
  if (!det) {
    std::stringstream errss;
    errss << "Workspace " << wksp->name()
          << " does not have detector with spectrum " << spectrum;
    throw std::runtime_error(errss.str());
  }
  l2 = det->getDistance(*sample);
  tth = wksp->detectorTwoTheta(det);

  difc = ((2.0 * PhysicalConstants::NeutronMass * sin(tth / 2.0) * (l1 + l2))
/
          (PhysicalConstants::h * 1e4));

  return;
};



*/

} // namespace DataHandling
} // namespace Mantid