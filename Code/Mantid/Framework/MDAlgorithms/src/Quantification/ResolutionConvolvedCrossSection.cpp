//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/ExperimentInfo.h"

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

    /// Function you want to fit to.
    /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
    void ResolutionConvolvedCrossSection::function(const API::FunctionDomain& domain, API::FunctionValues& values)const
    {
      const API::FunctionDomainMD* mdDomain = dynamic_cast<const API::FunctionDomainMD*>(&domain);
      if(!mdDomain)
      {
        throw std::invalid_argument("ResolutionConvolvedCrossSection can only be used with MD domains");
      }

      m_workspace = boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(mdDomain->getWorkspace());
      if(!m_workspace)
      {
        throw std::invalid_argument("ResolutionConvolvedCrossSection can only be used with MD event workspaces");
      }
      IFunctionMD::evaluateFunction(*mdDomain, values); // Calls functionMD repeatedly
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
      if(!m_convolution)
      {
        throw std::runtime_error("ResolutionConvolvedCrossSection::functionMD - Setup incomplete, no convolution type has been created");
      }

      double signal(0.0);
      const size_t numEvents = box.getNumEvents();
      // loop over each MDPoint in current MDBox
      for(size_t j = 0; j < numEvents; j++)
      {
        uint16_t innerRunIndex = box.getInnerRunIndex(j);
        API::ExperimentInfo_const_sptr exptInfo = m_workspace->getExperimentInfo(innerRunIndex);
        signal += m_convolution->signal(box, j, exptInfo);
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
      const std::vector<std::string> names = m_convolution->getAttributeNames();
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

    }

  }
}
