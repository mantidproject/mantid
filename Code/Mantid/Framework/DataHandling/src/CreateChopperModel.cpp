#include "MantidDataHandling/CreateChopperModel.h"
#include "MantidAPI/FermiChopperModel.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateChopperModel);

using Kernel::Direction;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using Kernel::BoundedValidator;
using Kernel::Direction;
using Kernel::ListValidator;
using Kernel::MandatoryValidator;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateChopperModel::name() const {
  return "CreateChopperModel";
};

/// Algorithm's version for identification. @see Algorithm::version
int CreateChopperModel::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateChopperModel::category() const {
  return "DataHandling";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateChopperModel::init() {
  declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
                  "An workspace to attach the model");

  std::vector<std::string> keys(1, "FermiChopperModel");
  declareProperty("ModelType", "",
                  boost::make_shared<ListValidator<std::string>>(keys),
                  "The string identifier for the model", Direction::Input);

  declareProperty("Parameters", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The parameters for the model as comma-separated list of "
                  "name=value pairs");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("ChopperPoint", 0, mustBePositive,
                  "The index of the chopper point. (Default=0)");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void CreateChopperModel::exec() {
  const std::string modelType = getProperty("ModelType");
  if (modelType != "FermiChopperModel") {
    throw std::invalid_argument("Invalid chopper model type.");
  }
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  API::ChopperModel *chopper = new API::FermiChopperModel;
  chopper->setRun(workspace->run());
  chopper->initialize(getProperty("Parameters"));

  int index = getProperty("ChopperPoint");
  workspace->setChopperModel(chopper, static_cast<size_t>(index));
}

} // namespace DataHandling
} // namespace Mantid
