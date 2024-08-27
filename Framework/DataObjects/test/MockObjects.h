// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeFactory.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

namespace Mantid {
namespace DataObjects {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockPeakShapeFactory : public PeakShapeFactory {
public:
  MOCK_CONST_METHOD1(create, Mantid::Geometry::PeakShape *(const std::string &source));
  MOCK_METHOD1(setSuccessor, void(std::shared_ptr<const PeakShapeFactory> successorFactory));
  ~MockPeakShapeFactory() override = default;
};

class MockPeakShape : public Mantid::Geometry::PeakShape {
public:
  MOCK_CONST_METHOD0(frame, Mantid::Kernel::SpecialCoordinateSystem());
  MOCK_CONST_METHOD0(toJSON, std::string());
  MOCK_CONST_METHOD0(clone, Mantid::Geometry::PeakShape *());
  MOCK_CONST_METHOD0(algorithmName, std::string());
  MOCK_CONST_METHOD0(algorithmVersion, int());
  MOCK_CONST_METHOD0(shapeName, std::string());
  MOCK_CONST_METHOD1(radius, std::optional<double>(Mantid::Geometry::PeakShape::RadiusType));
  ~MockPeakShape() override = default;
};
} // namespace DataObjects
} // namespace Mantid

GNU_DIAG_ON_SUGGEST_OVERRIDE
