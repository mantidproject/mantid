//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/MatrixProperty.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace Kernel {
/**
 * Constructor
 * @param propName :: Name of the property
 * @param validator :: A pointer to a validator whose ownership is
 * transferred to this object
 * @param direction :: The direction
 */
template <typename TYPE>
MatrixProperty<TYPE>::MatrixProperty(const std::string &propName,
                                     IValidator_sptr validator,
                                     unsigned int direction)
    : PropertyWithValue<HeldType>(propName, HeldType(), validator, direction) {}

/**
* Copy constructor
* @param rhs :: Contruct this object from rhs
*/
template <typename TYPE>
MatrixProperty<TYPE>::MatrixProperty(const MatrixProperty &rhs)
    : PropertyWithValue<HeldType>(rhs) {}

/// Destructor
template <typename TYPE> MatrixProperty<TYPE>::~MatrixProperty() {}

///@cond
// Symbol definitions
template class MANTID_KERNEL_DLL MatrixProperty<double>;
template class MANTID_KERNEL_DLL MatrixProperty<int>;
template class MANTID_KERNEL_DLL MatrixProperty<float>;
///@endcond
}
}

///@cond
/**
 * IPropertyManager::getValue definitions
 */
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Kernel::DblMatrix);
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Kernel::IntMatrix);
DEFINE_IPROPERTYMANAGER_GETVALUE(Mantid::Kernel::Matrix<float>);
///@endcond
