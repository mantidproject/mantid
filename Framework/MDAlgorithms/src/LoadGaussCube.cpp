// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/LoadGaussCube.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidMDAlgorithms/CreateMDHistoWorkspace.h"

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel::Strings;

namespace Mantid {
namespace MDAlgorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadGaussCube)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadGaussCube::name() const { return "LoadGaussCube"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadGaussCube::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadGaussCube::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadGaussCube::summary() const {
  return "Algorithm to load gauss cube files and output an 3D MDHistoWorkspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadGaussCube::init() {
  auto algcreateMD = AlgorithmManager::Instance().createUnmanaged("CreateMDHistoWorkspace");
  algcreateMD->initialize();

  const std::vector<std::string> exts{".cube"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "Path to gauss cube file (with extension .cube). Note algorithm assumes XYZ ordering.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "3D MDHistoWorkspace containing the data in the .cube file.");

  copyProperty(algcreateMD, "Names");
  copyProperty(algcreateMD, "Frames");
  copyProperty(algcreateMD, "Units");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadGaussCube::exec() {
  std::string Filename = getProperty("Filename");

  // open file
  std::ifstream in(Filename.c_str());
  // skip header
  for (int iline = 0; iline < 2; ++iline) {
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  // read lower extent
  std::vector<double> extents(6, 0.0);
  std::string str = getWord(in, false); // init string and ignore the first element
  // get lower limit
  for (int idim = 0; idim < 3; ++idim) {
    str = getWord(in, false);
    if (str.length() < 1) {
      throw std::logic_error(std::string(
          "Third line must contain 4 elements (first isignored) and subsequent 3 are the lower extents of workspace."));
    }
    extents[2 * idim] = std::stod(str);
  }
  readToEndOfLine(in, true);

  // read nbins and upper extent
  std::vector<int> nbins(3, 0);
  for (int idim = 0; idim < 3; ++idim) {
    nbins[idim] = std::stoi(getWord(in, false)); // first element in row
    for (int iskip = 0; iskip < idim; ++iskip) {
      str = getWord(in, true); // ignore
    }
    extents[2 * idim + 1] = extents[2 * idim] + std::stod(getWord(in, true));
    readToEndOfLine(in, true);
  }
  readToEndOfLine(in, true);

  // read signal array
  auto nelem = nbins[0] * nbins[1] * nbins[2];
  std::vector<double> signal(nelem, 0.0);
  std::vector<double> error(nelem, 0.0);

  for (size_t ielem = 0; ielem < nelem && in.good(); ++ielem) {
    signal[ielem] = std::stod(getWord(in, true));
  }

  // make output workspace
  CreateMDHistoWorkspace alg;
  alg.initialize();
  alg.setProperty("SignalInput", signal);
  alg.setProperty("ErrorInput", error);
  alg.setProperty("Dimensionality", 3);
  alg.setProperty("NumberOfBins", nbins);
  alg.setProperty("Extents", extents);
  alg.setPropertyValue("Names", getPropertyValue("Names"));
  alg.setPropertyValue("Frames", getPropertyValue("Frames"));
  alg.setPropertyValue("Units", getPropertyValue("Units"));
  alg.setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
  alg.execute();

  setPropertyValue("OutputWorkspace", alg.getPropertyValue("OutputWorkspace"));
}

} // namespace MDAlgorithms
} // namespace Mantid
