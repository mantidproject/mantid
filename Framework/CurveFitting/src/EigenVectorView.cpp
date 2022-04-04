// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidCurveFitting/EigenVectorView.h"

namespace Mantid::CurveFitting {
/// EigenVector_View Constructors
/// default constructor
EigenVector_View::EigenVector_View() : m_view({}, 0, dynamic_stride(0, 0)) {}

// constructor: map->vector view
EigenVector_View::EigenVector_View(vec_map_type &vector, const size_t nElements, const size_t &startElement)
    : m_view(vector.data(), vector.size(), dynamic_stride(0, 1)) {
  if (nElements == -1) {
    // if nElements is default, do nothing as m_view is initialised as such
  } else {
    new (&m_view) vec_map_type(vector.data() + startElement, nElements, dynamic_stride(0, 1));
  }
};

/// constructor: vector->vector view
/// @param vector :: Eigen::VectorXd from which to take view.
/// @param startElement :: The first element of the view.
/// @param nElements :: The number of elements to view.
EigenVector_View::EigenVector_View(Eigen::VectorXd &vector, const size_t nElements, const size_t &startElement)
    : m_view(vector.data(), vector.size(), dynamic_stride(0, 1)) {
  if (nElements == -1) {
    // if nElements is default, do nothing as m_view is initialised as such
  } else {
    new (&m_view) vec_map_type(vector.data() + startElement, nElements, dynamic_stride(0, 1));
  }
};
// constructor: array->vector view
/// @param array :: array from which to take view.
/// @param startElement :: The first element of the view.
/// @param nElements :: The number of elements to view.
EigenVector_View::EigenVector_View(double *base, const size_t nElements, const size_t &startElement)
    : m_view(base + startElement, nElements, dynamic_stride(0, 1)) {}

// CONST constructor: map->vector view
EigenVector_View::EigenVector_View(const vec_map_type &vector, const size_t nElements, const size_t &startElement)
    : m_view({}, 0, dynamic_stride(0, 0)) {
  new (&m_view) vec_const_map_type(vector.data() + startElement, nElements, dynamic_stride(0, 1));
};

/// CONST constructor: vector->vector view
/// @param vector :: Eigen::VectorXd from which to take view.
/// @param startElement :: The first element of the view.
/// @param nElements :: The number of elements to view.
EigenVector_View::EigenVector_View(const Eigen::VectorXd &vector, const size_t nElements, const size_t &startElement)
    : m_view({}, 0, dynamic_stride(0, 0)), isConst(true) {
  new (&m_view) vec_const_map_type(vector.data() + startElement, nElements, dynamic_stride(0, 1));
};

/// CONST constructor: array->vector view
/// @param array :: array from which to take view.
/// @param startElement :: The first element of the view.
/// @param nElements :: The number of elements to view.
EigenVector_View::EigenVector_View(const double *base, const size_t nElements, const size_t &startElement)
    : m_view({}, 0, dynamic_stride(0, 0)), isConst(true) {
  new (&m_view) vec_const_map_type(base + startElement, nElements, dynamic_stride(0, 1));
}

/// CONST copy constructor
EigenVector_View::EigenVector_View(const EigenVector_View &v) : m_view({}, 0, dynamic_stride(0, 0)), isConst(true) {
  new (&m_view) vec_const_map_type(v.vector_inspector().data(), v.size(), dynamic_stride(0, 1));
}

/// copy constructor
EigenVector_View::EigenVector_View(EigenVector_View &v)
    : m_view(v.vector_mutator().data(), v.size(), dynamic_stride(0, 1)), isConst(false) {}

vec_map_type &EigenVector_View::vector_mutator() {
  if (!isConst) {
    return m_view;
  } else {
    throw std::runtime_error("Vector is const vector, cannot mutate const vector.");
  }
}

EigenVector_View &EigenVector_View::operator=(EigenVector_View &V) {

  new (&m_view)
      vec_map_type(V.m_view.data(), V.m_view.size(), dynamic_stride(V.m_view.outerStride(), V.m_view.innerStride()));

  return *this;
}

EigenVector_View &EigenVector_View::operator=(EigenVector_View &&V) {

  new (&m_view)
      vec_map_type(V.m_view.data(), V.m_view.size(), dynamic_stride(V.m_view.outerStride(), V.m_view.innerStride()));

  return *this;
}
} // namespace Mantid::CurveFitting