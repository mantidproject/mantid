#include "MantidDataHandling/SavePDFGui.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/MantidVersion.h"
#include <fstream>
#include <iomanip>

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePDFGui)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SavePDFGui::SavePDFGui() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SavePDFGui::~SavePDFGui() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string SavePDFGui::name() const { return "SavePDFGui"; }

/// Algorithm's version for identification. @see Algorithm::version
int SavePDFGui::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SavePDFGui::category() const { return "DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SavePDFGui::summary() const {
  return "Save files readable by PDFGui";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SavePDFGui::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save, ".gr"),
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

  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  if (nHist != 1) {
    result["InputWorkspace"] = "Workspace must contain only one spectrum";
  } else if (std::string(inputWS->getAxis(0)->unit()->label()) != "Angstrom") {
    result["InputWorkspace"] = "Expected x-units of Angstrom";
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SavePDFGui::exec() {
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string filename = getProperty("Filename");

  // --------- open the file
  std::ofstream out;
  out.open(filename.c_str(), std::ios_base::out);

  // --------- write the header in the style of
  //#Comment: neutron, Qmin=0.5, Qmax=31.42, Qdamp=0.017659, Qbroad= 0.0191822, Temperature = 300
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

  // --------- write the data
  auto x = inputWS->readX(0);
  auto dx = inputWS->readDx(0);
  auto y = inputWS->readY(0);
  auto dy = inputWS->readE(0);
  const size_t length = x.size();
  for (size_t i = 0; i < length; ++i) {
    out << "  " << x[i] << "  " << y[i] << "  " << dx[i] << "  " << dy[i]
        << "\n";
  }

  // --------- close the file
  out.close();
}

} // namespace DataHandling
} // namespace Mantid
