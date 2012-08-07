/*WIKI*
Creates a model for a chopper using the given parameters. The parameters are given as a string to allow flexibility for each chopper model having different parameterisation.

The chopper point is an index that can be used for multi-chopper instruments. The indices start from zero, with this being closest to moderator.

Available models with parameter names:
* FermiChopper -
** AngularVelocity - The angular velocity value or log name
** ChopperRadius - The radius, in metres, of the whole chopper
** SlitThickness - The thickness, in metres, of the slit
** SlitRadius - The radius of curvature, in metres, of the slit
** Ei - The Ei for this run as a value or log name
*WIKI*/

#include "MantidDataHandling/CreateChopperModel.h"
#include "MantidAPI/FermiChopperModel.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid
{
  namespace DataHandling
  {
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
    const std::string CreateChopperModel::name() const { return "CreateChopperModel";};

    /// Algorithm's version for identification. @see Algorithm::version
    int CreateChopperModel::version() const { return 1;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string CreateChopperModel::category() const { return "DataHandling";}

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void CreateChopperModel::initDocs()
    {
      this->setWikiSummary("Creates a chopper model for a given workspace");
      this->setOptionalMessage("Creates a chopper model for a given workspace");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void CreateChopperModel::init()
    {
      declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut), "An workspace to attach the model");
      
      std::vector<std::string> keys(1, "FermiChopperModel");
      declareProperty("ModelType", "", boost::make_shared<ListValidator<std::string>>(keys),
                      "The string identifier for the model", Direction::Input);
      
      declareProperty("Parameters", "", boost::make_shared<MandatoryValidator<std::string>>(),
                      "The parameters for the model as comma-separated list of name=value pairs");
      
      auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty("ChopperPoint", 0, mustBePositive, "The index of the chopper point. (Default=0)");
    }

    //----------------------------------------------------------------------------------------------
    /**
     * Execute the algorithm.
     */
    void CreateChopperModel::exec()
    {
      const std::string modelType = getProperty("ModelType");
      if(modelType != "FermiChopperModel")
      {
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
