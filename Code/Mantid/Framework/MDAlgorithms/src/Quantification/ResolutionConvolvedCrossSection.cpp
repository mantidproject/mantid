//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/CPUTimer.h"

/// Parallel region start macro. Different to generic one as that is specific to algorithms
/// Assumes boolean exceptionThrow has been declared
#define OMP_FOR_NO_CHECK \
  PRAGMA(omp parallel for)

#define OMP_START_INTERRUPT \
    if (!exceptionThrown && !this->cancellationRequestReceived()) \
    { \
      try \
      {

/// Parallel region end macro. Different to generic one as that is specific to algorithms
#define OMP_END_INTERRUPT \
      } \
      catch(std::exception &ex) \
      { \
        if (!exceptionThrown) \
        { \
          exceptionThrown = true; \
          g_log.error() << "ResolutionConvolvedCrossSection: " << ex.what() << "\n"; \
        } \
      } \
      catch(...) \
      { \
        exceptionThrown = true; \
      } \
    }

/// Check for exceptions in parallel region
#define OMP_INTERRUPT_CHECK \
  if (exceptionThrown) \
  { \
    g_log.debug("Exception thrown in parallel region"); \
    throw std::runtime_error("ResolutionConvolvedCrossSection: error (see log)"); \
  } \
  if(this->cancellationRequestReceived())\
  {\
    reportProgress();\
  }

namespace Mantid
{
  namespace MDAlgorithms
  {
    DECLARE_FUNCTION(ResolutionConvolvedCrossSection);

    namespace
    {
      // Attribute names
      const char * RESOLUTION_ATTR = "ResolutionFunction";
      const char * FOREGROUND_ATTR = "ForegroundModel";
    }

    Kernel::Logger & ResolutionConvolvedCrossSection::g_log = Kernel::Logger::get("ResolutionConvolvedCrossSection");

    /**
     * Constructor
     */
    ResolutionConvolvedCrossSection::ResolutionConvolvedCrossSection()
      : ParamFunction(), IFunctionMD(), m_convolution(NULL), m_workspace(),
        m_nthreads(API::FrameworkManager::Instance().getNumOMPThreads())
    {
    }

    /**
     * Destructor
     */
    ResolutionConvolvedCrossSection::~ResolutionConvolvedCrossSection()
    {
      delete m_convolution;
    }

    /**
     * Declare the attributes associated with this function
     */
    void ResolutionConvolvedCrossSection::declareAttributes()
    {
      declareAttribute(RESOLUTION_ATTR, IFunction::Attribute(""));
      declareAttribute(FOREGROUND_ATTR, IFunction::Attribute(""));
    }

    /**
     * Declares any model parameters.
     */
    void ResolutionConvolvedCrossSection::declareParameters()
    {
    }

    /**
     *  Set a value to a named attribute
     *  @param name :: The name of the attribute
     *  @param value :: The value of the attribute
     */
    void ResolutionConvolvedCrossSection::setAttribute(const std::string& name,
                                                       const API::IFunction::Attribute & value)
    {
      storeAttributeValue(name, value);

      const std::string fgModelName = getAttribute(FOREGROUND_ATTR).asString();
      const std::string convolutionType = getAttribute(RESOLUTION_ATTR).asString();
      if(!convolutionType.empty() && !fgModelName.empty())
      {
        setupResolutionFunction(convolutionType, fgModelName);
      }
      if(name != FOREGROUND_ATTR && name != RESOLUTION_ATTR)
      {
        m_convolution->setAttribute(name, value);
      }
    }

    /**
     * Override the call to set the workspace here to store it so we can access it throughout the evaluation
     * @param workspace :: A workspace containing the instrument definition
     */
    void ResolutionConvolvedCrossSection::setWorkspace(boost::shared_ptr<const API::Workspace> workspace)
    {
      assert(m_convolution);

      m_workspace = boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(workspace);
      if(!m_workspace)
      {
        throw std::invalid_argument("ResolutionConvolvedCrossSection can only be used with MD event workspaces");
      }
      IFunctionMD::setWorkspace(workspace);
      m_convolution->preprocess(m_workspace);
    }

    /**
     * Returns an estimate of the number of progress reports a single evaluation of the function will have.
     * @return
     */
    int64_t ResolutionConvolvedCrossSection::estimateNoProgressCalls() const
    {
      int64_t ncalls(1);
      if(m_workspace)
      {
        ncalls = static_cast<int64_t>(m_workspace->getNPoints());
      }
      return ncalls;
    }


    void ResolutionConvolvedCrossSection::function(const API::FunctionDomain& domain, API::FunctionValues& values) const
    {
      const API::FunctionDomainMD* domainMD = dynamic_cast<const API::FunctionDomainMD*>(&domain);
      if (!domainMD)
      {
        throw std::invalid_argument("Expected FunctionDomainMD in ResolutionConvolvedCrossSection");
      }

      std::vector<API::IMDIterator*> iterators = m_workspace->createIterators(m_nthreads);
      const int nthreads = static_cast<int>(iterators.size());
      std::vector<size_t> resultOffsets(nthreads, 0);
      // Each calculated result needs to be at the correct index in the according to its
      // position. The order here is the same that would be if it were computed serially.
      // The offsets vector sets the size of the offset into the calculated results for each
      // thread
      if(nthreads > 1 )
      {
        // The first result will have offset of 0 so skip it
        size_t totalBoxes = iterators[0]->getDataSize();
        for(int i = 1; i < nthreads; ++i)
        {
          // The offset is simply the total number of boxes before it
          resultOffsets[i] = totalBoxes;
          totalBoxes += iterators[i]->getDataSize();
        }
      }

      bool exceptionThrown = false;
      OMP_FOR_NO_CHECK
      for(int i = 0; i < nthreads; ++i)
      {
        API::IMDIterator *boxIterator = iterators[i];
        const size_t resultsOffset = resultOffsets[i];

        size_t boxIndex(0);
        do
        {
          OMP_START_INTERRUPT

          const double avgSignal = functionMD(*boxIterator);
          const size_t resultIndex = resultsOffset + boxIndex;
          PARALLEL_CRITICAL(ResolutionConvolvedCrossSection_function)
          {
            values.setCalculated(resultIndex, avgSignal);
          }
          ++boxIndex;

          OMP_END_INTERRUPT
        }
        while(boxIterator->next());
      }
      OMP_INTERRUPT_CHECK

      for(auto it = iterators.begin(); it != iterators.end(); ++it)
      {
        API::IMDIterator *boxIterator = *it;
        delete boxIterator;
      }
    }

    /**
     * Returns the value of the function for the given MD box. For each MDPoint
     * within the box the convolution of the current model with the resolution
     * is computed at summed. The average of this over the number of events
     * is returned
     * @param box An iterator pointing at the MDBox being looked at
     * @return The current value of the function
     */
    double ResolutionConvolvedCrossSection::functionMD(const Mantid::API::IMDIterator& box) const
    {
      assert(m_convolution);
      const size_t numEvents = box.getNumEvents();
      if(numEvents == 0) return 0.0;

      double signal(0.0);
      for(size_t j = 0; j < numEvents; ++j)
      {
        double contribution =  m_convolution->signal(box, box.getInnerRunIndex(j), j);
        signal += contribution;
        reportProgress();
      }

      // Return the mean
      return signal/static_cast<double>(numEvents);
    }

    /**
     * Set a pointer to the concrete convolution object from a named implementation.
     * @param name :: The name of a convolution type
     * @param fgModelName :: The foreground model name selected
     */
    void ResolutionConvolvedCrossSection::setupResolutionFunction(const std::string & name, const std::string & fgModelName)
    {
      if(m_convolution) return; // Done

      m_convolution = MDResolutionConvolutionFactory::Instance().createConvolution(name, fgModelName,*this);
      // Pass on the attributes
      auto names = m_convolution->getAttributeNames();
      for(auto iter = names.begin(); iter != names.end(); ++iter)
      {
        this->declareAttribute(*iter, m_convolution->getAttribute(*iter));
      }
      // Pull the foreground parameters on to here
      const ForegroundModel & fgModel = m_convolution->foregroundModel();
      const size_t nparams = fgModel.nParams();
      for(size_t i = 0; i < nparams; ++i)
      {
        this->declareParameter(fgModel.parameterName(i), fgModel.getInitialParameterValue(i), fgModel.parameterDescription(i));
      }
      // Pull the foreground attributes on to here
      names = fgModel.getAttributeNames();
      for(auto iter = names.begin(); iter != names.end(); ++iter)
      {
        this->declareAttribute(*iter, fgModel.getAttribute(*iter));
      }
    }

  }
}
