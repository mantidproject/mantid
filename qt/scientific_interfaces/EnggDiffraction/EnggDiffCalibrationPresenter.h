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
  std::tuple<std::string, std::string, std::string>
  parseCalibPath(const std::string &path) const;

  void processCalibrate();
  void processLoadCalibration();

  std::unique_ptr<IEnggDiffCalibrationModel> m_model;

  boost::shared_ptr<EnggDiffUserSettings> m_userSettings;
  
  boost::shared_ptr<IEnggDiffCalibrationView> m_view;
  
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFCALIBPRESENTER_H_
