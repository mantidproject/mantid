#ifndef MANTID_CURVEFITTING_FORTRANVECTOR_H_
#define MANTID_CURVEFITTING_FORTRANVECTOR_H_

#include <utility>

namespace Mantid {
namespace CurveFitting {

/** FortranVector is a wrapper template for GSLVactor and ComplexVector
  to simplify porting fortran programs to C++.
  This vector allows to use arbitrary index bases as they do in
  fortran. Indexing can begin with any integer number including
  negative.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
template <class VectorClass> class FortranVector : public VectorClass {
  /// Base for the index
  int m_base;
  /// Typedef the types returned by the base class's operators []. They aren't
  /// necessarily the same as the stored type (double or complex).
  using ElementConstType = decltype(std::declval<const VectorClass>()[0]);
  using ElementRefType = decltype(std::declval<VectorClass>()[0]);

public:
  /// Constructor
  FortranVector();
  /// Constructor
  explicit FortranVector(const int n);
  /// Constructor
  FortranVector(const int iFrom, const int iTo);
  /// Resize the vector
  void allocate(int firstIndex, int lastIndex);
  /// Resize the vector with base 1
  void allocate(int newSize);
  /// Get the length of the vector as an int.
  int len() const;

  ElementConstType operator()(int i) const;
  ElementRefType operator()(int i);
  ElementConstType operator[](int i) const;
  ElementRefType operator[](int i);

  VectorClass moveToBaseVector();

private:
  /// Calculate the size (1D) of a matrix First
  static size_t makeSize(int firstIndex, int lastIndex);
};

/// Calculate the size of a vector
template <class VectorClass>
size_t FortranVector<VectorClass>::makeSize(int firstIndex, int lastIndex) {
  if (lastIndex < firstIndex) {
    throw std::invalid_argument("Vector defined with invalid index range.");
  }
  return static_cast<size_t>(lastIndex - firstIndex + 1);
}

/// Constructor
template <class VectorClass>
FortranVector<VectorClass>::FortranVector()
    : VectorClass(makeSize(1, 1)), m_base(1) {}

/// Constructor
template <class VectorClass>
FortranVector<VectorClass>::FortranVector(const int n)
    : VectorClass(makeSize(1, n)), m_base(1) {}

/// Construct a FortranVector that has arbitrary index bases.
/// For example FortranVector(-2,2) creates a vector of length 5.
/// When accessing elements through operator(i) the index
/// must be in the range -2 <= j <= 2.
/// The index ranges defined by this constructor apply only to
/// operators () and [] but not to methods get() and set().
/// @param iFirst :: Lowest value for the index
/// @param iLast :: Highest value for the index
template <class VectorClass>
FortranVector<VectorClass>::FortranVector(const int iFirst, const int iLast)
    : VectorClass(makeSize(iFirst, iLast)), m_base(iFirst) {}

/// Resize the vector. Named this way to mimic the fortran style and to
/// avoid confusion with resize() method of the base class.
/// @param iFirst :: Lowest value for the index
/// @param iLast :: Highest value for the index
template <class VectorClass>
void FortranVector<VectorClass>::allocate(int iFirst, int iLast) {
  m_base = iFirst;
  this->resize(makeSize(iFirst, iLast));
}

/// Resize the vector. Named this way to mimic the fortran style and to
/// avoid confusion with resize() method of the base class.
/// @param newSize :: The new size of the vector. Index base is set to 1.
template <class VectorClass>
void FortranVector<VectorClass>::allocate(int newSize) {
  m_base = 1;
  this->resize(makeSize(1, newSize));
}

/// The "index" operator
template <class VectorClass>
typename FortranVector<VectorClass>::ElementConstType
    FortranVector<VectorClass>::
    operator()(int i) const {
  return this->VectorClass::operator[](static_cast<size_t>(i - m_base));
}

/// Get the reference to the data element
template <class VectorClass>
typename FortranVector<VectorClass>::ElementRefType FortranVector<VectorClass>::
operator()(int i) {
  return this->VectorClass::operator[](static_cast<size_t>(i - m_base));
}

/// The "index" operator
template <class VectorClass>
typename FortranVector<VectorClass>::ElementConstType
    FortranVector<VectorClass>::
    operator[](int i) const {
  return this->VectorClass::operator[](static_cast<size_t>(i - m_base));
}

/// Get the reference to the data element
template <class VectorClass>
typename FortranVector<VectorClass>::ElementRefType FortranVector<VectorClass>::
operator[](int i) {
  return this->VectorClass::operator[](static_cast<size_t>(i - m_base));
}

/// Move the data of this vector to a newly created vector of the bas class.
/// Do not use this vector after calling this method. The intension of it is
/// to keep fortran-style calculations separate from C++-style.
template <class VectorClass>
VectorClass FortranVector<VectorClass>::moveToBaseVector() {
  return VectorClass::move();
}

/// Get the length of the vector as an int.
template <class VectorClass> int FortranVector<VectorClass>::len() const {
  return static_cast<int>(this->size());
}

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FORTRANVECTOR_H_ */
