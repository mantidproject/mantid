//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/MatrixProperty.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
  namespace API
  {
    /**
     * Constructor
     * @param propName :: Name of the property
     * @param validator :: A pointer to a validator whose ownership is 
     * transferred to this object
     * @param direction :: The direction 
     */
    template<typename TYPE>
    MatrixProperty<TYPE>::MatrixProperty(const std::string & propName,
      Kernel::IValidator<HeldType> *validator, unsigned int direction) 
      : Kernel::PropertyWithValue<HeldType>(propName, HeldType(), validator, direction)
    {
    }

    /**
    * Copy constructor
    * @param rhs :: Contruct this object from rhs
    */
    template<typename TYPE>
    MatrixProperty<TYPE>::MatrixProperty(const MatrixProperty & rhs)
      : Kernel::PropertyWithValue<HeldType>(rhs)
    {
    }

    /// Destructor
    template<typename TYPE>
    MatrixProperty<TYPE>::~MatrixProperty()
    {
    }

    ///@cond
    // Symbol definitions
    template class MANTID_API_DLL MatrixProperty<double>;
    template class MANTID_API_DLL MatrixProperty<int>;
    template class MANTID_API_DLL MatrixProperty<float>;
    ///@endcond
  }
}

/**
 * IPropertyManager::getValue definitions so that algorithm.getProperty will work
 */
///@cond
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Geometry::DblMatrix);
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Geometry::IntMatrix);
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Geometry::Matrix<float>);

///@endcond

