//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
     * Default constructor only callable by the factory
     */
    ForegroundModel::ForegroundModel()
      : API::ParamFunctionAttributeHolder(), m_fittingFunction(NULL), m_parOffset(0)
    {
    }

    /**
     * Constructor taking the fitted function to access the current parameter values
     * @param A reference to the fitting function
     */
    ForegroundModel::ForegroundModel(const API::IFunction & fittingFunction)
      : API::ParamFunctionAttributeHolder(), m_fittingFunction(NULL), m_parOffset(0)
    {
      setFunctionUnderMinimization(fittingFunction);
    }

    /**
     * Set a reference to the convolved fitting function. Required as we need a default constructor for the factory
     * @param fittingFunction :: This object has access to the current value of the model parameters
     */
    void ForegroundModel::setFunctionUnderMinimization(const API::IFunction & fittingFunction)
    {
      m_fittingFunction = &fittingFunction;
      m_parOffset = fittingFunction.nParams();
    }

    /**
     *  Declares the parameters
     */
    void ForegroundModel::declareParameters()
    {
      throw Kernel::Exception::NotImplementedError("Error: Override ForegroundModel::declareParameters() and declare model parameters");
    }

    /**
     * Return the initial value of the  parameter by index
     * @param index :: An index for the parameter
     * @return The current value
     */
    double ForegroundModel::getInitialParameterValue(size_t index) const
    {
      return this->getParameter(index);
    }

    /**
     * Return the initial value of the  parameter by name
     * @param name :: The name of a parameter
     * @return The current value
     */
    double ForegroundModel::getInitialParameterValue(const std::string& name) const
    {
      return this->getParameter(name);
    }

    /**
     * Return the current parameter according to the fit by index
     * @param index :: An index for the parameter
     * @return The current value
     */
    double ForegroundModel::getCurrentParameterValue(const size_t index) const
    {
      return functionUnderMinimization().getParameter(index + m_parOffset);
    }

    /**
     * Return the current parameter according to the fit by name
     * @param name :: The name of a parameter
     * @return The current value
     */
    double ForegroundModel::getCurrentParameterValue(const std::string& name) const
    {
      return functionUnderMinimization().getParameter(name);
    }

    //-------------------------------------------------------------------------
    // Protected members
    //-------------------------------------------------------------------------
    /**
     * @return Returns a reference to the fitting function
     */
    const API::IFunction & ForegroundModel::functionUnderMinimization() const
    {
      assert(m_fittingFunction);
      return *m_fittingFunction;
    }

  }
}
