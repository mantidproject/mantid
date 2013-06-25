//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MagneticFormFactorTable.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    namespace
    {
      /// Length of the form factor interpolation table
      const int FORM_FACTOR_TABLE_LENGTH = 500;
      /// Name of the form factor attribute
      const char * FORM_FACTOR_ION = "FormFactorIon";
    }

    /**
     * Default constructor only callable by the factory
     */
    ForegroundModel::ForegroundModel()
      : API::ParamFunction(), m_fittingFunction(NULL), m_parOffset(0),
        m_formFactorTable(NULL)
    {
      addAttributes();
      setFormFactorIon("0"); // Off
    }

    /**
     * Constructor taking the fitted function to access the current parameter values
     * @param fittingFunction :: A reference to the fitting function
     */
    ForegroundModel::ForegroundModel(const API::IFunction & fittingFunction)
      : API::ParamFunction(), m_fittingFunction(NULL), m_parOffset(0),
        m_formFactorTable(NULL)
    {
      addAttributes();
      setFormFactorIon("0"); // Off
      setFunctionUnderMinimization(fittingFunction);
    }

    /**
     */
    ForegroundModel::~ForegroundModel()
    {
      delete m_formFactorTable;
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
     * Called when an attribute value is set
     * @param name :: The name of the attribute
     * @param attr :: The value of the attribute
     */
    void ForegroundModel::setAttribute(const std::string & name, const API::IFunction::Attribute & attr)
    {
      if(name == FORM_FACTOR_ION)
      {
        setFormFactorIon(attr.asString());
      }
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

    /**
     * Set the default ion type for the form factor calculation
     * @param ionType A symbol string containing the atom and charge
     */
    void ForegroundModel::setFormFactorIon(const std::string & ionType)
    {
      // "0" indicates off
      if(ionType == "0")
      {
        delete m_formFactorTable;
        m_formFactorTable = NULL;
      }
      else
      {
        using namespace PhysicalConstants;
        m_formFactorTable = new MagneticFormFactorTable(FORM_FACTOR_TABLE_LENGTH, getMagneticIon(ionType));
      }
    }

    /**
     * Returns the form factor for the given \f$Q^2\f$ value
     * @param qsqr :: \f$Q^2\f$ in \f$\AA^{-2}\f$
     * @returns The form factor for the given factor or 1.0 if turned off
     */
    double ForegroundModel::formFactor(const double qsqr) const
    {
      if(m_formFactorTable) return m_formFactorTable->value(qsqr);
      else return 1.0;
    }

    //-------------------------------------------------------------------------
    // Private members
    //-------------------------------------------------------------------------
    
    /**
     * Add attributes common to all models
     */
    void ForegroundModel::addAttributes()
    {
      // Ion type string for form factor. "0" = off
      declareAttribute(FORM_FACTOR_ION, API::IFunction::Attribute("0"));      
    }

  }
}
