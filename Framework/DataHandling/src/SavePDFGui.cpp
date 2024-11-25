// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SavePDFGui.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Unit.h"

#include <fstream>

namespace Mantid::DataHandling {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePDFGui)

/// Algorithm's name for identification. @see Algorithm::name
const std::string SavePDFGui::name() const { return "SavePDFGui"; }

/// Algorithm's version for identification. @see Algorithm::version
int SavePDFGui::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SavePDFGui::category() const { return "DataHandling\\Text"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SavePDFGui::summary() const { return "Save files readable by PDFGui"; }

/** Initialize the algorithm's properties.
 */
void SavePDFGui::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "An input workspace with units of Atomic Distance.");
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save, ".gr"),
                  "The filename to use for the saved data");
}

/// @copydoc Algorithm::validateInputs
std::map<std::string, std::string> SavePDFGui::validateInputs() {
  std::map<std::string, std::string> result;

  // check for null pointers - this is to protect against workspace groups
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    return result;
  }

  const auto nHist = static_cast<int>(inputWS->getNumberHistograms());
  if (nHist != 1) {
    result["InputWorkspace"] = "Workspace must contain only one spectrum";
  } else if (std::string(inputWS->getAxis(0)->unit()->label()) != "Angstrom") {
    result["InputWorkspace"] = "Expected x-units of Angstrom";
  }

  return result;
}

/** Execute the algorithm.
 */
void SavePDFGui::exec() {
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string filename = getProperty("Filename");

  // --------- open the file
  std::ofstream out;
  out.open(filename.c_str(), std::ios_base::out);

  // --------- write the header in the style of
  // #Comment: neutron, Qmin=0.5, Qmax=31.42, Qdamp=0.017659, Qbroad= 0.0191822,
  // Temperature = 300
  writeMetaData(out, inputWS);

  // --------- write the data
  writeWSData(out, inputWS);

  // --------- close the file
  out.close();
}

void SavePDFGui::writeMetaData(std::ofstream &out, const API::MatrixWorkspace_const_sptr &inputWS) {
  out << "#Comment: neutron";
  auto run = inputWS->run();
  if (run.hasProperty("Qmin")) {
    out << ", Qmin=" << run.getPropertyAsSingleValue("Qmin");
  }
  if (run.hasProperty("Qmax")) {
    out << ", Qmax=" << run.getPropertyAsSingleValue("Qmax");
  }
  if (run.hasProperty("Qdamp")) {
    out << ", Qdamp=" << run.getPropertyAsSingleValue("Qdamp");
  }
  if (run.hasProperty("Qbroad")) {
    out << ", Qbroad=" << run.getPropertyAsSingleValue("Qbroad");
  }
  // TODO add the sample temperature
  out << "\n";

  // --------- write the label for the data
  out << "##### start data\n";
  // out << "#O0 rg_int sig_rg_int low_int sig_low_int rmax rhofit\n"; // TODO
  out << "#S 1 - PDF from Mantid " << Kernel::MantidVersion::version() << "\n";
  // out << "#P0  -22.03808    1.10131 2556.26392    0.03422    1.50  0.5985\n";
  // // TODO
  out << "#L r G(r) dr dG(r)\n";
}

void SavePDFGui::writeWSData(std::ofstream &out, const API::MatrixWorkspace_const_sptr &inputWS) {
  const auto &x = inputWS->points(0);
  const auto &y = inputWS->y(0);
  const auto &dy = inputWS->e(0);
  HistogramData::HistogramDx dx(y.size(), 0.0);
  if (inputWS->sharedDx(0))
    dx = inputWS->dx(0);
  for (size_t i = 0; i < x.size(); ++i) {
    out << "  " << x[i] << "  " << y[i] << "  " << dx[i] << "  " << dy[i] << "\n";
  }
}

} // namespace Mantid::DataHandling
