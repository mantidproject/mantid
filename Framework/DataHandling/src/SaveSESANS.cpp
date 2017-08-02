#include "MantidDataHandling/SaveSESANS.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>

namespace Mantid {
namespace DataHandling {

// Register the algorithm with the AlgorithmFactory
DECLARE_ALGORITHM(SaveSESANS)

/// Get the algorithm's name
const std::string SaveSESANS::name() const { return "SaveSESANS"; }

/// Get a summary of the algorithm
const std::string SaveSESANS::summary() const {
  return "Save a file using the SESANS format";
}

/// Get the version number of the algorithm
int SaveSESANS::version() const { return 1; }

/// Get the algorithm's category
const std::string SaveSESANS::category() const { return "DataHandling\\Text"; }

/**
 * Validate user inputs
 * @return A map containing the names of any invalid properties and a description of the error
 */
std::map<std::string, std::string> SaveSESANS::validateInputs(){
  std::map<std::string, std::string> invalidProperties;

  for (auto propertyName: nonEmptyDoubleProperties){
    //    if (getProperty(propertyName) == EMPTY_DBL()){
    //invalidProperties[propertyName] = propertyName + " must be set";
    //}
  }

  return invalidProperties;
}
  
/**
 * Initialise the algorithm
 */
void SaveSESANS::init() {
  auto notDBL_MIN = boost::make_shared<Kernel::BoundedValidator<double>>();
  notDBL_MIN->setLower(DBL_MIN);

  auto validOrientation = boost::make_shared<Kernel::StringListValidator>(std::set<std::string>{"X", "Y", "Z"});
  
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "InputWorkspace", "", Kernel::Direction::Input),
                  "The name of the workspace to save");
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, fileExtensions),
                  "The name to use when saving the file");

  declareProperty("ThetaZMax", DBL_MIN, notDBL_MIN, "Theta_zmax", Kernel::Direction::Input);
  declareProperty("ThetaZMaxUnit", "radians", Kernel::Direction::Input);
  declareProperty("ThetaYMax", DBL_MIN, notDBL_MIN, "Theta_ymax", Kernel::Direction::Input);
  declareProperty("ThetaYMaxUnit", "radians", Kernel::Direction::Input);
  declareProperty("EchoConstant", DBL_MIN, notDBL_MIN, "Echo_constant", Kernel::Direction::Input);

  declareProperty<std::string>("Orientation", "Z", validOrientation,
                               "Orientation of the instrument");
}

/**
 * Execute the algorithm
 */
void SaveSESANS::exec() {
  API::MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");

  // Check workspace has only one spectrum
  if (ws->getNumberHistograms() != 1) {
    g_log.error("This algorithm expects a workspace with exactly 1 spectrum");
    throw std::runtime_error("SaveSESANS passed workspace with incorrect "
                             "number of spectra, expected 1");
  }

  std::ofstream outfile;
  outfile.open(getPropertyValue("Filename"));
  writeHeaders(outfile, ws);
  outfile << "\n"
          << "BEGIN_DATA"
          << "\n";

  const auto &wavelength = ws->points(0);
  const auto &yValues = ws->y(0);
  const auto &eValues = ws->e(0);

  const auto spinEchoLength = calculateSpinEchoLength(wavelength);
  const auto depolarisation = calculateDepolarisation(yValues, wavelength);
  const auto error = calculateError(eValues, yValues, wavelength);

  outfile << "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n";

  for (size_t i = 0; i < spinEchoLength.size(); ++i) {
    outfile << spinEchoLength[i] << " ";
    outfile << depolarisation[i] << " ";
    outfile << error[i] << " ";
    outfile << wavelength[i] << "\n";
  }

  outfile.close();
}

/**
 * Write header values to the output file
 * @param outfile ofstream to the output file
 * @param ws The workspace to save
 */
void SaveSESANS::writeHeaders(std::ofstream &outfile,
                              API::MatrixWorkspace_const_sptr &ws) {
  const API::Sample &sample = ws->sample();

  writeHeader(outfile, "FileFormatVersion", "1.0");
  writeHeader(outfile, "DataFileTitle", ws->getTitle());
  writeHeader(outfile, "Sample", sample.getName());
  writeHeader(outfile, "Thickness", std::to_string(sample.getThickness()));
  writeHeader(outfile, "Thickness_unit", "mm");
  writeHeader(outfile, "Theta_zmax", getPropertyValue("ThetaZMax"));
  writeHeader(outfile, "Theta_zmax_unit", getPropertyValue("ThetaZMaxUnit"));
  writeHeader(outfile, "Theta_ymax", getPropertyValue("ThetaYMax"));
  writeHeader(outfile, "Theta_ymax_unit", getPropertyValue("ThetaYMaxUnit"));
  writeHeader(outfile, "Orientation", "Z");
  writeHeader(outfile, "SpinEchoLength_unit", "A");
  writeHeader(outfile, "Depolarisation_unit", "A-2 cm-1");
  writeHeader(outfile, "Wavelength_unit", "A");
  writeHeader(outfile, "Echo_constant", getPropertyValue("EchoConstant"));
}

/**
 * Write a single header to the output file
 * @param outfile ofstream to the output file
 * @param name The name of the attribute being written
 * @param value The attribute's value
*/
void SaveSESANS::writeHeader(std::ofstream &outfile, const std::string &name,
                             const std::string &value) {
  outfile << std::setfill(' ') << std::setw(MAX_HDR_LENGTH) << std::left << name
          << value << "\n";
}

/**
 * Calculate SpinEchoLength column from wavelength
 * ( SEL = wavelength * wavelength * echoConstant)
 * @param wavelength The wavelength column
 * @return The corresponding SpinEchoLength column
 */
std::vector<double> SaveSESANS::calculateSpinEchoLength(
    const HistogramData::Points &wavelength) const {
  std::vector<double> spinEchoLength;
  const double echoConstant = getProperty("EchoConstant");

  // SEL is calculated as wavelength^2 * echoConstant
  transform(wavelength.begin(), wavelength.end(), back_inserter(spinEchoLength),
            [&](double w) { return w * w * echoConstant; });
  return spinEchoLength;
}

/**
 * Calculate the depolarisation column from wavelength and Y values in the
 * workspace
 * (depol = ln(y) / wavelength^2)
 * @param yValues Y column in the workspace
 * @param wavelength Wavelength column
 * @return The depolarisation column
 */
std::vector<double> SaveSESANS::calculateDepolarisation(
    const HistogramData::HistogramY &yValues,
    const HistogramData::Points &wavelength) const {
  Mantid::MantidVec depolarisation;

  // Depol is calculated as ln(y) / wavelength^2
  transform(yValues.begin(), yValues.end(), wavelength.begin(),
            back_inserter(depolarisation),
            [&](double y, double w) { return log(y) / (w * w); });
  return depolarisation;
}

/**
 * Calculate the error column from the workspace values
 * (error = e / (y * wavelength^2))
 * @param eValues Error values from the workspace
 * @param yValues Y values from the workspace
 * @param wavelength Wavelength values (X values from the workspace)
 * @return The error column
 */
Mantid::MantidVec
SaveSESANS::calculateError(const HistogramData::HistogramE &eValues,
                           const HistogramData::HistogramY &yValues,
                           const HistogramData::Points &wavelength) const {
  Mantid::MantidVec error;

  // Error is calculated as e / (y * wavelength^2)
  for (size_t i = 0; i < eValues.size(); i++) {
    error.push_back(eValues[i] / (yValues[i] * wavelength[i] * wavelength[i]));
  }
  return error;
}

} // namespace DataHandling
} // namespace Mantid
