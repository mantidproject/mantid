// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
enum class AnalysisMode { PointDetector, MultiDetector };

inline AnalysisMode analysisModeFromString(std::string const &analysisMode) {
  if (analysisMode == "PointDetectorAnalysis")
    return AnalysisMode::PointDetector;
  else if (analysisMode == "MultiDetectorAnalysis")
    return AnalysisMode::MultiDetector;
  else
    throw std::invalid_argument("Unexpected analysis mode.");
}

inline std::string analysisModeToString(AnalysisMode analysisMode) {
  switch (analysisMode) {
  case AnalysisMode::PointDetector:
    return "PointDetectorAnalysis";
  case AnalysisMode::MultiDetector:
    return "MultiDetectorAnalysis";
  }
  throw std::invalid_argument("Unexpected analysis mode");
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
