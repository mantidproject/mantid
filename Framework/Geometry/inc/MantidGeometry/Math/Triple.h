#ifndef Triple_h
#define Triple_h
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
/**
  \class Triple
  \brief Triple of three identical types
  \author S. Ansell
  \date April 2005
  \version 1.0

  Class maintians a type first/second/third triple
  similar to std::pair except all are identical

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/

template <typename T> class MANTID_GEOMETRY_DLL Triple {
public:
  T first;  ///< First item
  T second; ///< Second item
  T third;  ///< Third item

  Triple();
  Triple(const Triple<T> & /*A*/);
  Triple(const T & /*A*/, const T & /*B*/, const T & /*C*/);
  Triple<T> &operator=(const Triple<T> & /*A*/);
  ~Triple();

  T operator[](const int A) const;
  T &operator[](const int A);
  int operator<(const Triple<T> & /*A*/) const;
  int operator>(const Triple<T> & /*A*/) const;
  int operator==(const Triple<T> & /*A*/) const;
  int operator!=(const Triple<T> & /*A*/) const;
};

/**
  \class DTriple
  \brief Triple of three different things
  \author S. Ansell
  \date April 2005
  \version 1.0

  Class maintians a different type first/second/third triple
  All are of a different type
*/

template <typename F, typename S, typename T>
class MANTID_GEOMETRY_DLL DTriple {
public:
  F first;  ///< First item
  S second; ///< Second item
  T third;  ///< Third item

  DTriple();
  DTriple(const DTriple<F, S, T> & /*A*/);
  DTriple(const F & /*A*/, const S & /*B*/, const T & /*C*/);
  DTriple<F, S, T> &operator=(const DTriple<F, S, T> & /*A*/);
  ~DTriple();

  int operator<(const DTriple<F, S, T> & /*A*/) const;
  int operator>(const DTriple<F, S, T> & /*A*/) const;
  int operator==(const DTriple<F, S, T> & /*A*/) const;
  int operator!=(const DTriple<F, S, T> & /*A*/) const;
};
} // NAMESPACE Mantid
#endif
