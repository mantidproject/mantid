//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpec.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include <fstream>
#include <cstring>
#include <boost/tokenizer.hpp>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSpec)

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadSpec::LoadSpec() {}

/// Initialisation method.
void LoadSpec::init() {
  std::vector<std::string> exts;
  exts.push_back(".dat");
  exts.push_back(".txt");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be .txt or .dat.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the workspace that will be created, filled with the read-in "
      "data and stored in the [[Analysis Data Service]].");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();
  units.insert(units.begin(), "MomemtumTransfer");
  declareProperty("Unit", "Energy",
                  boost::make_shared<Kernel::StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\")");
}

/**
*   Executes the algorithm.
*/
void LoadSpec::exec() {
  std::string filename = getProperty("Filename");
  // std::string separator = " "; //separator can be 1 or more spaces
  std::ifstream file(filename.c_str());

  file.seekg(0, std::ios::end);
  Progress progress(this, 0, 1, static_cast<int>(file.tellg()));
  file.seekg(0, std::ios::beg);

  std::string str;

  std::vector<DataObjects::Histogram1D> spectra;

  int nBins = 0; // number of rows

  // bool numeric = true;
  std::vector<double> input;

  // determine the number of lines starting by #L
  // as there is one per set of data
  int spectra_nbr = 0;
  while (getline(file, str)) {
    if (str[0] == '#' && str[1] == 'L') {
      spectra_nbr++;
    }
  }

  spectra.resize(spectra_nbr);
  file.clear(); // end of file has been reached so we need to clear file state
  file.seekg(0, std::ios::beg); // go back to beginning of file

  int working_with_spectrum_nbr = -1; // spectrum number
  while (getline(file, str)) {
    progress.report(static_cast<int>(file.tellg()));

    // line with data, need to be parsed by white spaces
    if (!str.empty() && str[0] != '#') {
      typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
      boost::char_separator<char> sep(" ");
      tokenizer tok(str, sep);
      for (tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
        std::stringstream ss;
        ss << *beg;
        double d;
        ss >> d;
        input.push_back(d);
      }
    }

    if (str.empty()) {
      if (working_with_spectrum_nbr != -1) {
        for (int j = 0; j < static_cast<int>(input.size() - 1); j++) {
          spectra[working_with_spectrum_nbr].dataX().push_back(input[j]);
          j++;
          spectra[working_with_spectrum_nbr].dataY().push_back(input[j]);
          j++;
          spectra[working_with_spectrum_nbr].dataE().push_back(input[j]);
          nBins = j / 3;
        }
      }
      working_with_spectrum_nbr++;
      input.clear();
    }

  } // end of read file

  try {
    if (spectra_nbr == 0)
      throw "Undefined number of spectra";

    if (working_with_spectrum_nbr == (spectra_nbr - 1)) {
      for (int j = 0; j < static_cast<int>(input.size() - 1); j++) {
        spectra[working_with_spectrum_nbr].dataX().push_back(input[j]);
        j++;
        spectra[working_with_spectrum_nbr].dataY().push_back(input[j]);
        j++;
        spectra[working_with_spectrum_nbr].dataE().push_back(input[j]);
        nBins = j / 3;
      }
    }
  } catch (...) {
  }

  try {
    int nSpectra = spectra_nbr;
    MatrixWorkspace_sptr localWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            WorkspaceFactory::Instance().create("Workspace2D", nSpectra, nBins,
                                                nBins));

    localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create(getProperty("Unit"));

    for (int i = 0; i < nSpectra; i++) {
      localWorkspace->dataX(i) = spectra[i].dataX();
      localWorkspace->dataY(i) = spectra[i].dataY();
      localWorkspace->dataE(i) = spectra[i].dataE();
      // Just have spectrum number start at 1 and count up
      localWorkspace->getSpectrum(i)->setSpectrumNo(i + 1);
    }

    setProperty("OutputWorkspace", localWorkspace);
  } catch (Exception::NotFoundError &) {
    // Asked for dimensionless workspace (obviously not in unit factory)
  }
}

} // namespace DataHandling
} // namespace Mantid
