// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveGDA.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Unit.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid::DataHandling {

using namespace API;

namespace { // helper functions

const int POINTS_PER_LINE = 4;

double mean(const std::vector<double> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

// Compute the mean resolution of the x axis of the input workspace
// Resolution is calculated as the difference between adjacent pairs of values,
// normalised by the second of the two
double computeAverageDeltaTByT(const HistogramData::HistogramX &tValues) {
  std::vector<double> deltaTByT;
  deltaTByT.reserve(tValues.size() - 1);

  std::adjacent_difference(tValues.begin(), tValues.end(), std::back_inserter(deltaTByT),
                           [](const double previous, const double current) { return (previous - current) / current; });
  // First element is just first element of tValues, so remove it
  deltaTByT.erase(deltaTByT.begin());
  return mean(deltaTByT);
}

std::string generateBankHeader(int bank, int minT, size_t numberBins, double deltaTByT) {
  std::stringstream stream;
  const auto numberLines = static_cast<size_t>(std::ceil(static_cast<double>(numberBins) / POINTS_PER_LINE));

  stream << std::setprecision(2) << "BANK " << bank << " " << numberBins << "  " << numberLines << " RALF  " << minT
         << "  96  " << minT << " " << deltaTByT << " ALT";
  return stream.str();
}

std::optional<std::vector<std::string>> getParamLinesFromGSASFile(const std::string &paramsFilename) {
  // ICONS signifies that a line contains TOF to D conversion factors
  const static std::string paramLineDelimiter = "ICONS";
  std::ifstream paramsFile;
  paramsFile.open(paramsFilename);

  if (paramsFile.is_open()) {
    std::vector<std::string> paramLines;
    std::string line;
    while (std::getline(paramsFile, line)) {
      if (line.find(paramLineDelimiter) != std::string::npos) {
        paramLines.emplace_back(line);
      }
    }
    return paramLines;
  } else {
    return std::nullopt;
  }
}

} // anonymous namespace

DECLARE_ALGORITHM(SaveGDA)

SaveGDA::CalibrationParams::CalibrationParams(const double _difc, const double _difa, const double _tzero)
    : difa(_difa), difc(_difc), tzero(_tzero) {}

const std::string SaveGDA::name() const { return "SaveGDA"; }

const std::string SaveGDA::summary() const {
  return "Save a group of focused banks to the MAUD three-column GDA format";
}

int SaveGDA::version() const { return 1; }

const std::vector<std::string> SaveGDA::seeAlso() const { return {"SaveBankScatteringAngles"}; }

const std::string SaveGDA::category() const { return "DataHandling\\Text;Diffraction\\DataHandling"; }

const std::string SaveGDA::PROP_OUTPUT_FILENAME = "OutputFilename";

const std::string SaveGDA::PROP_INPUT_WS = "InputWorkspace";

const std::string SaveGDA::PROP_PARAMS_FILENAME = "GSASParamFile";

const std::string SaveGDA::PROP_GROUPING_SCHEME = "GroupingScheme";

void SaveGDA::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PROP_INPUT_WS, "", Kernel::Direction::Input),
                  "A GroupWorkspace where every sub-workspace is a "
                  "single-spectra focused run corresponding to a particular "
                  "bank");

  const static std::vector<std::string> outExts{".gda"};
  declareProperty(std::make_unique<FileProperty>(PROP_OUTPUT_FILENAME, "", FileProperty::Save, outExts),
                  "The name of the file to save to");

  const static std::vector<std::string> paramsExts{".ipf", ".prm", ".parm", ".iprm"};
  declareProperty(std::make_unique<FileProperty>(PROP_PARAMS_FILENAME, "", FileProperty::Load, paramsExts),
                  "GSAS calibration file containing conversion factors from D to TOF");

  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>(PROP_GROUPING_SCHEME),
                  "An array of bank IDs, where the value at element i is the "
                  "ID of the bank in " +
                      PROP_PARAMS_FILENAME + " to associate spectrum i with");
}

void SaveGDA::exec() {
  const std::string filename = getProperty(PROP_OUTPUT_FILENAME);
  std::ofstream outFile(filename.c_str());

  if (!outFile) {
    throw Kernel::Exception::FileError("Unable to create file: ", filename);
  }

  outFile << std::fixed << std::setprecision(0) << std::setfill(' ');

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  const auto calibParams = parseParamsFile();
  const std::vector<int> groupingScheme = getProperty(PROP_GROUPING_SCHEME);

  for (int i = 0; i < inputWS->getNumberOfEntries(); ++i) {
    const auto ws = inputWS->getItem(i);
    const auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws);

    auto x = matrixWS->dataX(0);
    const size_t bankIndex(groupingScheme[i] - 1);
    if (bankIndex >= calibParams.size()) {
      throw Kernel::Exception::IndexError(bankIndex, calibParams.size(), "Bank number out of range");
    }
    const auto &bankCalibParams = calibParams[bankIndex];

    // For historic reasons, TOF is scaled by 32 in MAUD
    const static double tofScale = 32;
    std::vector<double> tofScaled;
    tofScaled.reserve(x.size());
    Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> yunused;
    dSpacingUnit.toTOF(x, yunused, 0., Kernel::DeltaEMode::Elastic,
                       {{Kernel::UnitParams::difa, bankCalibParams.difa},
                        {Kernel::UnitParams::difc, bankCalibParams.difc},
                        {Kernel::UnitParams::tzero, bankCalibParams.tzero}});
    std::transform(x.begin(), x.end(), std::back_inserter(tofScaled),
                   [](const double tofVal) { return tofVal * tofScale; });
    const auto averageDeltaTByT = computeAverageDeltaTByT(tofScaled);

    const auto &intensity = matrixWS->y(0);
    const auto &error = matrixWS->e(0);
    const auto numPoints = std::min({tofScaled.size(), intensity.size(), error.size()});

    const auto header =
        generateBankHeader(i + 1, static_cast<int>(std::round(tofScaled[0])), numPoints, averageDeltaTByT);

    outFile << std::left << std::setw(80) << header << '\n' << std::right;

    for (size_t j = 0; j < numPoints; ++j) {
      outFile << std::setw(8) << tofScaled[j] << std::setw(7) << intensity[j] * 1000 << std::setw(5) << error[j] * 1000;

      if (j % POINTS_PER_LINE == POINTS_PER_LINE - 1) {
        // new line every 4 points
        outFile << '\n';
      } else if (j == numPoints - 1) {
        // make sure line is 80 characters long
        outFile << std::string(80 - (i % POINTS_PER_LINE + 1) * 20, ' ') << '\n';
      }
    }
  }
}

std::map<std::string, std::string> SaveGDA::validateInputs() {
  std::map<std::string, std::string> issues;
  std::optional<std::string> inputWSIssue;

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  for (const auto &ws : *inputWS) {
    const auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrixWS) {
      if (matrixWS->getNumberHistograms() != 1) {
        inputWSIssue = "The workspace " + matrixWS->getName() +
                       " has the wrong number of histograms. It "
                       "should contain data for a single focused "
                       "spectra";
      } else if (matrixWS->getAxis(0)->unit()->unitID() != "dSpacing") {
        inputWSIssue = "The workspace " + matrixWS->getName() +
                       " has incorrect units. SaveGDA "
                       "expects input workspaces with "
                       "units of D-spacing";
      }
    } else { // not matrixWS
      inputWSIssue = "The workspace " + ws->getName() + " is of the wrong type. It should be a MatrixWorkspace";
    }
  }
  if (inputWSIssue) {
    issues[PROP_INPUT_WS] = *inputWSIssue;
  }

  const std::vector<int> groupingScheme = getProperty(PROP_GROUPING_SCHEME);
  const auto numSpectraInGroupingScheme = groupingScheme.size();
  const auto numSpectraInWS = static_cast<size_t>(inputWS->getNumberOfEntries());
  if (numSpectraInGroupingScheme != numSpectraInWS) {
    issues[PROP_GROUPING_SCHEME] = "The grouping scheme must contain one entry for every focused spectrum "
                                   "in the input workspace. " +
                                   PROP_GROUPING_SCHEME + " has " + std::to_string(numSpectraInGroupingScheme) +
                                   " entries whereas " + PROP_INPUT_WS + " has " + std::to_string(numSpectraInWS);
  }

  return issues;
}

std::vector<SaveGDA::CalibrationParams> SaveGDA::parseParamsFile() const {
  const std::string paramsFilename = getProperty(PROP_PARAMS_FILENAME);
  const auto paramLines = getParamLinesFromGSASFile(paramsFilename);
  if (!paramLines) {
    g_log.error(strerror(errno));
    throw Kernel::Exception::FileError("Could not read GSAS parameter file", paramsFilename);
  }
  std::vector<CalibrationParams> calibParams;
  for (const auto &paramLine : *paramLines) {
    std::vector<std::string> lineItems;
    boost::algorithm::split(lineItems, paramLine, boost::is_any_of("\t "), boost::token_compress_on);
    calibParams.emplace_back(std::stod(lineItems[3]), std::stod(lineItems[4]), std::stod(lineItems[5]));
  }
  return calibParams;
}

} // namespace Mantid::DataHandling
