// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDaveGrp.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <fstream>

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDaveGrp)

using namespace Mantid::Kernel;
using namespace Mantid::API;

/** Initialize the algorithm's properties.
 */
void SaveDaveGrp::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                        "An input workspace.");
  this->declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".grp"),
                        "A DAVE grouped data format file that will be created");
  this->declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("ToMicroEV", false, Kernel::Direction::Input),
                        "Transform all energy units from milli eV to micro eV");
}

/** Execute the algorithm.
 */
void SaveDaveGrp::exec() {
  // Get the workspace
  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  std::size_t nSpectra = ws->getNumberHistograms();
  std::size_t nBins = ws->blocksize();
  if (nSpectra * nBins == 0)
    throw std::invalid_argument("Either the number of bins or the number of histograms is 0");
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

  file << "# Number of " << xcaption << " values\n";
  file << nBins << '\n';
  file << "# Number of " << ycaption << " values\n";
  file << nSpectra << '\n';

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
  file << "# " << xcaption << " (" << xunit << ") values\n";
  auto x = ws->points(0);
  for (std::size_t i = 0; i < nBins; i++) {
    double xvalue = x[i];
    if (xToMicroeV)
      xvalue *= 1000.;
    file << xvalue << '\n';
  }

  if (yToMicroeV)
    yunit = "micro eV";
  file << "# " << ycaption << " (" << yunit << ") values\n";
  double yvalue;
  if ((*ws->getAxis(1)).length() == (nSpectra + 1)) {
    for (std::size_t i = 0; i < nSpectra; i++) {
      yvalue = 0.5 * (((*ws->getAxis(1))(i)) + ((*ws->getAxis(1))(i + 1)));
      if (yToMicroeV)
        yvalue *= 1000.;
      file << yvalue << '\n';
    }
  } else {
    for (std::size_t i = 0; i < nSpectra; i++) {
      yvalue = (*ws->getAxis(1))(i);
      if (yToMicroeV)
        yvalue *= 1000.;
      file << yvalue << '\n';
    }
  }
  Progress progress(this, 0.0, 1.0, nSpectra);
  for (std::size_t i = 0; i < nSpectra; i++) {
    file << "# Group " << i << '\n';
    auto &Y = ws->y(i);
    auto &E = ws->e(i);
    auto itE = E.cbegin();
    std::for_each(Y.cbegin(), Y.cend(), [&itE, &file](const double y) { file << y << " " << *itE++ << "\n"; });

    progress.report();
  }
  file.close();
}

} // namespace Mantid::DataHandling
