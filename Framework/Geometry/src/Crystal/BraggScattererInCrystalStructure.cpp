// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/BraggScattererInCrystalStructure.h"
#include <stdexcept>

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidKernel/ListValidator.h"
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4005)
#endif

#include <muParser.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Default constructor.
BraggScattererInCrystalStructure::BraggScattererInCrystalStructure()
    : BraggScatterer(), m_position(), m_cell(UnitCell(1, 1, 1, 90, 90, 90)) {}

/// Sets the position of the scatterer to the supplied coordinates - vector is
/// wrapped to [0, 1).
void BraggScattererInCrystalStructure::setPosition(
    const Kernel::V3D &position) {
  m_position = getWrappedVector(position);
}

/// Returns the position of the scatterer.
Kernel::V3D BraggScattererInCrystalStructure::getPosition() const {
  return m_position;
}

/// Returns the cell which is currently set.
UnitCell BraggScattererInCrystalStructure::getCell() const { return m_cell; }

/// Assigns a unit cell, which may be required for certain calculations.
void BraggScattererInCrystalStructure::setCell(const UnitCell &cell) {
  m_cell = cell;
}

/// Declares basic properties, should not be overridden by subclasses, use
/// declareScattererProperties instead.
void BraggScattererInCrystalStructure::declareProperties() {
  declareProperty(std::make_unique<Kernel::PropertyWithValue<std::string>>(
                      "Position", "[0, 0, 0]"),
                  "Position of the scatterer");

  IValidator_sptr unitCellStringValidator =
      boost::make_shared<UnitCellStringValidator>();
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<std::string>>(
          "UnitCell", "1.0 1.0 1.0 90.0 90.0 90.0", unitCellStringValidator),
      "Unit cell.");
  exposePropertyToComposite("UnitCell");

  declareScattererProperties();
}

V3D BraggScattererInCrystalStructure::getPositionFromString(
    const std::string &positionString) const {
  std::vector<std::string> numberParts =
      getTokenizedPositionString(positionString);

  mu::Parser parser;

  V3D position;
  for (size_t i = 0; i < numberParts.size(); ++i) {
    parser.SetExpr(numberParts[i]);
    position[i] = parser.Eval();
  }

  return position;
}

/**
 * Additional property processing
 *
 * Takes care of handling new property values.
 *
 * Please note that derived classes should not re-implement this method, as
 * the processing for the base properties is absolutely necessary. Instead, all
 * deriving classes should override the method afterScattererPropertySet,
 * which is called from this method.
 */
void BraggScattererInCrystalStructure::afterPropertySet(
    const std::string &propertyName) {
  if (propertyName == "Position") {
    std::string position = getProperty("Position");

    setPosition(getPositionFromString(position));
  } else if (propertyName == "UnitCell") {
    setCell(strToUnitCell(getProperty("UnitCell")));
  }

  afterScattererPropertySet(propertyName);
}

/// Return a clone of the validator.
IValidator_sptr UnitCellStringValidator::clone() const {
  return boost::make_shared<UnitCellStringValidator>(*this);
}

/// Check if the string is valid input for Geometry::strToUnitCell.
std::string UnitCellStringValidator::checkValidity(
    const std::string &unitCellString) const {
  boost::regex unitCellRegex("((\\d+(\\.\\d+){0,1}\\s+){2}|(\\d+(\\.\\d+){0,1}"
                             "\\s+){5})(\\d+(\\.\\d+){0,1}\\s*)");

  if (!boost::regex_match(unitCellString, unitCellRegex)) {
    return "Unit cell string is invalid: " + unitCellString;
  }

  return "";
}

/// Returns components of comma-separated position string, cleaned from [ and ].
std::vector<std::string>
getTokenizedPositionString(const std::string &position) {
  std::string positionStringClean = position;
  positionStringClean.erase(std::remove_if(positionStringClean.begin(),
                                           positionStringClean.end(),
                                           boost::is_any_of("[]")),
                            positionStringClean.end());

  std::vector<std::string> numberParts;
  boost::split(numberParts, positionStringClean, boost::is_any_of(","));

  if (numberParts.size() != 3) {
    throw std::invalid_argument("Cannot parse '" + position +
                                "' as a position.");
  }

  return numberParts;
}

} // namespace Geometry
} // namespace Mantid
