// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_LIMITS_HANDLER_H
#define REF_LIMITS_HANDLER_H

#include "ui_RefImageView.h"

namespace MantidQt {
namespace RefDetectorViewer {
/** Retrieves the states of the peak/background/TOF limit settings from the gui
 */

class RefLimitsHandler {
public:
  /// Construct object to manage range (peak/back/TOF) controls in the UI
  RefLimitsHandler(Ui_RefImageViewer *ivUI);

  /// get peak, back and tof values
  int getPeakLeft() const;
  int getPeakRight() const;
  int getBackLeft() const;
  int getBackRight() const;
  int getTOFmin() const;
  int getTOFmax() const;

  void setPeakLeft(const int value);
  void setPeakRight(const int value);
  void setBackLeft(const int value);
  void setBackRight(const int value);
  void setTOFmin(const int value);
  void setTOFmax(const int value);

  void setActiveValue(const double x, const double y);

private:
  const Ui_RefImageViewer *const m_ui;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_LIMITS_HANDLER_H
