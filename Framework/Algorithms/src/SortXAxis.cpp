#include "MantidAlgorithms/SortXAxis.h"

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
         "ascending fashion.";
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

  for (auto specNum = 0u; specNum < inputWorkspace->getNumberHistograms();
       specNum++) {

    std::vector<std::size_t> indexes;
    const auto sizeOfX = inputWorkspace->x(specNum).size();
    const auto sizeOfY = inputWorkspace->y(specNum).size();

    indexes.reserve(sizeOfX);
    createIndexes(indexes, sizeOfX);

    // Order the algorithm given the property
    auto ordering = std::string(getProperty("Ordering"));
    if (ordering == "Ascending") {
      orderIndexesAscending(indexes, inputWorkspace, specNum);
    } else if (ordering == "Descending") {
      orderIndexesDecending(indexes, inputWorkspace, specNum);
    }

    // Move an ordered X to the output workspace
    for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
      outputWorkspace->mutableX(specNum)[workspaceIndex] =
          inputWorkspace->x(specNum)[indexes[workspaceIndex]];
    }

    // If Dx's are present, move Dx's to the output workspace
    if (inputWorkspace->hasDx(specNum)) {
      for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX;
           workspaceIndex++) {
        outputWorkspace->mutableDx(specNum)[workspaceIndex] =
            inputWorkspace->dx(specNum)[indexes[workspaceIndex]];
      }
    }

    // If Histogram data find the biggest index value and remove it from indexes
    if (inputWorkspace->isHistogramData()) {
      auto i = std::find(indexes.cbegin(), indexes.cend(), sizeOfY);
      indexes.erase(i);
    }

    for (auto workspaceIndex = 0u; workspaceIndex < sizeOfY; workspaceIndex++) {
      outputWorkspace->mutableY(specNum)[workspaceIndex] =
          inputWorkspace->y(specNum)[indexes[workspaceIndex]];
      outputWorkspace->mutableE(specNum)[workspaceIndex] =
          inputWorkspace->e(specNum)[indexes[workspaceIndex]];
    }
  }
  setProperty("OutputWorkspace", outputWorkspace);
}

void createIndexes(std::vector<std::size_t> &indexes, const size_t sizeOfX) {
  for (auto workspaceIndex = 0u; workspaceIndex < sizeOfX; workspaceIndex++) {
    indexes.emplace_back(workspaceIndex);
  }
}

void orderIndexesAscending(std::vector<std::size_t> &indexes,
                           MatrixWorkspace_const_sptr inputWorkspace,
                           unsigned int specNum) {
  std::sort(indexes.begin(), indexes.end(),
            [&](std::size_t lhs, std::size_t rhs) -> bool {
              return inputWorkspace->x(specNum)[lhs] <
                     inputWorkspace->x(specNum)[rhs];
            });
}

void orderIndexesDecending(std::vector<std::size_t> &indexes,
                           MatrixWorkspace_const_sptr inputWorkspace,
                           unsigned int specNum) {
  std::sort(indexes.begin(), indexes.end(),
            [&](std::size_t lhs, std::size_t rhs) -> bool {
              return inputWorkspace->x(specNum)[lhs] >
                     inputWorkspace->x(specNum)[rhs];
            });
}
} // namespace Algorithms
} // namespace Mantid
