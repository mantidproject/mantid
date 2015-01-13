#include "MantidCrystal/FilterPeaks.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace {
double HKLSum(const Mantid::API::IPeak &p) {
  return p.getH() + p.getK() + p.getL();
}

double HKL2(const Mantid::API::IPeak &p) {
  return p.getH() * p.getH() + p.getK() * p.getK() + p.getL() * p.getL();
}

double intensity(const Mantid::API::IPeak &p) { return p.getIntensity(); }

double SN(const Mantid::API::IPeak &p) {
  return p.getIntensity() / p.getSigmaIntensity();
}
}

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FilterPeaks)

using namespace Kernel;
using namespace API;
using DataObjects::PeaksWorkspace;
using DataObjects::PeaksWorkspace_const_sptr;
using DataObjects::PeaksWorkspace_sptr;
using DataObjects::Peak;

/** Constructor
 */
FilterPeaks::FilterPeaks() {}

/** Destructor
 */
FilterPeaks::~FilterPeaks() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string FilterPeaks::name() const { return "FilterPeaks"; };
/// Algorithm's version for identification. @see Algorithm::version
int FilterPeaks::version() const { return 1; };
/// Algorithm's category for identification. @see Algorithm::category
const std::string FilterPeaks::category() const { return "Crystal"; }

/** Initialize the algorithm's properties.
 */
void FilterPeaks::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "The input workspace");
  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The filtered workspace");

  std::vector<std::string> filters;
  filters.push_back("h+k+l");
  filters.push_back("h^2+k^2+l^2");
  filters.push_back("Intensity");
  filters.push_back("Signal/Noise");
  declareProperty("FilterVariable", "",
                  boost::make_shared<StringListValidator>(filters),
                  "The variable on which to filter the peaks");

  declareProperty("FilterValue", EMPTY_DBL(),
                  boost::make_shared<MandatoryValidator<double>>(),
                  "The value of the FilterVariable to compare each peak to");

  std::vector<std::string> operation;
  operation.push_back("<");
  operation.push_back(">");
  operation.push_back("=");
  operation.push_back("<=");
  operation.push_back(">=");
  declareProperty("Operator", "<",
                  boost::make_shared<StringListValidator>(operation), "");
}

/** Execute the algorithm.
 */
void FilterPeaks::exec() {
  PeaksWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  IPeaksWorkspace_sptr filteredWS = WorkspaceFactory::Instance().createPeaks();
  // Copy over ExperimentInfo from input workspace
  filteredWS->copyExperimentInfoFrom(inputWS.get());

  const std::string FilterVariable = getProperty("FilterVariable");
  double (*filterFunction)(const Mantid::API::IPeak &) = 0;
  if (FilterVariable == "h+k+l")
    filterFunction = &HKLSum;
  else if (FilterVariable == "h^2+k^2+l^2")
    filterFunction = &HKL2;
  else if (FilterVariable == "Intensity")
    filterFunction = &intensity;
  else if (FilterVariable == "Signal/Noise")
    filterFunction = &SN;

  const double FilterValue = getProperty("FilterValue");
  const std::string Operator = getProperty("Operator");

  for (int i = 0; i < inputWS->getNumberPeaks(); ++i) {
    bool pass(false);
    const API::IPeak &currentPeak = inputWS->getPeak(i);
    const double currentValue =
        filterFunction(currentPeak); // filterFunction pointer set above

    if (Operator == "<")
      pass = (currentValue < FilterValue);
    else if (Operator == ">")
      pass = (currentValue > FilterValue);
    else if (Operator == "=")
      pass = (currentValue == FilterValue);
    else if (Operator == "<=")
      pass = (currentValue <= FilterValue);
    else if (Operator == ">=")
      pass = (currentValue >= FilterValue);

    if (pass)
      filteredWS->addPeak(currentPeak);
  }

  setProperty("OutputWorkspace", filteredWS);
}

} // namespace Crystal
} // namespace Mantid
