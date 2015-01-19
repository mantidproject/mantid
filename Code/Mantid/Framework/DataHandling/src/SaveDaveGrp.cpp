#include "MantidDataHandling/SaveDaveGrp.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"
#include <fstream>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDaveGrp)

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveDaveGrp::SaveDaveGrp() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveDaveGrp::~SaveDaveGrp() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDaveGrp::init() {
  this->declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  std::vector<std::string> exts;
  exts.push_back(".grp");
  this->declareProperty(
      new FileProperty("Filename", "", FileProperty::Save, exts),
      "A DAVE grouped data format file that will be created");
  this->declareProperty(new Kernel::PropertyWithValue<bool>(
                            "ToMicroEV", false, Kernel::Direction::Input),
                        "Transform all energy units from milli eV to micro eV");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveDaveGrp::exec() {
  // Get the workspace
  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  std::size_t nSpectra = ws->getNumberHistograms();
  std::size_t nBins = ws->blocksize();
  if (nSpectra * nBins == 0)
    throw std::invalid_argument(
        "Either the number of bins or the number of histograms is 0");
  bool isHist = ws->isHistogramData();
  std::string xcaption = ws->getAxis(0)->unit()->caption();
  std::string ycaption = ws->getAxis(1)->unit()->caption();
  if (xcaption.length() == 0)
    xcaption = "X";
  if (ycaption.length() == 0 || ycaption == "Spectrum")
    ycaption = "Y";

  std::string filename = getProperty("Filename");
  std::ofstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }

  file << "# Number of " << xcaption << " values" << std::endl;
  file << nBins << std::endl;
  file << "# Number of " << ycaption << " values" << std::endl;
  file << nSpectra << std::endl;

  bool toMicroeV = getProperty("ToMicroEV");
  bool xToMicroeV = false, yToMicroeV = false;

  std::string xunit = ws->getAxis(0)->unit()->label();
  std::string yunit = ws->getAxis(1)->unit()->label();
  if (yunit == "Angstrom^-1")
    yunit = "1/Angstroms"; // backwards compatability with old versions
  if (toMicroeV && (xunit == "meV"))
    xToMicroeV = true;
  if (toMicroeV && (yunit == "meV"))
    yToMicroeV = true;

  if (xToMicroeV)
    xunit = "micro eV";
  file << "# " << xcaption << " (" << xunit << ") values" << std::endl;
  std::vector<double> x = ws->readX(0);
  for (std::size_t i = 0; i < nBins; i++) {
    double xvalue = (isHist) ? (x[i] + x[i + 1]) * 0.5 : x[i];
    if (xToMicroeV)
      xvalue *= 1000.;
    file << xvalue << std::endl;
  }

  if (yToMicroeV)
    yunit = "micro eV";
  file << "# " << ycaption << " (" << yunit << ") values" << std::endl;
  double yvalue;
  if ((*ws->getAxis(1)).length() == (nSpectra + 1)) {
    for (std::size_t i = 0; i < nSpectra; i++) {
      yvalue = 0.5 * (((*ws->getAxis(1))(i)) + ((*ws->getAxis(1))(i + 1)));
      if (yToMicroeV)
        yvalue *= 1000.;
      file << yvalue << std::endl;
    }
  } else {
    for (std::size_t i = 0; i < nSpectra; i++) {
      yvalue = (*ws->getAxis(1))(i);
      if (yToMicroeV)
        yvalue *= 1000.;
      file << yvalue << std::endl;
    }
  }
  Progress progress(this, 0, 1, nSpectra);
  for (std::size_t i = 0; i < nSpectra; i++) {
    file << "# Group " << i << std::endl;
    std::vector<double> y = ws->readY(i), er = ws->readE(i);
    std::vector<double>::iterator ity, iter;
    for (ity = y.begin(), iter = er.begin(); ity != y.end(); ++ity, ++iter)
      file << (*ity) << " " << (*iter) << std::endl;
    progress.report();
  }
  file.close();
}

} // namespace Mantid
} // namespace DataHandling
