#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_

#include <boost/optional.hpp>

class IEnggDiffCalibrationView {
public:
  virtual void displayLoadedCeriaRunNumber(const std::string &runNumber) = 0;

  virtual void displayLoadedVanadiumRunNumber(const std::string &runNumber) = 0;

  virtual boost::optional<std::string> getInputFilename() const = 0;

  virtual boost::optional<std::string> getNewCalibCeriaRunNumber() const = 0;

  virtual boost::optional<std::string> getNewCalibVanadiumRunNumber() const = 0;

  virtual void userWarning(const std::string &warningTitle,
                           const std::string &warningDescription) = 0;
};

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBRATIONVIEW_H_
