/*WIKI*
Runs a simulation of a model with a selected resolution function.

*WIKI*/

#include "MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"
#include "MantidMDEvents/MDEventFactory.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SimulateResolutionConvolvedModel);

    using namespace API;
    using namespace Kernel;
    using Geometry::MDHistoDimensionBuilder;
    using Geometry::Vec_MDHistoDimensionBuilder;
    using MDEvents::MDEventWorkspace;
    using MDEvents::MDEvent;

    namespace
    {
      // Property names
      const char * INPUT_WS_NAME = "InputWorkspace";
      const char * SIMULATED_NAME = "OutputWorkspace";
      const char * RESOLUTION_NAME = "ResolutionFunction";
      const char * FOREGROUND_NAME = "ForegroundModel";
      const char * PARS_NAME = "Parameters";
    }


    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string SimulateResolutionConvolvedModel::name() const { return "SimulateResolutionConvolvedModel";}

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
      declareProperty(new WorkspaceProperty<IMDEventWorkspace>(INPUT_WS_NAME,"",Direction::Input),
                      "The input MDEvent workspace");

      declareProperty(new WorkspaceProperty<IMDEventWorkspace>(SIMULATED_NAME,"",Direction::Output),
                      "The simulated output workspace");

      std::vector<std::string> models = MDResolutionConvolutionFactory::Instance().getKeys();
      declareProperty(RESOLUTION_NAME, "", boost::make_shared<ListValidator<std::string>>(models),
                      "The name of a resolution model", Direction::Input);

      models = ForegroundModelFactory::Instance().getKeys();
      declareProperty(FOREGROUND_NAME, "", boost::make_shared<ListValidator<std::string>>(models),
                      "The name of a foreground function", Direction::Input);

      declareProperty(PARS_NAME, "", boost::make_shared<MandatoryValidator<std::string>>(),
                      "The parameters/attributes for the function & model. See Fit documentation for format",
                      Direction::Input);

    }

    /**
     * Executes the simulation
     */
    void SimulateResolutionConvolvedModel::exec()
    {
      m_inputWS = getProperty("InputWorkspace");
      auto resolution = createFunction();
      createDomains();

      // Do the real work
      resolution->function(*m_domain, *m_calculatedValues);

      IMDEventWorkspace_sptr outputWS = createOutputWorkspace();
      this->setProperty("OutputWorkspace", outputWS);
    }

    /**
     * @return The new MD function to be evaluated
     *
     */
    boost::shared_ptr<API::IFunction>
    SimulateResolutionConvolvedModel::createFunction() const
    {
      const std::string functionStr = this->createFunctionString();
      auto ifunction = FunctionFactory::Instance().createInitialized(functionStr);
      auto functionMD = boost::dynamic_pointer_cast<IFunctionMD>(ifunction);
      if(!functionMD)
      {
        throw std::invalid_argument("Function provided is not an MD function");
      }
      ifunction->setWorkspace(m_inputWS);
      return ifunction;
    }

    /**
     * Create the input & output domains from the input workspace
     */
    void SimulateResolutionConvolvedModel::createDomains()
    {
      auto iterator = m_inputWS->createIterator();
      const size_t dataSize = iterator->getDataSize();
      delete iterator;
      m_domain.reset(new FunctionDomainMD(m_inputWS, 0, dataSize));
      m_calculatedValues.reset(new FunctionValues(*m_domain));
    }

    /**
     *  Generate the output MD workspace that is a result of the simulation
     * @return The generated MD event workspace
     */
    API::IMDEventWorkspace_sptr
    SimulateResolutionConvolvedModel::createOutputWorkspace() const
    {
      static const size_t nd = 4;
      typedef MDEventWorkspace<MDEvent<nd>,nd> QOmegaWorkspace;

      auto outputWS = new QOmegaWorkspace;
      Vec_MDHistoDimensionBuilder dimensionVec(nd);
      MDHistoDimensionBuilder& qx = dimensionVec[0];
      MDHistoDimensionBuilder& qy = dimensionVec[1];
      MDHistoDimensionBuilder& qz = dimensionVec[2];
      MDHistoDimensionBuilder& en = dimensionVec[3];

      // Meta data
      qx.setId("qx");
      qx.setUnits("A^(-1)");
      qy.setId("qy");
      qy.setUnits("A^(-1)");
      qz.setId("qz");
      qz.setUnits("A^(-1)");
      en.setId("en");
      en.setUnits("MeV");

      // Bins extents
      for(size_t i = 0;i < nd; ++i)
      {
        boost::shared_ptr<const Geometry::IMDDimension> inputDim = m_inputWS->getDimension(i);
        dimensionVec[i].setName(inputDim->getName());
        dimensionVec[i].setNumBins(1);
        dimensionVec[i].setMin(inputDim->getMinimum());
        dimensionVec[i].setMax(inputDim->getMaximum());
      }
      //Add dimensions to the workspace by invoking the dimension builders.
      outputWS->addDimension(qx.create());
      outputWS->addDimension(qy.create());
      outputWS->addDimension(qz.create());
      outputWS->addDimension(en.create());

      // Run information
      outputWS->copyExperimentInfos(*m_inputWS);
      // Set sensible defaults for splitting behaviour
      BoxController_sptr bc = outputWS->getBoxController();
      bc->setSplitInto(3);
      bc->setSplitThreshold(3000);

      outputWS->initialize();

      auto inputIter = m_inputWS->createIterator();
      size_t boxCount(0);
      const float errorSq = 0.0;
      do
      {
        const size_t numEvents = inputIter->getNumEvents();
        const float signal = static_cast<float>(m_calculatedValues->getCalculated(boxCount));

        for(size_t i = 0; i != numEvents; ++i)
        {
          
          coord_t centers[4] = { inputIter->getInnerPosition(i,0), inputIter->getInnerPosition(i,1), 
                                 inputIter->getInnerPosition(i,4), inputIter->getInnerPosition(i,3) };
          outputWS->addEvent(MDEvent<4>(signal, errorSq, 
                                        inputIter->getInnerRunIndex(i), 
                                        inputIter->getInnerDetectorID(i),
                                        centers));
          
          
        }
        ++boxCount;
      }
      while(inputIter->next());

      delete inputIter;
      API::MemoryManager::Instance().releaseFreeMemory();
      // This splits up all the boxes according to split thresholds and sizes.
      auto threadScheduler = new Kernel::ThreadSchedulerFIFO();
      Kernel::ThreadPool threadPool(threadScheduler);
      outputWS->splitAllIfNeeded(threadScheduler);
      threadPool.joinAll();

      // Flush memory
      API::MemoryManager::Instance().releaseFreeMemory();

      return IMDEventWorkspace_sptr(outputWS);
    }

  } // namespace MDAlgorithms
} // namespace Mantid
