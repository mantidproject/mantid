#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBPRESENTER_H_

#include "DllConfig.h"
#include "EnggDiffUserSettings.h"
#include "IEnggDiffCalibrationModel.h"
#include "IEnggDiffCalibrationPresenter.h"
#include "IEnggDiffCalibrationView.h"

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffCalibrationPresenter
    : public IEnggDiffCalibrationPresenter {

public:
  EnggDiffCalibrationPresenter(
      std::unique_ptr<IEnggDiffCalibrationModel> model,
      boost::shared_ptr<IEnggDiffCalibrationView> view,
      boost::shared_ptr<EnggDiffUserSettings> userSettings);

  void notify(IEnggDiffCalibrationPresenter::Notification notif) override;

private:
  void processCalibrate();
  void processCalibrateCropped();
  void processLoadCalibration();

  void displayCalibOutput(const GSASCalibrationParameters &calibParams);

  /// Get ceria input from the view. Warn the user and return empty optional if
  /// it is invalid
  boost::optional<std::string> getAndValidateCeriaInput() const;

  /// Get vanadium input from the view. Warn the user and return empty optional
  /// if it is invalid
  boost::optional<std::string> getAndValidateVanadiumInput() const;

  /// Parse calibration filepath and extract instrument name, vanadium run
  /// number and ceria run number
  std::tuple<std::string, std::string, std::string>
  parseCalibPath(const std::string &path) const;

  std::unique_ptr<IEnggDiffCalibrationModel> m_model;

  boost::shared_ptr<EnggDiffUserSettings> m_userSettings;

  boost::shared_ptr<IEnggDiffCalibrationView> m_view;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBPRESENTER_H_
