// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTSETTINGSVIEW_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTSETTINGSVIEW_H_

#include "DllConfig.h"

#include <QDialog>
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IIndirectSettingsView : public QDialog {
  Q_OBJECT

public:
  IIndirectSettingsView(QWidget *parent = nullptr) : QDialog(parent){};
  //virtual ~IIndirectSettingsView(){};

  virtual void setInterfaceSettingsVisible(bool visible) = 0;
  virtual void setInterfaceGroupBoxTitle(QString const &title) = 0;

  virtual void setRestrictInputByNameVisible(bool visible) = 0;
  virtual void setPlotErrorBarsVisible(bool visible) = 0;

  virtual void setSelectedFacility(QString const &text) = 0;
  virtual QString getSelectedFacility() const = 0;

  virtual void setRestrictInputByNameChecked(bool check) = 0;
  virtual bool isRestrictInputByNameChecked() const = 0;

  virtual void setPlotErrorBarsChecked(bool check) = 0;
  virtual bool isPlotErrorBarsChecked() const = 0;

  virtual void setSetting(QString const &settingsGroup,
                          QString const &settingName, bool const &value) = 0;
  virtual QVariant getSetting(QString const &settingsGroup,
                              QString const &settingName) = 0;

  virtual void setApplyText(QString const &text) = 0;
  virtual void setApplyEnabled(bool enable) = 0;
  virtual void setOkEnabled(bool enable) = 0;
  virtual void setCancelEnabled(bool enable) = 0;

signals:
  void updateRestrictInputByName(std::string const &text);
  void okClicked();
  void applyClicked();
  void cancelClicked();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTSETTINGSVIEW_H_ */
