#include "MantidKernel/VMD.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {

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

//-------------------------------------------------------------------------------------------------
/** Make an orthogonal system with 2 input 3D vectors.
 * Currently only works in 3D!
 *
 * @param vectors :: list of 2 vectors with 3D
 * @return list of 3 vectors
 */
template <typename TYPE>
std::vector<VMDBase<TYPE>>
VMDBase<TYPE>::makeVectorsOrthogonal(std::vector<VMDBase> &vectors) {
  if (vectors.size() != 2)
    throw std::runtime_error(
        "VMDBase::makeVectorsOrthogonal(): Need 2 input vectors.");
  if (vectors[0].getNumDims() != 3 || vectors[1].getNumDims() != 3)
    throw std::runtime_error(
        "VMDBase::makeVectorsOrthogonal(): Need 3D input vectors.");
  std::vector<V3D> in, out;
  for (size_t i = 0; i < vectors.size(); i++)
    in.push_back(V3D(vectors[i][0], vectors[i][1], vectors[i][2]));
  out = V3D::makeVectorsOrthogonal(in);

  std::vector<VMDBase> retVal;
  for (size_t i = 0; i < out.size(); i++)
    retVal.push_back(VMDBase(out[i]));
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
template <typename TYPE>
VMDBase<TYPE>
VMDBase<TYPE>::getNormalVector(const std::vector<VMDBase<TYPE>> &vectors) {
  if (vectors.size() <= 0)
    throw std::invalid_argument(
        "VMDBase::getNormalVector: Must give at least 1 vector");
  size_t nd = vectors[0].getNumDims();
  if (nd < 2)
    throw std::invalid_argument(
        "VMDBase::getNormalVector: Must have at least 2 dimensions!");
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
  double length = normal.normalize();
  if (length == 0)
    throw std::runtime_error("VMDBase::getNormalVector: 0-length normal found. "
                             "Are your vectors collinear?");

  return normal;
}

/// Instantiate VMDBase classes
template MANTID_KERNEL_DLL class VMDBase<double>;
template MANTID_KERNEL_DLL class VMDBase<float>;

} // namespace Mantid
} // namespace Kernel
