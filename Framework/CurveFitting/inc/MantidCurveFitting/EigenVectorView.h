// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidCurveFitting/DllConfig.h"

#include <Eigen/Core>

namespace Mantid::CurveFitting {
typedef Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic> dynamic_stride;
typedef Eigen::Map<Eigen::VectorXd, 0, dynamic_stride> vec_map_type;
typedef Eigen::Map<const Eigen::VectorXd, 0, dynamic_stride> vec_const_map_type;

class MANTID_CURVEFITTING_DLL EigenVector_View {
public:
  // EigenVector_View Constructors
  // default constructor
  EigenVector_View();

  /// constructor: map->vector view
  EigenVector_View(vec_map_type &vector, const int nElements = -1, const size_t startElement = 0);

  // constructor: vector->vector view
  EigenVector_View(Eigen::VectorXd &vector, const int nElements = -1, const size_t startElement = 0);

  // constructor: array->vector view
  EigenVector_View(double *base, const size_t nElements, const size_t startElement = 0);

  /// CONST constructor: map->vector view
  EigenVector_View(const vec_map_type &vector, const size_t nElements = -1, const size_t startElement = 0);

  // CONST constructor: vector->vector view
  EigenVector_View(const Eigen::VectorXd &vector, const size_t nElements, const size_t startElement = 0);

  // CONST constructor: array->vector view
  EigenVector_View(const double *base, const size_t nElements, const size_t startElement = 0);

  // copy constructor
  EigenVector_View(EigenVector_View &v);

  // CONST copy constructor
  EigenVector_View(const EigenVector_View &v);

  vec_map_type &vector_mutator();
  inline const vec_map_type vector_inspector() const { return m_view; }
  inline vec_map_type vector_copy() const { return m_view; }
  inline size_t size() const { return m_view.size(); }

  EigenVector_View &operator=(EigenVector_View &V);
  EigenVector_View &operator=(EigenVector_View &&V);

protected:
  vec_map_type m_view;
  bool m_isConst = false;
};
} // namespace Mantid::CurveFitting
