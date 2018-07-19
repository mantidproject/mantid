#include "MantidAlgorithms/SortXAxis2.h"
#include <iostream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(SortXAxis)

const std::string SortXAxis::name() const { return "SortXAxis"; }

int SortXAxis::version() const { return 2; }

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
}

void SortXAxis::exec() {

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = inputWorkspace->clone();

  // Check if it is a valid histogram here
  bool isAProperHistogram = determineIfHistogramIsValid(inputWorkspace);

  // Define everything you can outside of the for loop
  // Assume that all spec are the same size
  const auto sizeOfX = inputWorkspace->x(0).size();
  const auto sizeOfY = inputWorkspace->y(0).size();

  std::string theOrder = getProperty("Ordering");

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int specNum = 0u; specNum < (int)inputWorkspace->getNumberHistograms();
       specNum++) {
    PARALLEL_START_INTERUPT_REGION
    auto workspaceIndicies = createIndexes(sizeOfX);

    sortIndicesByX(workspaceIndicies, theOrder, inputWorkspace, specNum);

    copyToOutputWorkspace(workspaceIndicies, inputWorkspace, outputWorkspace,
                          sizeOfX, sizeOfY, specNum, isAProperHistogram);
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
 * @param workspaceIndicies, the vector of indicies values
 * @param inputWorkspace, the original workspace
 * @param specNum the Spectrum number to be sorted
 * @param compare std::less<double> for Ascending order std::greater<double>
 * for descending order
 */
template <typename Comparator>
void sortByXValue(std::vector<std::size_t> &workspaceIndicies,
                  MatrixWorkspace_const_sptr inputWorkspace,
                  unsigned int specNum, Comparator const &compare) {
  std::sort(workspaceIndicies.begin(), workspaceIndicies.end(),
            [&](std::size_t lhs, std::size_t rhs) -> bool {
              return compare(inputWorkspace->x(specNum)[lhs],
                             inputWorkspace->x(specNum)[rhs]);
            });
}

void SortXAxis::sortIndicesByX(std::vector<std::size_t> &workspaceIndicies,
                               std::string order,
                               MatrixWorkspace_const_sptr inputWorkspace,
                               unsigned int specNum) {
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
 * @param sizeOfX the Maximum index of X
 * @param specNum the Spectrum it is currently copying over
 */
void SortXAxis::copyXandDxToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, const size_t sizeOfX,
    unsigned int specNum) {
  // Move an ordered X to the output workspace
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
    outputWorkspace->mutableX(specNum)[workspaceIndex] =
        inputWorkspace->x(specNum)[workspaceIndicies[workspaceIndex]];
  }

  // If Dx's are present, move Dx's to the output workspace
  // If Dx's are present, move Dx's to the output workspace
  if (inputWorkspace->hasDx(specNum)) {
    for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
      outputWorkspace->mutableDx(specNum)[workspaceIndex] =
          inputWorkspace->dx(specNum)[workspaceIndicies[workspaceIndex]];
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
 * @param sizeOfY the Maxiumum index of Y
 * @param specNum the spectrum number being copied into
 * @param isAProperHistogram whether or not it has been determined to be a valid
 * histogram earlier on.
 */
void SortXAxis::copyYandEToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, const size_t sizeOfY,
    unsigned int specNum, bool isAProperHistogram) {
  // If Histogram data find the biggest index value and remove it from
  // workspaceIndicies
  if (isAProperHistogram) {
    auto lastIndexIt =
        std::find(workspaceIndicies.begin(), workspaceIndicies.end(), sizeOfY);
    workspaceIndicies.erase(lastIndexIt);
  }

  auto &inSpaceY = inputWorkspace->y(specNum);
  auto &inSpaceE = inputWorkspace->e(specNum);
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfY; workspaceIndex++) {
    outputWorkspace->mutableY(specNum)[workspaceIndex] =
        inSpaceY[workspaceIndicies[workspaceIndex]];
    outputWorkspace->mutableE(specNum)[workspaceIndex] =
        inSpaceE[workspaceIndicies[workspaceIndex]];
  }
}

void SortXAxis::copyToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, const size_t sizeOfX,
    const size_t sizeOfY, unsigned int specNum, bool isAProperHistogram) {
  copyXandDxToOutputWorkspace(workspaceIndicies, inputWorkspace,
                              outputWorkspace, sizeOfX, specNum);
  copyYandEToOutputWorkspace(workspaceIndicies, inputWorkspace, outputWorkspace,
                             sizeOfY, specNum, isAProperHistogram);
}

/**
 * @brief determines whether or not a given spectrum is sorted based on a passed
 * comparator
 *
 * @tparam Comparator
 * @param compare std::less<double> for descending, and std::greater<double> for
 * ascending.
 * @param inputWorkspace the unsorted input workspace
 * @param specNum the spectrum number currently being compared
 * @return true if it is sorted
 * @return false if it is not sorted
 */
template <typename Comparator>
bool isItSorted(Comparator const &compare,
                MatrixWorkspace_const_sptr inputWorkspace) {
  for (auto specNum = 0u; specNum < inputWorkspace->getNumberHistograms();
       specNum++) {
    if (!std::is_sorted(inputWorkspace->x(specNum).begin(),
                        inputWorkspace->x(specNum).end(),
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
    MatrixWorkspace_const_sptr inputWorkspace) {
  // Assuming all X and Ys are the same, if X is not the same size as y, assume
  // it is a histogram
  if (inputWorkspace->x(0).size() != inputWorkspace->y(0).size()) {
    // The only way to guarantee that a histogram is a proper histogram, is to
    // check whether each data value is in the correct order.
    if (!isItSorted(std::greater<double>(), inputWorkspace)) {
      if (!isItSorted(std::less<double>(), inputWorkspace)) {
        throw std::runtime_error("Data entered looks like a histogram, but is "
                                 "not a valid histogram");
      }
    }
    return true;
  }
  return false;
}

} // namespace Algorithms
} // namespace Mantid