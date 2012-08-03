//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/CPUTimer.h"

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
      : ParamFunctionAttributeHolder(), IFunctionMD(), m_convolution(NULL), m_workspace()
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
     * @param workspace
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
      m_convolution->useNumberOfThreads(PARALLEL_GET_MAX_THREADS);
      m_convolution->preprocess(m_workspace);
    }

    void ResolutionConvolvedCrossSection::function(const API::FunctionDomain& domain, API::FunctionValues& values) const
    {
      const API::FunctionDomainMD* dmd = dynamic_cast<const API::FunctionDomainMD*>(&domain);
      if (!dmd)
      {
        throw std::invalid_argument("Unexpected domain in IFunctionMD");
      }
      dmd->reset();
      size_t i = 0;
      for(const API::IMDIterator* r = dmd->getNextIterator(); r != NULL; r = dmd->getNextIterator())
      {
        values.setCalculated(i,functionMD(*r));
        i++;
      };
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
      const int64_t numEvents = static_cast<int64_t>(box.getNumEvents());
      if(numEvents == 0) return 0.0;

      double signal(0.0);
      PARALLEL_FOR_NO_WSP_CHECK()
      for(int64_t j = 0; j < numEvents; ++j)
      {
        const int64_t loopIndex = static_cast<int64_t>(j);
        double contribution =  m_convolution->signal(box, box.getInnerRunIndex(loopIndex), loopIndex);
        PARALLEL_ATOMIC
        signal += contribution;
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
