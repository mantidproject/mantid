#include "MantidDataHandling/SaveGDA.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/make_unique.h"

#include <boost/optional.hpp>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Mantid {
namespace DataHandling {

using namespace API;

namespace { // helper functions

double mean(const std::vector<double> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

double computeAverageDeltaTByT(const HistogramData::HistogramX &tValues) {
  std::vector<double> deltaTByT;
  deltaTByT.reserve(tValues.size() - 1);

  std::adjacent_difference(tValues.begin(), tValues.end(),
                           std::back_inserter(deltaTByT),
                           [](const double previous, const double current) {
                             return (previous - current) / current;
                           });
  // First element is just first element of tValues, so remove it
  deltaTByT.erase(deltaTByT.begin());
  return mean(deltaTByT);
}

std::string generateBankHeader(int bank, int minT, size_t numberBins,
                               double deltaTByT) {
  std::stringstream stream;
  stream << std::setprecision(2) << "BANK " << bank << " " << numberBins << "  "
         << numberBins / 4 << " RALF  " << minT << "  96  " << minT << " "
         << deltaTByT << " ALT";
  return stream.str();
}

} // anonymous namespace

DECLARE_ALGORITHM(SaveGDA)

const std::string SaveGDA::name() const { return "SaveGDA"; }

const std::string SaveGDA::summary() const {
  return "Save a group of focused banks to the MAUD three-column GDA format";
}

int SaveGDA::version() const { return 1; }

const std::vector<std::string> SaveGDA::seeAlso() const {
  return {"SaveBankScatteringAngles", "SaveGSS", "SaveFocusedXYE"};
}

const std::string SaveGDA::category() const {
  return "DataHandling\\Text;Diffraction\\DataHandling";
}

const std::string SaveGDA::PROP_FILENAME = "Filename";

const std::string SaveGDA::PROP_INPUT_WS = "InputWorkspace";

void SaveGDA::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      PROP_INPUT_WS, "", Kernel::Direction::Input),
                  "A GroupWorkspace where every sub-workspace is a "
                  "single-spectra focused run corresponding to a particular "
                  "bank");

  const static std::vector<std::string> exts{".gda"};
  declareProperty(Kernel::make_unique<FileProperty>(PROP_FILENAME, "",
                                                    FileProperty::Save, exts),
                  "The name of the file to save to");
}

void SaveGDA::exec() {
  const std::string filename = getProperty(PROP_FILENAME);
  std::ofstream outFile(filename.c_str());

  if (!outFile) {
    throw Kernel::Exception::FileError("Unable to create file: ", filename);
  }

  outFile << std::fixed << std::setprecision(0) << std::setfill(' ');

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  for (int i = 0; i < inputWS->getNumberOfEntries(); ++i) {
    const auto ws = inputWS->getItem(i);
    const auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);

    const auto &tof = matrixWS->x(0);
    std::vector<double> tofScaled;
    tofScaled.reserve(tof.size());
    std::transform(tof.begin(), tof.end(), std::back_inserter(tofScaled),
                   [](const double t) { return t * 32; });

    const auto &intensity = matrixWS->y(0);
    const auto &error = matrixWS->e(0);
    const auto numPoints =
        std::min({tofScaled.size(), intensity.size(), error.size()});

    const auto averageDeltaTByT = computeAverageDeltaTByT(tof);
    const auto header = generateBankHeader(i + 1, (int)std::round(tofScaled[0]),
                                           numPoints, averageDeltaTByT);

    outFile << std::left << std::setw(80) << header << '\n' << std::right;

    for (size_t j = 0; j < numPoints; ++j) {
      outFile << std::setw(8) << tofScaled[j] << std::setw(7)
              << intensity[j] * 1000 << std::setw(5) << error[j] * 1000;

      if (j % 4 == 3) {
        // new line every 4 points
        outFile << '\n';
      }  else if (j == numPoints - 1) {
        // make sure line is 80 characters long
        outFile << std::string(80 - (i % 4 + 1) * 20, ' ') << '\n';
      }
    }
  }
}

std::map<std::string, std::string> SaveGDA::validateInputs() {
  std::map<std::string, std::string> issues;
  boost::optional<std::string> inputWSIssue;

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  for (const auto &ws : *inputWS) {
    const auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrixWS) {
      if (matrixWS->getNumberHistograms() != 1) {
        inputWSIssue = "The workspace " + matrixWS->getName() +
                       " has the wrong number of histograms. It "
                       "should contain data for a single focused "
                       "spectra";
      } else if (matrixWS->getAxis(0)->unit()->unitID() != "TOF") {
        inputWSIssue = "The workspace " + matrixWS->getName() +
                       " has incorrect units. SaveGDA "
                       "expects input workspaces with "
                       "units of TOF";
      }
    } else { // not matrixWS
      inputWSIssue = "The workspace " + ws->getName() +
                     " is of the wrong type. It should be a MatrixWorkspace";
    }
  }
  if (inputWSIssue) {
    issues[PROP_INPUT_WS] = *inputWSIssue;
  }
  return issues;
}

} // DataHandling
} // Mantid
