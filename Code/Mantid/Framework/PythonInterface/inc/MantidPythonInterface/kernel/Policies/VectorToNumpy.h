#ifndef MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
#define MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include "MantidKernel/System.h"
#include "MantidPythonInterface/kernel/NumpyConverters.h"
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/and.hpp>
#include <vector>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Policies
    {
      //-----------------------------------------------------------------------
      // Conversion Policies
      //-----------------------------------------------------------------------

      /**
       * WrapReadOnly is a policy for VectorToPython
       * to wrap the vector in a read-only numpy array
       * that looks at the original data. No copy is performed
       */

      struct WrapReadOnly
      {
        template<typename VectorType>
        struct apply
        {
          /**
           * Returns a read-only Numpy array wrapped around an existing vector
           * @param cvector
           * @return
           */
          static PyObject * create(const VectorType & cvector)
          {
            return Numpy::wrapWithReadOnlyNumpy(cvector);
          }
        };
      };

      /**
       * WrapReadWrite is a policy for VectorToPython
       * to wrap the vector in a read-write numpy array
       * that looks at the original data. No copy is performed
       */
      struct WrapReadWrite
      {
        template<typename VectorType>
        struct apply
        {
          /**
           * Returns a read-write Numpy array wrapped around an existing vector
           * @param cvector
           * @return
           */
          static PyObject * create(const VectorType & cvector)
          {
            return Numpy::wrapWithNumpy(cvector);
          }
        };
      };

      namespace // anonymous
      {
        /**
         * Helper struct that implements the conversion
         * policy.
         */
        template<typename VectorType, typename ConversionPolicy>
        struct ConvertVectorToNDArray
        {
          inline PyObject * operator()(const VectorType & cvector) const
          {
            typedef typename ConversionPolicy::template apply<VectorType> policy;
            return policy::create(cvector);
          }

          inline PyTypeObject const* get_pytype() const
          {
            return Numpy::getNDArrayType();
          }
        };
      }

      //-----------------------------------------------------------------------
      // return_value_policy
      //-----------------------------------------------------------------------
      namespace
      {
        /// MPL struct to figure out if a type is a std::vector
        /// The general one inherits from boost::false_type
        template<typename T>
        struct is_std_vector : boost::false_type
        {};

        /// Specialization for std::vector types to inherit from
        /// boost::true_type
        template<typename T>
        struct is_std_vector<std::vector<T> > : boost::true_type
        {};

        template<typename T>
        struct VectorToNumpy_Requires_StdVector_Return_Type
        {};

      }
      /**
       * Implements a return value policy that
       * returns a numpy array from a std::vector
       *
       * The type of conversion is specified by a policy:
       * (1) WrapReadOnly - Creates a read-only array around the original data (no copy is performed)
       * (2) WrapReadWrite - Creates a read-write array around the original data (no copy is performed)
       */
      template<typename ConversionPolicy>
      struct VectorToNumpy
      {
        // The boost::python framework calls return_value_policy::apply<T>::type
        template <class T>
        struct apply
        {
          // Typedef that removes and const or reference qualifiers from the type
          typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type non_const_type;
          // MPL compile-time check that T is a reference to a std::vector
          typedef typename boost::mpl::if_c<
               boost::mpl::and_<boost::is_reference<T>, is_std_vector<non_const_type> >::value
             , ConvertVectorToNDArray<T, ConversionPolicy>
             , VectorToNumpy_Requires_StdVector_Return_Type<T>
             >::type type;
        };
      };

    }
  }
}

#endif // MANTID_PYTHONINTERFACE_VECTORTONUMPY_H_
