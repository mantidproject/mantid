#ifndef MANTID_CUSTOMINTERFACES_ANALYSISMODE_H_
#define MANTID_CUSTOMINTERFACES_ANALYSISMODE_H_
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
enum class AnalysisMode { PointDetector, MultiDetector };

inline AnalysisMode analysisModeFromString(std::string const& analysisMode) {
  if (analysisMode == "PointDetectorAnalysis")
    return AnalysisMode::PointDetector;
  else if (analysisMode == "MultiDetectorAnalysis")
    return AnalysisMode::MultiDetector;
  else
    throw std::runtime_error("Unexpected analysis mode.");
}
}
}
#endif // MANTID_CUSTOMINTERFACES_ANALYSISMODE_H_
