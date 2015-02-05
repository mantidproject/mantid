#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/cow_ptr.h"
#include <jsoncpp/json/json.h>

namespace Mantid {
namespace DataObjects {

PeakShapeEllipsoid::PeakShapeEllipsoid(
    std::vector<Kernel::V3D> directions, std::vector<double> abcRadii,
    std::vector<double> abcRadiiBackgroundInner,
    std::vector<double> abcRadiiBackgroundOuter,
    API::SpecialCoordinateSystem frame, std::string algorithmName,
    int algorithmVersion)
    : PeakShapeBase(frame, algorithmName, algorithmVersion),
      m_directions(directions), m_abc_radii(abcRadii),
      m_abc_radiiBackgroundInner(abcRadiiBackgroundInner),
      m_abc_radiiBackgroundOuter(abcRadiiBackgroundOuter) {

  if (directions.size() != 3) {
    throw std::invalid_argument("directions must be of size 3");
  }
  if (abcRadii.size() != 3) {
    throw std::invalid_argument("radii must be of size 3");
  }
  if (abcRadiiBackgroundInner.size() != 3) {
    throw std::invalid_argument("radii inner must be of size 3");
  }
  if (abcRadiiBackgroundOuter.size() != 3) {
    throw std::invalid_argument("radii outer must be of size 3");
  }
}

PeakShapeEllipsoid::PeakShapeEllipsoid(const PeakShapeEllipsoid &other)
    : PeakShapeBase(other), m_directions(other.directions()),
      m_abc_radii(other.abcRadii()),
      m_abc_radiiBackgroundInner(other.abcRadiiBackgroundInner()),
      m_abc_radiiBackgroundOuter(other.abcRadiiBackgroundOuter()) {}

PeakShapeEllipsoid &PeakShapeEllipsoid::
operator=(const PeakShapeEllipsoid &other)  {
  if (&other != this) {
    PeakShapeBase::operator=(other);
    m_directions = other.directions();
    m_abc_radii = other.abcRadii();
    m_abc_radiiBackgroundInner = other.abcRadiiBackgroundInner();
    m_abc_radiiBackgroundOuter = other.abcRadiiBackgroundOuter();
  }
  return *this;
}

bool PeakShapeEllipsoid::operator==(const PeakShapeEllipsoid &other) const {
  return PeakShapeBase::operator==(other) &&
         other.directions() == this->directions() &&
         other.abcRadii() == this->abcRadii() &&
         other.abcRadiiBackgroundInner() == this->abcRadiiBackgroundInner() &&
         other.abcRadiiBackgroundOuter() == this->abcRadiiBackgroundOuter();
}

PeakShapeEllipsoid::~PeakShapeEllipsoid() {}

std::vector<double> PeakShapeEllipsoid::abcRadii() const { return m_abc_radii; }

std::vector<double> PeakShapeEllipsoid::abcRadiiBackgroundInner() const {
  return m_abc_radiiBackgroundInner;
}

std::vector<double> PeakShapeEllipsoid::abcRadiiBackgroundOuter() const {
  return m_abc_radiiBackgroundOuter;
}

std::vector<Kernel::V3D> PeakShapeEllipsoid::directions() const {
  return m_directions;
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

  Json::StyledWriter writer;
  return writer.write(root);
}

PeakShape *PeakShapeEllipsoid::clone() const {
  return new PeakShapeEllipsoid(*this);
}

std::string PeakShapeEllipsoid::shapeName() const { return "ellipsoid"; }

} // namespace DataObjects
} // namespace Mantid
