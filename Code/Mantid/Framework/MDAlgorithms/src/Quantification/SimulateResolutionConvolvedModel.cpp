/*WIKI*
Runs a simulation of a model with a selected resolution function.

*WIKI*/

#include "MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SimulateResolutionConvolvedModel);

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string SimulateResolutionConvolvedModel::name() const { return "SimulateResolutionConvolvedModel";};

    /// Algorithm's version for identification. @see Algorithm::version
    int SimulateResolutionConvolvedModel::version() const { return 1;}

    /// Sets documentation strings for this algorithm
    void SimulateResolutionConvolvedModel::initDocs()
    {
      this->setWikiSummary("Runs a simulation of a model with a selected resolution function");
      this->setOptionalMessage("Runs a simulation of a model with a selected resolution function");
    }

    /**
     * Returns the number of iterations that should be performed
     * @returns 1 for the simulation
     */
    int SimulateResolutionConvolvedModel::niterations() const
    {
      return 1;
    }

    /**
     * Initialize the algorithm's properties.
     */
    void SimulateResolutionConvolvedModel::init()
    {
      FitResolutionConvolvedModel::init();
      // We don't want the iterations property
      const bool delProp(true);
      this->removeProperty(this->maxIterationsPropertyName(),delProp);
    }

  } // namespace MDAlgorithms
} // namespace Mantid
