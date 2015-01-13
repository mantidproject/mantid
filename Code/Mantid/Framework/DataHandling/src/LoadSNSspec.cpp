//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSspec.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include <fstream>
#include <cstring>
#include <boost/tokenizer.hpp>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadSNSspec);

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSNSspec::confidence(Kernel::FileDescriptor &descriptor) const {
  if (!descriptor.isAscii())
    return 0;

  auto &file = descriptor.data();

  int confidence(0), axiscols(0), datacols(0);
  std::string str;
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  boost::char_separator<char> sep(" ");
  bool snsspec(false);

  while (std::getline(file, str)) {
    // File is opened in binary mode so getline will leave a \r at the end of an
    // empty line if it exists
    if (str.empty() || str == "\r")
      continue;

    try {
      // if it's comment line
      if (str.at(0) == '#') {
        if (str.at(1) == 'L') {
          tokenizer tok(str, sep);
          for (tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
            ++axiscols;
          }
          // if the file contains a comment line starting with "#L" followed
          // by three columns this could be loadsnsspec file
          if (axiscols > 2) {
            snsspec = true;
          }
        }
      } else {
        // check first data line is a 3 column line
        tokenizer tok(str, sep);
        for (tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
          ++datacols;
        }
        break;
      }
    } catch (std::out_of_range &) {
    }
  }
  if (snsspec && datacols == 3) // three column data
  {
    confidence = 80;
  }
  return confidence;
}

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadSNSspec::LoadSNSspec() {}

/// Initialisation method.
void LoadSNSspec::init() {
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
  declareProperty("Unit", "Energy",
                  boost::make_shared<Kernel::StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\") (default: Energy)");
}

/**
*   Executes the algorithm.
*/
void LoadSNSspec::exec() {
  std::string filename = getProperty("Filename");
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
    if (str.empty())
      continue;
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
        spectra[working_with_spectrum_nbr].dataX().push_back(
            input[input.size() - 1]);
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
      spectra[working_with_spectrum_nbr].dataX().push_back(
          input[input.size() - 1]);
    }
  } catch (...) {
  }

  try {
    int nSpectra = spectra_nbr;
    MatrixWorkspace_sptr localWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            WorkspaceFactory::Instance().create("Workspace2D", nSpectra,
                                                nBins + 1, nBins));

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
