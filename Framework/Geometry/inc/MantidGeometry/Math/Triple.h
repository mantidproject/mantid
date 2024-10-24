// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
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
*/

template <typename T> class MANTID_GEOMETRY_DLL Triple {
public:
  T first;  ///< First item
  T second; ///< Second item
  T third;  ///< Third item

  Triple();
  Triple(const Triple<T> &);
  Triple(const T &, const T &, const T &);
  Triple<T> &operator=(const Triple<T> &);
  ~Triple();

  T operator[](const int A) const;
  T &operator[](const int A);
  bool operator<(const Triple<T> &) const;
  bool operator>(const Triple<T> &) const;
  bool operator==(const Triple<T> &) const;
  bool operator!=(const Triple<T> &) const;
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

template <typename F, typename S, typename T> class MANTID_GEOMETRY_DLL DTriple {
public:
  F first;  ///< First item
  S second; ///< Second item
  T third;  ///< Third item

  DTriple();
  DTriple(const DTriple<F, S, T> &);
  DTriple(const F &, const S &, const T &);
  DTriple<F, S, T> &operator=(const DTriple<F, S, T> &);
  ~DTriple();

  bool operator<(const DTriple<F, S, T> &) const;
  bool operator>(const DTriple<F, S, T> &) const;
  bool operator==(const DTriple<F, S, T> &) const;
  bool operator!=(const DTriple<F, S, T> &) const;
};
} // NAMESPACE Mantid
