#include "MantidAlgorithms/SortXAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortXAxis)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SortXAxis::name() const { return "SortXAxis"; }

/// Algorithm's version for identification. @see Algorithm::version
int SortXAxis::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SortXAxis::category() const { return "Transforms\\Axes;Utility\\Sorting"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SortXAxis::summary() const {
  return "Clones the input MatrixWorkspace(s) and orders the x-axis in an ascending fashion.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 *         self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input),
                             doc="Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output),
                             doc="Sorted Output Workspace")
        self.declareProperty("Ordering",
                             defaultValue="Ascending",
                             validator=StringListValidator(["Ascending", "Descending"]),
                             direction=Direction.Input,
                             doc="Ascending or descending sorting")
 */
void SortXAxis::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input Workspace");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Sorted Output Workspace");

  auto orderingValues = std::vector<std::string>({"Ascending",
                                              "Descending"});
  auto orderingValidator =
      boost::make_shared<StringListValidator>(orderingValues);
  declareProperty("Ordering", orderingValues[0], orderingValidator,
                  "Ascending or descending sorting",
                  Direction::Input);            
}                  
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SortXAxis::exec() {
  //bool dryrun = getProperty("DryRun");
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = inputWorkspace->clone();

  for(auto i = 0u; i<inputWorkspace->getNumberHistograms(); i++){
    std::vector<std::size_t> indexes;
    const auto sizeOf = inputWorkspace->dataX(i).size();

    indexes.reserve(sizeOf);

    for(auto workspaceIndex = 0u; workspaceIndex<sizeOf; workspaceIndex++){
      indexes.emplace_back(workspaceIndex);
    }
    auto ascending = [&](std::size_t lhs, std::size_t rhs) -> bool {
      return inputWorkspace->dataX(i)[lhs] < inputWorkspace->dataX(i)[rhs];
    };

    auto descending = [&](std::size_t lhs, std::size_t rhs) -> bool {
      return inputWorkspace->dataX(i)[lhs] > inputWorkspace->dataX(i)[rhs];
    };

    auto ordering = std::string(getProperty("Ordering"));
    if(ordering == "Ascending"){
      std::sort(indexes.begin(), indexes.end(), ascending);
    } else if(ordering == "Descending"){
      std::sort(indexes.begin(), indexes.end(), descending);
    }
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
