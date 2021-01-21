// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/CoordTransformAffineParser.h"
#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace DataObjects {
/// Forward declaration
class CoordTransformDistance;

/** A parser for processing coordinate transform xml
 *
 * @author Owen Arnold
 * @date 25/july/2011
 */
class DLLExport CoordTransformDistanceParser : public CoordTransformAffineParser {
public:
  CoordTransformDistanceParser();
  Mantid::API::CoordTransform *createTransform(Poco::XML::Element *coordTransElement) const override;

private:
  CoordTransformDistanceParser(const CoordTransformDistanceParser &);
  CoordTransformDistanceParser &operator=(const CoordTransformDistanceParser &);
};
} // namespace DataObjects
} // namespace Mantid
