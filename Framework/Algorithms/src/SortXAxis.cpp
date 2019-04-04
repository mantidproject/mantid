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

  const auto orderingValues =
      std::vector<std::string>({"Ascending", "Descending"});
  const auto orderingValidator =
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

std::map<std::string, std::string> SortXAxis::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  const bool ignoreHistogramValidation =
      getProperty("IgnoreHistogramValidation");

  const auto ySize = inputWorkspace->y(0).size();
  const auto xSize = inputWorkspace->x(0).size();

  if (ySize != xSize && ySize != xSize - 1)
    errors["InputWorkspace"] =
        "The workspace provided is not a point data or histogram workspace.";

  if (!ignoreHistogramValidation && ySize == xSize - 1) {
    const auto message = determineIfHistogramIsValid(inputWorkspace);
    if (!message.empty())
      errors["InputWorkspace"] = message;
  }

  return errors;
}

void SortXAxis::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = inputWorkspace->clone();

  // Define everything you can outside of the for loop
  // Assume that all spec are the same size
  const auto sizeOfX = inputWorkspace->x(0).size();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int specNum = 0u; specNum < (int)inputWorkspace->getNumberHistograms();
       specNum++) {
    PARALLEL_START_INTERUPT_REGION
    auto workspaceIndices = createIndices(sizeOfX);

    sortIndicesByX(workspaceIndices, getProperty("Ordering"), *inputWorkspace,
                   specNum);

    copyToOutputWorkspace(workspaceIndices, *inputWorkspace, *outputWorkspace,
                          specNum);
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
std::vector<std::size_t> SortXAxis::createIndices(const size_t sizeOfX) {
  std::vector<std::size_t> workspaceIndices;
  workspaceIndices.reserve(sizeOfX);
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
    workspaceIndices.emplace_back(workspaceIndex);
  }
  return workspaceIndices;
}

/**
 * @brief A template for sorting the values given a comparator
 *
 * @tparam Comparator
 * @param workspaceIndices the vector of indicies values
 * @param inputWorkspace the original workspace
 * @param specNum the Spectrum number to be sorted
 * @param compare std::less<double> for Ascending order std::greater<double>
 * for descending order
 */
template <typename Comparator>
void sortByXValue(std::vector<std::size_t> &workspaceIndices,
                  const Mantid::API::MatrixWorkspace &inputWorkspace,
                  unsigned int specNum, Comparator const &compare) {
  std::sort(workspaceIndices.begin(), workspaceIndices.end(),
            [&](std::size_t lhs, std::size_t rhs) -> bool {
              return compare(inputWorkspace.x(specNum)[lhs],
                             inputWorkspace.x(specNum)[rhs]);
            });
}

void SortXAxis::sortIndicesByX(
    std::vector<std::size_t> &workspaceIndices, std::string order,
    const Mantid::API::MatrixWorkspace &inputWorkspace, unsigned int specNum) {
  if (order == "Ascending") {
    sortByXValue(workspaceIndices, inputWorkspace, specNum,
                 std::less<double>());
  } else if (order == "Descending") {
    sortByXValue(workspaceIndices, inputWorkspace, specNum,
                 std::greater<double>());
  }
}

/**
 * @brief Copies the sorted inputworkspace into the output workspace without
 * using clone because of how histograms are supported, for the X Axis and the
 * Dx Axis.
 *
 * @param workspaceIndices the sorted vector of indecies
 * @param inputWorkspace the unsorted initial workspace
 * @param outputWorkspace the emptry output workspace
 * @param specNum the Spectrum it is currently copying over
 */
void SortXAxis::copyXandDxToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndices,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum) {
  // Move an ordered X to the output workspace
  for (auto workspaceIndex = 0u;
       workspaceIndex < inputWorkspace.x(specNum).size(); workspaceIndex++) {
    outputWorkspace.mutableX(specNum)[workspaceIndex] =
        inputWorkspace.x(specNum)[workspaceIndices[workspaceIndex]];
  }

  // If Dx's are present, move Dx's to the output workspace
  // If Dx's are present, move Dx's to the output workspace
  if (inputWorkspace.hasDx(specNum)) {
    for (auto workspaceIndex = 0u;
         workspaceIndex < inputWorkspace.dx(specNum).size(); workspaceIndex++) {
      outputWorkspace.mutableDx(specNum)[workspaceIndex] =
          inputWorkspace.dx(specNum)[workspaceIndices[workspaceIndex]];
    }
  }
}

/**
 * @brief Copies the sorted inputworkspace into the output workspace without
 * using clone because of how histograms are supported, for the Y Axis and the E
 * Axis.
 *
 * @param workspaceIndices the sorted vector of indicies
 * @param inputWorkspace the unsorted input workspaces
 * @param outputWorkspace the empty output workspace
 * @param specNum the spectrum number being copied into
 */
void SortXAxis::copyYandEToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndices,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum) {
  auto &inSpaceY = inputWorkspace.y(specNum);
  const auto ySize = inSpaceY.size();

  // If the input workspace is a histogram, remove the largest index
  if (ySize == workspaceIndices.size() - 1) {
    const auto iter =
        std::find(workspaceIndices.begin(), workspaceIndices.end(), ySize);
    if (iter != workspaceIndices.end())
      workspaceIndices.erase(iter);
  }

  for (auto workspaceIndex = 0u; workspaceIndex < ySize; workspaceIndex++) {
    outputWorkspace.mutableY(specNum)[workspaceIndex] =
        inSpaceY[workspaceIndices[workspaceIndex]];
  }

  auto &inSpaceE = inputWorkspace.e(specNum);
  for (auto workspaceIndex = 0u;
       workspaceIndex < inputWorkspace.e(specNum).size(); workspaceIndex++) {
    outputWorkspace.mutableE(specNum)[workspaceIndex] =
        inSpaceE[workspaceIndices[workspaceIndex]];
  }
}

void SortXAxis::copyToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndices,
    const Mantid::API::MatrixWorkspace &inputWorkspace,
    Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum) {
  copyXandDxToOutputWorkspace(workspaceIndices, inputWorkspace, outputWorkspace,
                              specNum);
  copyYandEToOutputWorkspace(workspaceIndices, inputWorkspace, outputWorkspace,
                             specNum);
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
bool SortXAxis::isItSorted(Comparator const &compare,
                           MatrixWorkspace_const_sptr inputWorkspace) const {
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
 * @brief Determines whether it is a valid histogram or not. Assumes that the y
 * and x spectra are the same size
 *
 * @param inputWorkspace the unsorted input workspace
 * @returns an error message if the data is unordered
 */
std::string SortXAxis::determineIfHistogramIsValid(
    MatrixWorkspace_const_sptr inputWorkspace) const {
  // The only way to guarantee that a histogram is a proper histogram, is to
  // check whether each data value is in the correct order.
  if (!isItSorted(std::greater<double>(), inputWorkspace) &&
      !isItSorted(std::less<double>(), inputWorkspace))
    return "The data entered contains an invalid histogram: histogram has "
           "an unordered x-axis.";
}

} // namespace Algorithms
} // namespace Mantid
