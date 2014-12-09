#ifndef MANTID_KERNEL_VMD_H_
#define MANTID_KERNEL_VMD_H_
    
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
# include <boost/algorithm/string/split.hpp>
# include <boost/algorithm/string/trim.hpp>
#endif
#include <cstddef>
#include <sstream>
#include <stdexcept>


namespace Mantid
{
namespace Kernel
{

  /** Simple vector class for multiple dimensions (i.e. > 3).
    
    @author Janik Zikovsky
    @date 2011-08-30

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  template<typename TYPE=double>
  class DLLExport VMDBase
  {
  public:
    //-------------------------------------------------------------------------------------------
    /** Default constructor, build with 1 dimension */
    VMDBase()
    : nd(1)
    {
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(0.0);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param nd :: number of dimensions  */
    VMDBase(size_t nd)
    : nd(nd)
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(0.0);
    }

    //-------------------------------------------------------------------------------------------
    /** 2D Constructor
     * @param val0 :: value at first dimension
     * @param val1 :: value at second dimension
     */
    VMDBase(double val0, double val1)
    : nd(2)
    {
      data = new TYPE[nd];
      data[0] = TYPE(val0);
      data[1] = TYPE(val1);
    }

    //-------------------------------------------------------------------------------------------
    /** 3D Constructor
     * @param val0 :: value at first dimension
     * @param val1 :: value at second dimension
     * @param val2 :: value at third dimension
     */
    VMDBase(double val0, double val1, double val2)
    : nd(3)
    {
      data = new TYPE[nd];
      data[0] = TYPE(val0);
      data[1] = TYPE(val1);
      data[2] = TYPE(val2);
    }

    //-------------------------------------------------------------------------------------------
    /** 4D Constructor
     * @param val0 :: value at first dimension
     * @param val1 :: value at second dimension
     * @param val2 :: value at third dimension
     * @param val3 :: value at fourth dimension
     */
    VMDBase(double val0, double val1, double val2, double val3)
    : nd(4)
    {
      data = new TYPE[nd];
      data[0] = TYPE(val0);
      data[1] = TYPE(val1);
      data[2] = TYPE(val2);
      data[3] = TYPE(val3);
    }

    //-------------------------------------------------------------------------------------------
    /** 5D Constructor
     * @param val0 :: value at first dimension
     * @param val1 :: value at second dimension
     * @param val2 :: value at third dimension
     * @param val3 :: value at fourth dimension
     * @param val4 :: value at fifth dimension
     */
    VMDBase(double val0, double val1, double val2, double val3, double val4)
    : nd(5)
    {
      data = new TYPE[nd];
      data[0] = TYPE(val0);
      data[1] = TYPE(val1);
      data[2] = TYPE(val2);
      data[3] = TYPE(val3);
      data[4] = TYPE(val4);
    }

    //-------------------------------------------------------------------------------------------
    /** 6D Constructor
     * @param val0 :: value at first dimension
     * @param val1 :: value at second dimension
     * @param val2 :: value at third dimension
     * @param val3 :: value at fourth dimension
     * @param val4 :: value at fifth dimension
     * @param val5 :: value at sixth dimension
     */
    VMDBase(double val0, double val1, double val2, double val3, double val4, double val5)
    : nd(6)
    {
      data = new TYPE[nd];
      data[0] = TYPE(val0);
      data[1] = TYPE(val1);
      data[2] = TYPE(val2);
      data[3] = TYPE(val3);
      data[4] = TYPE(val4);
      data[5] = TYPE(val5);
    }

    //-------------------------------------------------------------------------------------------
    /** Copy constructor
     * @param other :: other to copy */
    VMDBase(const VMDBase & other)
    : nd(other.nd)
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = other.data[d];
    }


    //-------------------------------------------------------------------------------------------
    /** Assignment operator
     * @param other :: copy into this
     */
    VMDBase& operator=(const VMDBase& other)
    {
      if ((other.nd) != nd)
      {
        nd = other.nd;
        delete [] data;
        data = new TYPE[nd];
      }
      for (size_t d=0; d<nd; d++) data[d] = other.data[d];
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param nd :: number of dimensions
     * @param bareData :: pointer to a nd-sized bare data array */
    VMDBase(size_t nd, const double * bareData)
    : nd(nd)
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(bareData[d]);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param nd :: number of dimensions
     * @param bareData :: pointer to a nd-sized bare data array */
    VMDBase(size_t nd, const float * bareData)
    : nd(nd)
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(bareData[d]);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param vector :: V3D */
    VMDBase(const V3D & vector)
    : nd(3)
    {
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(vector[d]);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param vector :: vector of doubles */
    template<class T>
    VMDBase(const std::vector<T> & vector)
    : nd(vector.size())
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(vector[d]);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor
     * @param vector :: vector of floats */
    VMDBase(const std::vector<float> & vector)
    : nd(vector.size())
    {
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = TYPE(vector[d]);
    }

    //-------------------------------------------------------------------------------------------
    /** Constructor from string
     * @param str :: string of comma or space-separated numbers for each component */
    VMDBase(const std::string & str)
    {
      using boost::algorithm::split;
      using boost::algorithm::is_any_of;

      std::vector<std::string> strs;
      boost::split(strs, str, boost::is_any_of(", "));

      std::vector<TYPE> vals;
      for (size_t d=0; d<strs.size(); d++)
      {
        if (!strs[d].empty())
        {
          TYPE v;
          if (!Strings::convert(strs[d], v))
            throw std::invalid_argument("VMDBase: Unable to convert the string '" + strs[d] + "' to a number.");
          vals.push_back(v);
        }
      }
      nd = vals.size();
      if (nd <= 0) throw std::invalid_argument("nd must be > 0");
      data = new TYPE[nd];
      for (size_t d=0; d<nd; d++) data[d] = vals[d];
    }

    //-------------------------------------------------------------------------------------------
    /// Destructor
    virtual ~VMDBase()
    {
      delete [] data;
    }
    
    //-------------------------------------------------------------------------------------------
    /// @return the number of dimensions
    size_t getNumDims() const
    { return nd; }

    //-------------------------------------------------------------------------------------------
    /// @return the number of dimensions
    size_t size() const
    { return nd; }

    /** @return the value at the index */
    const TYPE& operator[](const size_t index) const
    { return data[index]; }

    /** @return the value at the index */
    TYPE& operator[](const size_t index)
    { return data[index]; }

    //-------------------------------------------------------------------------------------------
    /** @return the bare data array directly. */
    const TYPE * getBareArray() const
    {
      return data;
    }

    //-------------------------------------------------------------------------------------------
    /** Return a simple string representation of the vector
     * @param separator :: string to place between values, one space is the default
     */
    std::string toString(const std::string & separator = " ") const
    {
      std::ostringstream mess;
      for (size_t d=0; d<nd; d++)
        mess << (d>0?separator:"")
        << data[d];
      return mess.str();
    }

    //-------------------------------------------------------------------------------------------
    /** Get the vector as a vector
     * @tparam T :: type to convert to (double/float)
     * @return the vector as a std::vector
     */
    template<class T>
    std::vector<T> toVector() const
    {
      typename std::vector<T> out;
      for (size_t d=0; d<nd; d++)
        out.push_back(T(data[d]));
      return out;
    }


    //-------------------------------------------------------------------------------------------
    /** Equals operator with tolerance factor
      @param v :: VMDBase for comparison
      @return true if the items are equal
     */
    bool operator==(const VMDBase& v) const
    {
      if (v.nd != nd) return false;
      for (size_t d=0; d<nd; d++)
        if ((std::fabs(data[d]-v.data[d]) > Tolerance))
          return false;
      return true;
    }

    //-------------------------------------------------------------------------------------------
    /** Not-equals operator with tolerance factor
      @param v :: VMDBase for comparison
      @return true if the items are equal
     */
    bool operator!=(const VMDBase& v) const
    {
      return !operator==(v);
    }

    //-------------------------------------------------------------------------------------------
    /** Add two vectors together
     * @param v :: other vector, must match number of dimensions  */
    VMDBase operator+(const VMDBase& v) const
    {
      VMDBase out(*this);
      out += v;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Add two vectors together
     * @param v :: other vector, must match number of dimensions  */
    VMDBase & operator+=(const VMDBase& v)
    {
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      for (size_t d=0; d<nd; d++) data[d] += v.data[d];
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Subtract two vectors
     * @param v
     *  :: other vector, must match number of dimensions  */
    VMDBase operator-(const VMDBase& v) const
    {
      VMDBase out(*this);
      out -= v;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Subtract two vectors
     * @param v :: other vector, must match number of dimensions  */
    VMDBase & operator-=(const VMDBase& v)
    {
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      for (size_t d=0; d<nd; d++) data[d] -= v.data[d];
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Inner product of two vectors (element-by-element)
     * @param v :: other vector, must match number of dimensions  */
    VMDBase operator*(const VMDBase& v) const
    {
      VMDBase out(*this);
      out *= v;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Inner product of two vectors (element-by-element)
     * @param v :: other vector, must match number of dimensions  */
    VMDBase & operator*=(const VMDBase& v)
    {
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      for (size_t d=0; d<nd; d++) data[d] *= v.data[d];
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Inner division of two vectors (element-by-element)
     * @param v :: other vector, must match number of dimensions  */
    VMDBase operator/(const VMDBase& v) const
    {
      VMDBase out(*this);
      out /= v;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Inner division of two vectors (element-by-element)
     * @param v :: other vector, must match number of dimensions  */
    VMDBase & operator/=(const VMDBase& v)
    {
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      for (size_t d=0; d<nd; d++) data[d] /= v.data[d];
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Multiply by a scalar
     * @param scalar :: double scalar to multiply each element  */
    VMDBase operator*(const double scalar) const
    {
      VMDBase out(*this);
      out *= scalar;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Multiply by a scalar
     * @param scalar :: double scalar to multiply each element  */
    VMDBase & operator*=(const double scalar)
    {
      for (size_t d=0; d<nd; d++) data[d] *= TYPE(scalar);
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Divide by a scalar
     * @param scalar :: double scalar to Divide each element  */
    VMDBase operator/(const double scalar) const
    {
      VMDBase out(*this);
      out /= scalar;
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Divide by a scalar
     * @param scalar :: double scalar to Divide each element  */
    VMDBase & operator/=(const double scalar)
    {
      for (size_t d=0; d<nd; d++) data[d] /= TYPE(scalar);
      return *this;
    }

    //-------------------------------------------------------------------------------------------
    /** Scalar product of two vectors
     * @param v :: other vector, must match number of dimensions  */
    TYPE scalar_prod(const VMDBase& v) const
    {
      TYPE out=0;
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      for (size_t d=0; d<nd; d++)
        out += (data[d] * v.data[d]);
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** Cross product of two vectors. Only works in 3D
     * @param v :: other vector, also 3D  */
    VMDBase cross_prod(const VMDBase& v) const
    {
      if (v.nd != this->nd) throw std::runtime_error("Mismatch in number of dimensions in operation between two VMDBase vectors.");
      if (v.nd != 3) throw std::runtime_error("Cross product of vectors only works in 3 dimensions.");
      V3D a(data[0], data[1], data[2]);
      V3D b(v.data[0], v.data[1], v.data[2]);
      V3D c = a.cross_prod(b);
      VMDBase out(c);
      return out;
    }

    //-------------------------------------------------------------------------------------------
    /** @return the length of this vector */
    TYPE length() const
    {
      return TYPE(std::sqrt(this->norm2()));
    }

    /** @return the length of this vector */
    TYPE norm() const
    {
      return this->length();
    }

    /** @return the length of this vector */
    TYPE norm2() const
    {
      return this->scalar_prod(*this);
    }

    //-------------------------------------------------------------------------------------------
    /** Normalize this vector to unity length
     * @return the length of this vector BEFORE normalizing */
    TYPE normalize()
    {
      TYPE length = this->length();
      for (size_t d=0; d<nd; d++)
        data[d] /= length;
      return length;
    }


    //-------------------------------------------------------------------------------------------
    /** Return the angle between this and another vector
     *  @param v :: The other vector
     *  @return The angle between the vectors in radians (0 < theta < pi)
     */
    TYPE angle(const VMDBase& v) const
    {
      return TYPE(acos( this->scalar_prod(v) / (this->norm() * v.norm()) ));
    }

    static std::vector<VMDBase> makeVectorsOrthogonal(std::vector<VMDBase> & vectors);
    static VMDBase getNormalVector(const std::vector<VMDBase> & vectors);

  protected:
    /// Number of dimensions
    size_t nd;
    /// Data, an array of size nd
    TYPE * data;
  };

  /// Underlying data type for the VMD type
  typedef float VMD_t;

  /// Define the VMD as using the double or float data type.
  typedef VMDBase<VMD_t> VMD;

  // Overload operator <<
  MANTID_KERNEL_DLL std::ostream& operator<<(std::ostream&, const VMDBase<double>&);
  MANTID_KERNEL_DLL std::ostream& operator<<(std::ostream&, const VMDBase<float>&);

} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_VMD_H_ */
