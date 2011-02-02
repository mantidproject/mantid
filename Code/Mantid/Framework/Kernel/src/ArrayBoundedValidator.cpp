#include "MantidKernel/ArrayBoundedValidator.h"

#include <iostream>
#include <sstream>
#include <string>

namespace Mantid
{
namespace Kernel
{

/**
 * Default constructor
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator() : IValidator<std::vector<TYPE> >()
{
  this->boundVal = new BoundedValidator<TYPE>();
}

/**
 * Copy constructor
 * @param abv :: the ArrayBoundedValidator to copy
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(const ArrayBoundedValidator<TYPE> &abv) : IValidator<std::vector<TYPE> >()
{
  this->boundVal = dynamic_cast<BoundedValidator<TYPE> *>(abv.boundVal->clone());
}

/**
 * Constructor via bounds parameters
 * @param lowerBound :: the lower bound value to validate
 * @param upperBound :: the upper bound value to validate
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(const TYPE lowerBound, const TYPE upperBound) : IValidator<std::vector<TYPE> >()
{
  this->boundVal = new BoundedValidator<TYPE>(lowerBound, upperBound);
}

/**
 * Constructor via a BoundedValidator
 * @param bv :: the BoundedValidator object to use
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(BoundedValidator<TYPE> &bv)
{
  this->boundVal = dynamic_cast<BoundedValidator<TYPE> *>(bv.clone());
}

/**
 * Object destructor
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::~ArrayBoundedValidator()
{
  if (this->boundVal)
  {
    delete this->boundVal;
  }
}

/**
 * Create a clone of the current ArrayBoundedValidator
 * @return the cloned object
 */
template <typename TYPE>
IValidator<std::vector<TYPE> >* ArrayBoundedValidator<TYPE>::clone()
{
  return new ArrayBoundedValidator<TYPE>(*this);
}

/**
 * Accessor for the stored BoundedValidator
 * @return a pointer to the stored BoundedValidator
 */
template <typename TYPE>
BoundedValidator<TYPE>* ArrayBoundedValidator<TYPE>::getValidator() const
{
  return this->boundVal;
}

/**
 * Function to check the validity of the array elements
 * @param value :: the array to be checked
 * @return a listing of the indicies that fail the bounds checks
 */
template <typename TYPE>
std::string
ArrayBoundedValidator<TYPE>::isValid( const std::vector<TYPE> &value ) const
{
  std::string failure = this->checkValidity(value);
  return failure;
}

/**
 * Function that actually does the work of checking the validity of the
 * array elements.
 * @param value :: the array to be checked
 * @return a listing of the indicies that fail the bounds checks
 */
template <typename TYPE>
std::string
ArrayBoundedValidator<TYPE>::checkValidity( const std::vector<TYPE> &value ) const
{
  // declare a class that can do conversions to string
  std::ostringstream error;
  // load in the "no error" condition
  error << "";
  typename std::vector<TYPE>::const_iterator it;
  std::size_t index = 0;
  for (it = value.begin(); it != value.end(); ++it)
  {
    std::string retval = this->boundVal->isValid(*it);
    if ( !retval.empty() )
    {
      error << "At index " << index << ": " << retval;
    }
    index++;
  }

  return error.str();
}

template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::hasLower() const
{
  return this->boundVal->hasLower();
}

template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::hasUpper() const
{
  return this->boundVal->hasUpper();
}

template <typename TYPE>
const TYPE& ArrayBoundedValidator<TYPE>::lower() const
{
  return this->boundVal->lower();
}

template <typename TYPE>
const TYPE& ArrayBoundedValidator<TYPE>::upper() const
{
  return this->boundVal->upper();
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setLower( const TYPE& value )
{
  this->boundVal->setLower(value);
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setUpper( const TYPE& value )
{
  this->boundVal->setUpper(value);
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::clearLower()
{
  this->boundVal->clearLower();
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::clearUpper()
{
  this->boundVal->clearUpper();
}

// Required explicit instantiations
template class ArrayBoundedValidator<double>;
template class ArrayBoundedValidator<int>;

} // Kernel
} // Mantid
