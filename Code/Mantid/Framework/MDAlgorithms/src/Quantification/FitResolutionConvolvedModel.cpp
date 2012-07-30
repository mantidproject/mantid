/*WIKI*
  Fits a dataset using a resolution function convolved with a foreground model
 *WIKI*/

#include "MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h"

#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(FitResolutionConvolvedModel);

    using Kernel::Direction;
    using Kernel::ListValidator;
    using Kernel::MandatoryValidator;
    using API::WorkspaceProperty;
    using API::IMDEventWorkspace;
    using API::IMDEventWorkspace_sptr;

    namespace
    {
      // Property names
      const char * INPUT_WS_NAME = "InputWorkspace";
      const char * RESOLUTION_NAME = "ResolutionFunction";
      const char * FOREGROUND_NAME = "ForegroundModel";
      const char * PARS_NAME = "Parameters";
      const char * MAX_ITER_NAME = "MaxIterations";
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string FitResolutionConvolvedModel::name() const { return "FitResolutionConvolvedModel"; }

    /// Algorithm's version for identification. @see Algorithm::version
    int FitResolutionConvolvedModel::version() const { return 1; }

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string FitResolutionConvolvedModel::category() const { return "Quantification"; }

    /// Sets documentation strings for this algorithm
    void FitResolutionConvolvedModel::initDocs()
    {
      this->setWikiSummary("Fits a cuts/slices from an MDEventWorkspace using a resolution function convolved with a foreground model");
      this->setOptionalMessage("Fits a cuts/slices from an MDEventWorkspace using a resolution function convolved with a foreground model");
    }

    //----------------------------------------------------------------------------------------------

    /// Returns the number of iterations that should be performed
    int FitResolutionConvolvedModel::niterations() const
    {
      int maxIter = getProperty(MAX_ITER_NAME);
      return maxIter;
    }

    /// Returns the name of the max iterations property
    std::string FitResolutionConvolvedModel::maxIterationsPropertyName() const
    {
      return MAX_ITER_NAME;
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void FitResolutionConvolvedModel::init()
    {
      declareProperty(new WorkspaceProperty<IMDEventWorkspace>(INPUT_WS_NAME,"",Direction::Input),
                      "The input MDEvent workspace");

      std::vector<std::string> models = MDResolutionConvolutionFactory::Instance().getKeys();
      declareProperty(RESOLUTION_NAME, "", boost::make_shared<ListValidator<std::string>>(models),
                      "The name of a resolution model", Direction::Input);

      models = ForegroundModelFactory::Instance().getKeys();
      declareProperty(FOREGROUND_NAME, "", boost::make_shared<ListValidator<std::string>>(models),
                      "The name of a foreground function", Direction::Input);

      declareProperty(MAX_ITER_NAME, 20, "The maximum number of iterations to perform for the fitting",
                      Direction::Input);

      declareProperty(PARS_NAME, "", boost::make_shared<MandatoryValidator<std::string>>(),
                      "The parameters/attributes for the function & model. See Fit documentation for format",
                      Direction::Input);
    }

    //----------------------------------------------------------------------------------------------
    /**
     * Execute the algorithm.
     */
    void FitResolutionConvolvedModel::exec()
    {
      API::IAlgorithm_sptr fit = createFittingAlgorithm();
      fit->setPropertyValue("Function", createFunctionString());
      fit->setProperty("InputWorkspace", getPropertyValue(INPUT_WS_NAME));

      // Maximum number of allowed iterations
      const int maxIter = niterations();
      fit->setProperty("MaxIterations", maxIter);

      try
      {
        fit->execute();
      }
      catch(std::exception & exc)
      {
        throw std::runtime_error(std::string("FitResolutionConvolvedModel - Error running Fit: ") + exc.what());
      }
    }

    /**
     * Create the fitting sub algorithm
     * @return A shared pointer to the new algorithm
     */
    API::IAlgorithm_sptr FitResolutionConvolvedModel::createFittingAlgorithm()
    {
      const double startProgress(0.0), endProgress(1.0);
      const bool enableLogging(true);
      return createSubAlgorithm("Fit", startProgress, endProgress, enableLogging);
    }

    /**
     * Create the function string required by fit
     * @return Creates a string that can be passed to fit containing the necessary setup for the function
     */
    std::string FitResolutionConvolvedModel::createFunctionString() const
    {
      std::ostringstream stringBuilder;

      const char seperator(',');
      stringBuilder << "name=" << ResolutionConvolvedCrossSection().name() << seperator
                    << "ResolutionFunction=" << this->getPropertyValue(RESOLUTION_NAME) << seperator
                    << "ForegroundModel=" << this->getPropertyValue(FOREGROUND_NAME) << seperator
                    << this->getPropertyValue("Parameters");
      return stringBuilder.str();
    }


  } // namespace MDAlgorithms
} // namespace Mantid
