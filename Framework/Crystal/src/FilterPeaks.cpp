// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FilterPeaks.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using Mantid::DataObjects::Peak;

namespace {

// Filter functions
double intensity(const Mantid::Geometry::IPeak &p) { return p.getIntensity(); }

double wavelength(const Mantid::Geometry::IPeak &p) { return p.getWavelength(); }

double dspacing(const Mantid::Geometry::IPeak &p) { return p.getDSpacing(); }

double tof(const Mantid::Geometry::IPeak &p) { return p.getTOF(); }

double HKLSum(const Mantid::Geometry::IPeak &p) { return p.getH() + p.getK() + p.getL(); }

double HKL2(const Mantid::Geometry::IPeak &p) { return p.getIntHKL().norm2(); }

double MNP2(const Mantid::Geometry::IPeak &p) { return p.getIntMNP().norm2(); }

double QMOD(const Mantid::Geometry::IPeak &p) { return p.getQSampleFrame().norm(); }

double SN(const Mantid::Geometry::IPeak &p) {
  return (p.getSigmaIntensity() > 0) ? (p.getIntensity() / p.getSigmaIntensity()) : (0.0);
}

double RUN(const Mantid::Geometry::IPeak &p) { return p.getRunNumber(); }

std::string BANKNAME(const Mantid::Geometry::IPeak &p) {
  const Peak &fullPeak = dynamic_cast<const Peak &>(p);
  return fullPeak.getBankName();
}

} // namespace

// namespace
namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FilterPeaks)

using namespace Kernel;
using namespace API;
using API::IPeaksWorkspace;
using API::IPeaksWorkspace_const_sptr;
using API::IPeaksWorkspace_sptr;

/// Algorithm's name for identification. @see Algorithm::name
const std::string FilterPeaks::name() const { return "FilterPeaks"; }
/// Algorithm's version for identification. @see Algorithm::version
int FilterPeaks::version() const { return 1; }
/// Algorithm's category for identification. @see Algorithm::category
const std::string FilterPeaks::category() const { return "Crystal\\Peaks"; }

/** Initialize the algorithm's properties.
 */
void FilterPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The filtered workspace");

  // filter by property
  const std::string FILTER("Filter Options");
  std::vector<std::string> filters{"h+k+l", "h^2+k^2+l^2", "m^2+n^2+p^2", "Intensity", "Signal/Noise",
                                   "QMod",  "Wavelength",  "DSpacing",    "TOF",       "RunNumber"};
  declareProperty("FilterVariable", "h+k+l", std::make_shared<StringListValidator>(filters),
                  "The variable on which to filter the peaks");

  declareProperty("FilterValue", EMPTY_DBL(), "The value of the FilterVariable to compare each peak to");

  std::vector<std::string> operation{"<", ">", "=", "!=", "<=", ">="};
  declareProperty("Operator", "<", std::make_shared<StringListValidator>(operation), "");
  setPropertyGroup("FilterVariable", FILTER);
  setPropertyGroup("FilterValue", FILTER);
  setPropertyGroup("Operator", FILTER);

  // select by bankname
  const std::string SELECT("Select Bank by Name");
  std::vector<std::string> action{"=", "!="};
  declareProperty("Criterion", "=", std::make_shared<StringListValidator>(action), "");
  declareProperty("BankName", "", "Selected bank name, empty means skip selection. Applicable only to PeaksWorkspace");
  setPropertyGroup("Criterion", SELECT);
  setPropertyGroup("BankName", SELECT);
}

/** Execute the algorithm.
 */
void FilterPeaks::exec() {
  IPeaksWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  IPeaksWorkspace_sptr filteredWS =
      std::dynamic_pointer_cast<IPeaksWorkspace>(WorkspaceFactory::Instance().createPeaks(inputWS->id()));

  // Copy over ExperimentInfo from input workspace
  filteredWS->copyExperimentInfoFrom(inputWS.get());

  const double filterValue = getProperty("FilterValue");
  const std::string Operator = getProperty("Operator");
  const std::string filterVariable = getProperty("FilterVariable");

  const std::string bankname = getProperty("BankName");
  const std::string criterion = getProperty("Criterion");

  if (!bankname.empty() && (inputWS->id() == "PeaksWorkspace")) {
    FilterFunctionStr filterFunction = &BANKNAME;
    IPeaksWorkspace_sptr selectedWS = filteredWS->clone();

    if (criterion == "=")
      filterPeaksStr<std::equal_to<std::string>>(*inputWS, *selectedWS, filterFunction, bankname);
    else if (criterion == "!=")
      filterPeaksStr<std::not_equal_to<std::string>>(*inputWS, *selectedWS, filterFunction, bankname);
    else
      throw std::invalid_argument("Unsupported operator " + criterion + " for BankName filter");

    inputWS = selectedWS;
    setProperty("OutputWorkspace", selectedWS);
  }

  if (!isDefault("FilterValue")) {
    const auto filterFunction = getFilterVariableFunction(filterVariable);
    // Choose which version of the function to u.se based on the operator
    if (Operator == "<")
      filterPeaks<std::less<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else if (Operator == ">")
      filterPeaks<std::greater<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else if (Operator == "=")
      filterPeaks<std::equal_to<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else if (Operator == "!=")
      filterPeaks<std::not_equal_to<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else if (Operator == "<=")
      filterPeaks<std::less_equal<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else if (Operator == ">=")
      filterPeaks<std::greater_equal<double>>(*inputWS, *filteredWS, filterFunction, filterValue);
    else
      throw std::invalid_argument("Unknown Operator " + Operator);

    setProperty("OutputWorkspace", filteredWS);
  }
}

/** Get the filter function to use to filter peaks
 *
 * This will return a std::function object that takes a peak
 * and returns a double representing the current value of this
 * peak to be compared against the user's filter value.
 *
 * @param filterVariable :: a string representing the filter function to use.
 * e.g. "h+k+l" or "TOF".
 * @return a function object which will return the a value for a given peak
 */
FilterPeaks::FilterFunction FilterPeaks::getFilterVariableFunction(const std::string &filterVariable) const {
  FilterFunction filterFunction;
  if (filterVariable == "h+k+l")
    filterFunction = &HKLSum;
  else if (filterVariable == "h^2+k^2+l^2")
    filterFunction = &HKL2;
  else if (filterVariable == "m^2+n^2+p^2")
    filterFunction = &MNP2;
  else if (filterVariable == "Intensity")
    filterFunction = &intensity;
  else if (filterVariable == "Wavelength")
    filterFunction = &wavelength;
  else if (filterVariable == "DSpacing")
    filterFunction = &dspacing;
  else if (filterVariable == "TOF")
    filterFunction = &tof;
  else if (filterVariable == "Signal/Noise")
    filterFunction = &SN;
  else if (filterVariable == "QMod")
    filterFunction = &QMOD;
  else if (filterVariable == "RunNumber")
    filterFunction = &RUN;
  else
    throw std::invalid_argument("Unknown FilterVariable: " + filterVariable);
  return filterFunction;
}

} // namespace Mantid::Crystal
