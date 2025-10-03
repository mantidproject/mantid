// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidJson/Json.h"
#include "MantidKernel/cow_ptr.h"
#include <json/json.h>

#include <utility>

namespace Mantid::DataObjects {

PeakShapeEllipsoid::PeakShapeEllipsoid(const std::array<Kernel::V3D, 3> &directions,
                                       const PeakEllipsoidExtent &abcRadii,
                                       const PeakEllipsoidExtent &abcRadiiBackgroundInner,
                                       const PeakEllipsoidExtent &abcRadiiBackgroundOuter,
                                       Kernel::SpecialCoordinateSystem frame, std::string algorithmName,
                                       int algorithmVersion, const Kernel::V3D &translation)
    : PeakShapeBase(frame, std::move(algorithmName), algorithmVersion), m_directions(directions), m_abc_radii(abcRadii),
      m_abc_radiiBackgroundInner(abcRadiiBackgroundInner), m_abc_radiiBackgroundOuter(abcRadiiBackgroundOuter),
      m_translation(translation) {}

bool PeakShapeEllipsoid::operator==(const PeakShapeEllipsoid &other) const {
  return PeakShapeBase::operator==(other) && other.directions() == this->directions() &&
         other.abcRadii() == this->abcRadii() && other.abcRadiiBackgroundInner() == this->abcRadiiBackgroundInner() &&
         other.abcRadiiBackgroundOuter() == this->abcRadiiBackgroundOuter() &&
         other.translation() == this->translation();
}

const PeakEllipsoidExtent &PeakShapeEllipsoid::abcRadii() const { return m_abc_radii; }

const PeakEllipsoidExtent &PeakShapeEllipsoid::abcRadiiBackgroundInner() const { return m_abc_radiiBackgroundInner; }

const PeakEllipsoidExtent &PeakShapeEllipsoid::abcRadiiBackgroundOuter() const { return m_abc_radiiBackgroundOuter; }

const std::array<Kernel::V3D, 3> &PeakShapeEllipsoid::directions() const { return m_directions; }

const Kernel::V3D &PeakShapeEllipsoid::translation() const { return m_translation; }

std::array<Kernel::V3D, 3>
PeakShapeEllipsoid::getDirectionInSpecificFrame(Kernel::Matrix<double> &invertedGoniometerMatrix) const {

  if ((invertedGoniometerMatrix.numCols() != m_directions.size()) ||
      (invertedGoniometerMatrix.numRows() != m_directions.size())) {
    throw std::invalid_argument("The inverted goniometer matrix is not "
                                "compatible with the direction vector");
  }

  std::array<Kernel::V3D, 3> directionsInFrame;
  std::transform(m_directions.cbegin(), m_directions.cend(), directionsInFrame.begin(),
                 [&invertedGoniometerMatrix](const auto &direction) { return invertedGoniometerMatrix * direction; });
  return directionsInFrame;
}

std::string PeakShapeEllipsoid::toJSON() const {
  Json::Value root;
  PeakShapeBase::buildCommon(root);
  root["radius0"] = Json::Value(m_abc_radii[0]);
  root["radius1"] = Json::Value(m_abc_radii[1]);
  root["radius2"] = Json::Value(m_abc_radii[2]);

  root["background_inner_radius0"] = Json::Value(m_abc_radiiBackgroundInner[0]);
  root["background_inner_radius1"] = Json::Value(m_abc_radiiBackgroundInner[1]);
  root["background_inner_radius2"] = Json::Value(m_abc_radiiBackgroundInner[2]);

  root["background_outer_radius0"] = Json::Value(m_abc_radiiBackgroundOuter[0]);
  root["background_outer_radius1"] = Json::Value(m_abc_radiiBackgroundOuter[1]);
  root["background_outer_radius2"] = Json::Value(m_abc_radiiBackgroundOuter[2]);

  root["direction0"] = m_directions[0].toString();
  root["direction1"] = m_directions[1].toString();
  root["direction2"] = m_directions[2].toString();

  root["translation0"] = Json::Value(m_translation[0]);
  root["translation1"] = Json::Value(m_translation[1]);
  root["translation2"] = Json::Value(m_translation[2]);

  return Mantid::JsonHelpers::jsonToString(root);
}

PeakShapeEllipsoid *PeakShapeEllipsoid::clone() const { return new PeakShapeEllipsoid(*this); }

std::string PeakShapeEllipsoid::shapeName() const { return PeakShapeEllipsoid::ellipsoidShapeName(); }

std::optional<double> PeakShapeEllipsoid::radius(RadiusType type) const {
  std::optional<double> radius;

  switch (type) {
  case (RadiusType::Radius):
    radius = *(std::max_element(m_abc_radii.cbegin(), m_abc_radii.cend()));
    break;
  case (RadiusType::OuterRadius):
    radius = *(std::max_element(m_abc_radiiBackgroundOuter.cbegin(), m_abc_radiiBackgroundOuter.cend()));
    break;
  case (RadiusType::InnerRadius):
    radius = *(std::max_element(m_abc_radiiBackgroundInner.cbegin(), m_abc_radiiBackgroundInner.cend()));
    break;
  }
  return radius;
}

const std::string PeakShapeEllipsoid::ellipsoidShapeName() { return "ellipsoid"; }

} // namespace Mantid::DataObjects
