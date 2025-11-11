// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/VMD.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid::Kernel {

/** Default constructor, build with 1 dimension */
template <typename TYPE> VMDBase<TYPE>::VMDBase() : nd(1) {
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(0.0);
}

/** Constructor
 * @param nd :: number of dimensions  */
template <typename TYPE> VMDBase<TYPE>::VMDBase(size_t nd) : nd(nd) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(0.0);
}

/** 2D Constructor
 * @param val0 :: value at first dimension
 * @param val1 :: value at second dimension
 */
template <typename TYPE> VMDBase<TYPE>::VMDBase(double val0, double val1) : nd(2) {
  data = new TYPE[nd];
  data[0] = TYPE(val0);
  data[1] = TYPE(val1);
}

/** 3D Constructor
 * @param val0 :: value at first dimension
 * @param val1 :: value at second dimension
 * @param val2 :: value at third dimension
 */
template <typename TYPE> VMDBase<TYPE>::VMDBase(double val0, double val1, double val2) : nd(3) {
  data = new TYPE[nd];
  data[0] = TYPE(val0);
  data[1] = TYPE(val1);
  data[2] = TYPE(val2);
}

/** 4D Constructor
 * @param val0 :: value at first dimension
 * @param val1 :: value at second dimension
 * @param val2 :: value at third dimension
 * @param val3 :: value at fourth dimension
 */
template <typename TYPE> VMDBase<TYPE>::VMDBase(double val0, double val1, double val2, double val3) : nd(4) {
  data = new TYPE[nd];
  data[0] = TYPE(val0);
  data[1] = TYPE(val1);
  data[2] = TYPE(val2);
  data[3] = TYPE(val3);
}

/** 5D Constructor
 * @param val0 :: value at first dimension
 * @param val1 :: value at second dimension
 * @param val2 :: value at third dimension
 * @param val3 :: value at fourth dimension
 * @param val4 :: value at fifth dimension
 */
template <typename TYPE>
VMDBase<TYPE>::VMDBase(double val0, double val1, double val2, double val3, double val4) : nd(5) {
  data = new TYPE[nd];
  data[0] = TYPE(val0);
  data[1] = TYPE(val1);
  data[2] = TYPE(val2);
  data[3] = TYPE(val3);
  data[4] = TYPE(val4);
}

/** 6D Constructor
 * @param val0 :: value at first dimension
 * @param val1 :: value at second dimension
 * @param val2 :: value at third dimension
 * @param val3 :: value at fourth dimension
 * @param val4 :: value at fifth dimension
 * @param val5 :: value at sixth dimension
 */
template <typename TYPE>
VMDBase<TYPE>::VMDBase(double val0, double val1, double val2, double val3, double val4, double val5) : nd(6) {
  data = new TYPE[nd];
  data[0] = TYPE(val0);
  data[1] = TYPE(val1);
  data[2] = TYPE(val2);
  data[3] = TYPE(val3);
  data[4] = TYPE(val4);
  data[5] = TYPE(val5);
}

/** Copy constructor
 * @param other :: other to copy */
template <typename TYPE> VMDBase<TYPE>::VMDBase(const VMDBase &other) : nd(other.nd) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = other.data[d];
}

/** Assignment operator
 * @param other :: copy into this
 */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator=(const VMDBase &other) {
  if ((other.nd) != nd) {
    nd = other.nd;
    delete[] data;
    data = new TYPE[nd];
  }
  for (size_t d = 0; d < nd; d++)
    data[d] = other.data[d];
  return *this;
}

/** Move constructor
 * @param other :: move into this
 */
template <typename TYPE> VMDBase<TYPE>::VMDBase(VMDBase &&other) noexcept : nd(other.nd), data(other.data) {
  other.data = nullptr;
  other.nd = 0;
}

/** Move assignment
 * @param other :: move into this
 */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator=(VMDBase &&other) noexcept {
  if (this != &other) {
    this->nd = other.nd;
    other.nd = 0;
    delete[] this->data;
    this->data = other.data;
    other.data = nullptr;
  }
  return *this;
}

/** Constructor
 * @param nd :: number of dimensions
 * @param bareData :: pointer to a nd-sized bare data array */
template <typename TYPE> VMDBase<TYPE>::VMDBase(size_t nd, const double *bareData) : nd(nd) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(bareData[d]);
}

/** Constructor
 * @param nd :: number of dimensions
 * @param bareData :: pointer to a nd-sized bare data array */
template <typename TYPE> VMDBase<TYPE>::VMDBase(size_t nd, const float *bareData) : nd(nd) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(bareData[d]);
}

/** Constructor
 * @param vector :: V3D */
template <typename TYPE> VMDBase<TYPE>::VMDBase(const V3D &vector) : nd(3) {
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(vector[d]);
}

/** Constructor
 * @param vector :: vector of doubles */
template <typename TYPE> VMDBase<TYPE>::VMDBase(const std::vector<double> &vector) : nd(vector.size()) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(vector[d]);
}

/** Constructor
 * @param vector :: vector of floats */
template <typename TYPE> VMDBase<TYPE>::VMDBase(const std::vector<float> &vector) : nd(vector.size()) {
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  for (size_t d = 0; d < nd; d++)
    data[d] = TYPE(vector[d]);
}

/** Constructor from string
 * @param str :: string of comma or space-separated numbers for each component
 */
template <typename TYPE> VMDBase<TYPE>::VMDBase(const std::string &str) {

  StringTokenizer strs(str, ", ", StringTokenizer::TOK_IGNORE_EMPTY);

  std::vector<TYPE> vals;
  std::transform(strs.cbegin(), strs.cend(), std::back_inserter(vals), [](const std::string &token) {
    TYPE v;
    if (!Strings::convert(token, v))
      throw std::invalid_argument("VMDBase: Unable to convert the string '" + token + "' to a number.");
    return v;
  });

  nd = vals.size();
  if (nd == 0)
    throw std::invalid_argument("nd must be > 0");
  data = new TYPE[nd];
  std::copy(vals.cbegin(), vals.cend(), data);
}

/// Destructor
template <typename TYPE> VMDBase<TYPE>::~VMDBase() { delete[] data; }

/// @return the number of dimensions
template <typename TYPE> size_t VMDBase<TYPE>::getNumDims() const { return nd; }

/// @return the number of dimensions
template <typename TYPE> size_t VMDBase<TYPE>::size() const { return nd; }

/** @return the value at the index */
template <typename TYPE> const TYPE &VMDBase<TYPE>::operator[](const size_t index) const { return data[index]; }

/** @return the value at the index */
template <typename TYPE> TYPE &VMDBase<TYPE>::operator[](const size_t index) { return data[index]; }

/** @return the bare data array directly. */
template <typename TYPE> const TYPE *VMDBase<TYPE>::getBareArray() const { return data; }

/** Return a simple string representation of the vector
 * @param separator :: string to place between values, one space is the
 * default
 */
template <typename TYPE> std::string VMDBase<TYPE>::toString(const std::string &separator) const {
  std::ostringstream mess;
  for (size_t d = 0; d < nd; d++)
    mess << (d > 0 ? separator : "") << data[d];
  return mess.str();
}

/** Equals operator with tolerance factor
  @param v :: VMDBase for comparison
  @return true if the items are equal
 */
template <typename TYPE> bool VMDBase<TYPE>::operator==(const VMDBase &v) const {
  if (v.nd != nd)
    return false;
  for (size_t d = 0; d < nd; d++)
    if ((std::fabs(data[d] - v.data[d]) > Tolerance))
      return false;
  return true;
}

/** Not-equals operator with tolerance factor
  @param v :: VMDBase for comparison
  @return true if the items are equal
 */
template <typename TYPE> bool VMDBase<TYPE>::operator!=(const VMDBase &v) const { return !operator==(v); }

/** Add two vectors together
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator+(const VMDBase &v) const {
  VMDBase out(*this);
  out += v;
  return out;
}

/** Add two vectors together
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator+=(const VMDBase &v) {
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  for (size_t d = 0; d < nd; d++)
    data[d] += v.data[d];
  return *this;
}

/** Subtract two vectors
 * @param v
 *  :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator-(const VMDBase &v) const {
  VMDBase out(*this);
  out -= v;
  return out;
}

/** Subtract two vectors
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator-=(const VMDBase &v) {
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  for (size_t d = 0; d < nd; d++)
    data[d] -= v.data[d];
  return *this;
}

/** Inner product of two vectors (element-by-element)
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator*(const VMDBase &v) const {
  VMDBase out(*this);
  out *= v;
  return out;
}

/** Inner product of two vectors (element-by-element)
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator*=(const VMDBase &v) {
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  for (size_t d = 0; d < nd; d++)
    data[d] *= v.data[d];
  return *this;
}

/** Inner division of two vectors (element-by-element)
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator/(const VMDBase &v) const {
  VMDBase out(*this);
  out /= v;
  return out;
}

/** Inner division of two vectors (element-by-element)
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator/=(const VMDBase &v) {
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  for (size_t d = 0; d < nd; d++)
    data[d] /= v.data[d];
  return *this;
}

/** Multiply by a scalar
 * @param scalar :: double scalar to multiply each element  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator*(const double scalar) const {
  VMDBase out(*this);
  out *= scalar;
  return out;
}

/** Multiply by a scalar
 * @param scalar :: double scalar to multiply each element  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator*=(const double scalar) {
  for (size_t d = 0; d < nd; d++)
    data[d] *= TYPE(scalar);
  return *this;
}

/** Divide by a scalar
 * @param scalar :: double scalar to Divide each element  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::operator/(const double scalar) const {
  VMDBase out(*this);
  out /= scalar;
  return out;
}

/** Divide by a scalar
 * @param scalar :: double scalar to Divide each element  */
template <typename TYPE> VMDBase<TYPE> &VMDBase<TYPE>::operator/=(const double scalar) {
  for (size_t d = 0; d < nd; d++)
    data[d] /= TYPE(scalar);
  return *this;
}

/** Scalar product of two vectors
 * @param v :: other vector, must match number of dimensions  */
template <typename TYPE> TYPE VMDBase<TYPE>::scalar_prod(const VMDBase &v) const {
  TYPE out = 0;
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  for (size_t d = 0; d < nd; d++)
    out += (data[d] * v.data[d]);
  return out;
}

/** Cross product of two vectors. Only works in 3D
 * @param v :: other vector, also 3D  */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::cross_prod(const VMDBase &v) const {
  if (v.nd != this->nd)
    throw std::runtime_error("Mismatch in number of dimensions in operation "
                             "between two VMDBase vectors.");
  if (v.nd != 3)
    throw std::runtime_error("Cross product of vectors only works in 3 dimensions.");
  V3D a(data[0], data[1], data[2]);
  V3D b(v.data[0], v.data[1], v.data[2]);
  V3D c = a.cross_prod(b);
  VMDBase out(c);
  return out;
}

/** @return the length of this vector */
template <typename TYPE> TYPE VMDBase<TYPE>::length() const { return TYPE(std::sqrt(this->norm2())); }

/** @return the length of this vector */
template <typename TYPE> TYPE VMDBase<TYPE>::norm() const { return this->length(); }

/** @return the length of this vector */
template <typename TYPE> TYPE VMDBase<TYPE>::norm2() const { return this->scalar_prod(*this); }

/** Normalize this vector to unity length
 * @return the length of this vector BEFORE normalizing */
template <typename TYPE> TYPE VMDBase<TYPE>::normalize() {
  TYPE lengthPreNormalisation = this->length();
  for (size_t d = 0; d < nd; d++)
    data[d] /= lengthPreNormalisation;
  return lengthPreNormalisation;
}

/** Return the angle between this and another vector
 *  @param v :: The other vector
 *  @return The angle between the vectors in radians (0 < theta < pi)
 */
template <typename TYPE> TYPE VMDBase<TYPE>::angle(const VMDBase &v) const {
  return TYPE(acos(this->scalar_prod(v) / (this->norm() * v.norm())));
}

//-------------------------------------------------------------------------------------------------
/** Make an orthogonal system with 2 input 3D vectors.
 * Currently only works in 3D!
 *
 * @param vectors :: list of 2 vectors with 3D
 * @return list of 3 vectors
 */
template <typename TYPE>
std::vector<VMDBase<TYPE>> VMDBase<TYPE>::makeVectorsOrthogonal(std::vector<VMDBase> &vectors) {
  if (vectors.size() != 2)
    throw std::runtime_error("VMDBase::makeVectorsOrthogonal(): Need 2 input vectors.");
  if (vectors[0].getNumDims() != 3 || vectors[1].getNumDims() != 3)
    throw std::runtime_error("VMDBase::makeVectorsOrthogonal(): Need 3D input vectors.");
  std::vector<V3D> in, out;
  for (size_t i = 0; i < vectors.size(); i++)
    in.emplace_back(vectors[i][0], vectors[i][1], vectors[i][2]);
  out = V3D::makeVectorsOrthogonal(in);
  std::vector<VMDBase> retVal;
  retVal.reserve(out.size());
  std::copy(std::make_move_iterator(out.begin()), std::make_move_iterator(out.end()), std::back_inserter(retVal));
  return retVal;
}

//-------------------------------------------------------------------------------------------------
/** Given N-1 vectors defining a N-1 dimensional hyperplane in N dimensions,
 * returns a vector that is normal (perpendicular) to all the input vectors
 *
 * Given planar vectors a, b, c, ...
 * Build a NxN matrix of this style:
 *  x1  x2  x3  x4
 *  a1  a2  a4  a4
 *  b1  b2  b4  b4
 *  c1  c2  c4  c4
 *
 * Where xn = the basis unit vector of the space, e.g. x1 = x, x2 = y, etc.
 *
 * The determinant of the matrix gives the normal vector. This is analogous
 * to the determinant method of finding the cross product of 2 3D vectors.
 *
 * It can be shown that the resulting vector n is such that:
 *  n . a = 0; n . b = 0 etc.
 * ... meaning that all the in-plane vectors are perpendicular to the normal,
 *which is what we wanted!
 *
 * (I've worked it out in 4D and its a long proof (not shown here)
 * I'm assuming it holds for higher dimensions,
 * I'll let a mathematician prove this :) )
 *
 * @param vectors :: vector of N-1 vectors with N dimensions.
 * @throw if the vectors are collinear
 * @return the normal vector
 */
template <typename TYPE> VMDBase<TYPE> VMDBase<TYPE>::getNormalVector(const std::vector<VMDBase<TYPE>> &vectors) {
  if (vectors.empty())
    throw std::invalid_argument("VMDBase::getNormalVector: Must give at least 1 vector");
  size_t nd = vectors[0].getNumDims();
  if (nd < 2)
    throw std::invalid_argument("VMDBase::getNormalVector: Must have at least 2 dimensions!");
  if (vectors.size() != nd - 1)
    throw std::invalid_argument("VMDBase::getNormalVector: Must have as many "
                                "N-1 vectors if there are N dimensions.");
  for (size_t i = 0; i < vectors.size(); i++)
    if (vectors[i].getNumDims() != nd)
      throw std::invalid_argument("VMDBase::getNormalVector: Inconsistent "
                                  "number of dimensions in the vectors given!");

  // Start the normal vector
  VMDBase normal = VMDBase(nd);
  TYPE sign = +1.0;
  for (size_t d = 0; d < nd; d++) {
    // Make the sub-determinant with the columns of every other dimension.
    Matrix<TYPE> mat(nd - 1, nd - 1);
    for (size_t row = 0; row < vectors.size(); row++) {
      VMDBase vec = vectors[row];
      size_t col = 0;
      for (size_t i = 0; i < nd; i++) {
        if (i != d) // Skip the column of this dimension
        {
          mat[row][col] = vec[i];
          col++;
        }
      }
    } // Building the matrix rows

    TYPE det = mat.determinant();

    // The determinant of the sub-matrix = the normal at that dimension
    normal[d] = sign * det;

    // Sign flips each time
    sign *= TYPE(-1.0);
  } // each dimension of the normal vector

  // Unity normal is better.
  double lengthPreNormilisation = normal.normalize();
  if (lengthPreNormilisation == 0)
    throw std::runtime_error("VMDBase::getNormalVector: 0-length normal found. "
                             "Are your vectors collinear?");

  return normal;
}

/// Instantiate VMDBase classes
template class VMDBase<double>;
template class VMDBase<float>;

/**
  Prints a text representation of itself
  @param os :: the Stream to output to
  @param v :: the vector to output
  @return the output stream
  */
std::ostream &operator<<(std::ostream &os, const VMDBase<double> &v) {
  os << v.toString();
  return os;
}

std::ostream &operator<<(std::ostream &os, const VMDBase<float> &v) {
  os << v.toString();
  return os;
}

} // namespace Mantid::Kernel
