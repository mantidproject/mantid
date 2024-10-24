// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <stdexcept>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
enum class ROIType { Signal, Background, Transmission };

inline ROIType roiTypeFromString(std::string const &roiType) {
  if (roiType == "Signal")
    return ROIType::Signal;
  if (roiType == "Background")
    return ROIType::Background;
  if (roiType == "Transmission")
    return ROIType::Transmission;
  throw std::invalid_argument("Unexpected ROI type");
}

inline std::string roiTypeToString(ROIType roiType) {
  switch (roiType) {
  case ROIType::Signal:
    return "Signal";
  case ROIType::Background:
    return "Background";
  case ROIType::Transmission:
    return "Transmission";
  }
  throw std::invalid_argument("Unexpected ROI type");
}

inline std::string roiTypeToColor(ROIType roiType) {
  switch (roiType) {
  case ROIType::Signal:
    return "#FF00FF";
  case ROIType::Background:
    return "#00FF00";
  case ROIType::Transmission:
    return "#0000FF";
  }
  throw std::invalid_argument("Unexpected ROI type");
}

inline std::string roiTypeToHatch(ROIType roiType) {
  switch (roiType) {
  case ROIType::Signal:
    return "//";
  case ROIType::Background:
    return "\\\\";
  case ROIType::Transmission:
    return "O";
  }
  throw std::invalid_argument("Unexpected ROI type");
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
