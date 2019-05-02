// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SortXAxis.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/make_unique.h"
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(SortXAxis)

const std::string SortXAxis::name() const { return "SortXAxis"; }

int SortXAxis::version() const { return 1; }

const std::string SortXAxis::category() const {
  return "Transforms\\Axes;Utility\\Sorting";
}

const std::string SortXAxis::summary() const {
  return "Clones the input MatrixWorkspace(s) and orders the x-axis in an "
         "ascending or descending fashion.";
}

void SortXAxis::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input Workspace");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Sorted Output Workspace");

  auto orderingValues = std::vector<std::string>({"Ascending", "Descending"});
  auto orderingValidator =
      boost::make_shared<StringListValidator>(orderingValues);
  declareProperty("Ordering", orderingValues[0], orderingValidator,
                  "Ascending or descending sorting", Direction::Input);
  declareProperty("IgnoreHistogramValidation", false,
                  "This will stop SortXAxis from throwing if the workspace is "
                  "not a valid histogram for this algorithm to work on. THIS "
                  "IS TEMPORARY, this item will be removed for 4.1 and thus "
                  "should only be used internally for the TOSCA legacy data "
                  "in indirect .");
}

void SortXAxis::exec() {

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = inputWorkspace->clone();
  const bool ignoreHistogramValidation =
      getProperty("IgnoreHistogramValidation");

  // Check if it is a valid histogram here
  const bool isAProperHistogram =
      (!ignoreHistogramValidation)
          ? determineIfHistogramIsValid(*inputWorkspace)
          : true; // Assume a valid histogram if validation is to be ignored

  // Define everything you can outside of the for loop
  // Assume that all spec are the same size
  const auto sizeOfX = inputWorkspace->x(0).size();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int specNum = 0u; specNum < (int)inputWorkspace->getNumberHistograms();
       specNum++) {
    PARALLEL_START_INTERUPT_REGION
    auto workspaceIndicies = createIndexes(sizeOfX);

    sortIndicesByX(workspaceIndicies, getProperty("Ordering"), *inputWorkspace,
                   specNum);

    copyToOutputWorkspace(workspaceIndicies, *inputWorkspace, *outputWorkspace,
                          specNum, isAProperHistogram);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

/**
 * @brief Gets a vector of numbers from 0 to the sizeOfX-1 and returns it
 *
 * @param sizeOfX The size of the Spectrum's X axis
 * @return std::vector<std::size_t>
 */
std::vector<std::size_t> SortXAxis::createIndexes(const size_t sizeOfX) {
  std::vector<std::size_t> workspaceIndicies;
  workspaceIndicies.reserve(sizeOfX);
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
    workspaceIndicies.emplace_back(workspaceIndex);
  }
  return workspaceIndicies;
}

/**
 * @brief A template for sorting the values given a comparator
 *
 * @tparam Comparator
 * @param workspaceIndicies the vector of indicies values
 * @param inputWorkspace the original workspace
 * @param specNum the Spectrum number to be sorted
 * @param compare std::less<double> for Ascending order std::greater<double>
 * for descending order
 */
template <typename Comparator>
void sortByXValue(std::vector<std::size_t> &workspaceIndicies,
                  const Mantid::API::MatrixWorkspace &inputWorkspace,
                  unsigned int specNum, Comparator const &compare) {
  std::sort(workspaceIndicies.begin(), workspaceIndicies.end(),
            [&](std::size_t lhs, std::size_t rhs) -> bool {
              return compare(inputWorkspace.x(specNum)[lhs],
                             inputWorkspace.x(specNum)[rhs]);
            });
}

void SortXAxis::sortIndicesByX(
    std::vector<std::size_t> &workspaceIndicies, std::string order,
    const Mantid::API::MatrixWorkspace &inputWorkspace, unsigned int specNum) {
  if (order == "Ascending") {
    sortByXValue(workspaceIndicies, inputWorkspace, specNum,
                 std::less<double>());
  } else if (order == "Descending") {
    sortByXValue(workspaceIndicies, inputWorkspace, specNum,
                 std::greater<double>());
  }
}

/**
 * @brief Copies the sorted inputworkspace into the output workspace without
 * using clone because of how histograms are supported, for the X Axis and the
 * Dx Axis.
 *
 * @param workspaceIndicies the sorted vector of indecies
 * @param inputWorkspace the unsorted initial workspace
 * @param outputWorkspace the emptry output workspace
 * @param specNum the Spectrum it is currently copying over
 */
void SortXAxis::copyXandDxToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum) {
  // Move an ordered X to the output workspace
  for (auto workspaceIndex = 0u;
       workspaceIndex < inputWorkspace.x(specNum).size(); workspaceIndex++) {
    outputWorkspace.mutableX(specNum)[workspaceIndex] =
        inputWorkspace.x(specNum)[workspaceIndicies[workspaceIndex]];
  }

  // If Dx's are present, move Dx's to the output workspace
  // If Dx's are present, move Dx's to the output workspace
  if (inputWorkspace.hasDx(specNum)) {
    for (auto workspaceIndex = 0u;
         workspaceIndex < inputWorkspace.dx(specNum).size(); workspaceIndex++) {
      outputWorkspace.mutableDx(specNum)[workspaceIndex] =
          inputWorkspace.dx(specNum)[workspaceIndicies[workspaceIndex]];
    }
  }
}

/**
 * @brief Copies the sorted inputworkspace into the output workspace without
 * using clone because of how histograms are supported, for the Y Axis and the E
 * Axis.
 *
 * @param workspaceIndicies the sorted vector of indicies
 * @param inputWorkspace the unsorted input workspaces
 * @param outputWorkspace the empty output workspace
 * @param specNum the spectrum number being copied into
 * @param isAProperHistogram whether or not it has been determined to be a valid
 * histogram earlier on.
 */
void SortXAxis::copyYandEToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum,
    bool isAProperHistogram) {
  // If Histogram data find the biggest index value and remove it from
  // workspaceIndicies
  if (isAProperHistogram) {
    auto lastIndexIt =
        std::find(workspaceIndicies.begin(), workspaceIndicies.end(),
                  inputWorkspace.y(specNum).size());
    workspaceIndicies.erase(lastIndexIt);
  }

  auto &inSpaceY = inputWorkspace.y(specNum);
  for (auto workspaceIndex = 0u;
       workspaceIndex < inputWorkspace.y(specNum).size(); workspaceIndex++) {
    outputWorkspace.mutableY(specNum)[workspaceIndex] =
        inSpaceY[workspaceIndicies[workspaceIndex]];
  }

  auto &inSpaceE = inputWorkspace.e(specNum);
  for (auto workspaceIndex = 0u;
       workspaceIndex < inputWorkspace.e(specNum).size(); workspaceIndex++) {
    outputWorkspace.mutableE(specNum)[workspaceIndex] =
        inSpaceE[workspaceIndicies[workspaceIndex]];
  }
}

void SortXAxis::copyToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum,
    bool isAProperHistogram) {
  copyXandDxToOutputWorkspace(workspaceIndicies, inputWorkspace,
                              outputWorkspace, specNum);
  copyYandEToOutputWorkspace(workspaceIndicies, inputWorkspace, outputWorkspace,
                             specNum, isAProperHistogram);
}

/**
 * @brief determines whether or not a given spectrum is sorted based on a passed
 * comparator
 *
 * @tparam Comparator
 * @param compare std::less<double> for descending and std::greater<double> for
 * ascending
 * @param inputWorkspace the unsorted input workspace
 * @return true if it is sorted
 * @return false if it is not sorted
 */
template <typename Comparator>
bool isItSorted(Comparator const &compare,
                const Mantid::API::MatrixWorkspace &inputWorkspace) {
  for (auto specNum = 0u; specNum < inputWorkspace.getNumberHistograms();
       specNum++) {
    if (!std::is_sorted(inputWorkspace.x(specNum).begin(),
                        inputWorkspace.x(specNum).end(),
                        [&](double lhs, double rhs) -> bool {
                          return compare(lhs, rhs);
                        })) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Determines whether it is a valid histogram or not.
 *
 * @param inputWorkspace the unsorted input workspace
 * @return true if it is a valid histogram else produce a runtime_error
 * @return false if it is not a histogram, and is thus point data
 */
bool SortXAxis::determineIfHistogramIsValid(
    const Mantid::API::MatrixWorkspace &inputWorkspace) {
  // Assuming all X and Ys are the same, if X is not the same size as y, assume
  // it is a histogram
  if (inputWorkspace.x(0).size() != inputWorkspace.y(0).size()) {
    // The only way to guarantee that a histogram is a proper histogram, is to
    // check whether each data value is in the correct order.
    if (!isItSorted(std::greater<double>(), inputWorkspace)) {
      if (!isItSorted(std::less<double>(), inputWorkspace)) {
        throw std::runtime_error(
            "The data entered contains an invalid histogram: histogram has an "
            "unordered x-axis.");
      }
    }
    return true;
  }
  return false;
}

} // namespace Algorithms
} // namespace Mantid
