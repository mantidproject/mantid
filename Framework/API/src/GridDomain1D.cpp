// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <numeric>

#include "MantidAPI/GridDomain1D.h"
#include "MantidAPI/ITransformScale.h"
#include "MantidAPI/TransformScaleFactory.h"

namespace Mantid {
namespace API {

void GridDomain1D::initialize(double &startX, double &endX, size_t &n, const std::string &scaling) {
  m_points.resize(n);
  m_points.front() = startX;
  m_points.back() = endX;
  this->reScale(scaling);
}

void GridDomain1D::reScale(const std::string &scaling) {
  ITransformScale_sptr fx = Mantid::API::TransformScaleFactory::Instance().create(scaling);
  fx->transform(m_points);
  m_scaling = scaling;
}

} // namespace API
} // namespace Mantid
