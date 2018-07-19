#include "MantidAlgorithms/SortXAxis2.h"

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

std::vector<std::size_t> SortXAxis::createIndexes(const size_t sizeOfX) {
  std::vector<std::size_t> workspaceIndicies;
  workspaceIndicies.reserve(sizeOfX);
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
    workspaceIndicies.emplace_back(workspaceIndex);
  }
  return workspaceIndicies;
}

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
  if (inputWorkspace->hasDx(specNum)) {
    for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
      outputWorkspace->mutableDx(specNum)[workspaceIndex] =
          inputWorkspace->dx(specNum)[workspaceIndicies[workspaceIndex]];
    }
  }
}

void SortXAxis::copyYandEToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, const size_t sizeOfY,
    unsigned int specNum) {
  // If Histogram data find the biggest index value and remove it from
  // workspaceIndicies
  if (inputWorkspace->isHistogramData()) {
    auto lastIndexIt =
        std::find(workspaceIndicies.begin(), workspaceIndicies.end(), sizeOfY);
    workspaceIndicies.erase(lastIndexIt);
  }

  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfY; workspaceIndex++) {
    outputWorkspace->mutableY(specNum)[workspaceIndex] =
        inputWorkspace->y(specNum)[workspaceIndicies[workspaceIndex]];
    outputWorkspace->mutableE(specNum)[workspaceIndex] =
        inputWorkspace->e(specNum)[workspaceIndicies[workspaceIndex]];
  }
}

void SortXAxis::copyToOutputWorkspace(
    std::vector<std::size_t> &workspaceIndicies,
    MatrixWorkspace_const_sptr inputWorkspace,
    MatrixWorkspace_sptr outputWorkspace, const size_t sizeOfX,
    const size_t sizeOfY, unsigned int specNum) {
  copyXandDxToOutputWorkspace(workspaceIndicies, inputWorkspace,
                              outputWorkspace, sizeOfX, specNum);
  copyYandEToOutputWorkspace(workspaceIndicies, inputWorkspace, outputWorkspace,
                             sizeOfY, specNum);
}
} // namespace Algorithms
} // namespace Mantid
