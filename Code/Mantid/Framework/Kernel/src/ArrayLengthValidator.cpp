#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** No arg constructor
   */
  template <typename TYPE>
  ArrayLengthValidator<TYPE>::ArrayLengthValidator():IValidator<std::vector<TYPE> >(),m_arraySize(size_t(0)),m_hasArraySize(false)
  {
  }
  //----------------------------------------------------------------------------------------------
  /** Constructor
   * @param len:: the legth of the array
   */
  template <typename TYPE>
  ArrayLengthValidator<TYPE>::ArrayLengthValidator(const size_t len):IValidator<std::vector<TYPE> >(),m_arraySize(size_t(len)),m_hasArraySize(true)
  {
  }
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  template <typename TYPE>
  ArrayLengthValidator<TYPE>::~ArrayLengthValidator()
  {
  }
  template <typename TYPE>
  bool  ArrayLengthValidator<TYPE>::hasLength() const
  {
    return this->m_hasArraySize;
  }
  template <typename TYPE>
  const size_t&   ArrayLengthValidator<TYPE>:: getLength()    const
  {
    return this->m_arraySize;
  }
  template <typename TYPE>
  void ArrayLengthValidator<TYPE>::setLength(const size_t &value)
  {
    this->m_hasArraySize=true;
    this->m_arraySize=value;
  }
  template <typename TYPE>
  void ArrayLengthValidator<TYPE>::clearLength()
  {
    this->m_hasArraySize=false;
    this->m_arraySize=size_t(0);
  }
  template <typename TYPE>
  IValidator<std::vector <TYPE> >* ArrayLengthValidator<TYPE>::clone()
  {
    return new ArrayLengthValidator(*this);
  }

  template <typename TYPE>
  std::string ArrayLengthValidator<TYPE>::isValid(const std::vector<TYPE> &value ) const
  {
    return this->checkValidity(value);
  }

  template <typename TYPE>
  std::string ArrayLengthValidator<TYPE>::checkValidity( const std::vector<TYPE> &value ) const
  {
    if (value.size()==this->m_arraySize )
    {
      return "";
    }
    else return"Incorrect size";
  }
  // Required explicit instantiations
  template class ArrayLengthValidator<double>;
  template class ArrayLengthValidator<int>;
} // namespace Mantid
} // namespace Kernel
