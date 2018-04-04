#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffCalibrationPresenter {
public:
  virtual ~IEnggDiffCalibrationPresenter() = default;

  enum class Notification {
    LoadCalibration          ///< Load a calibration from file
  };

  virtual void
  notify(Notification notif) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFCALIBPRESENTER_H_
