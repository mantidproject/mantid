// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QTOPTIONSDIALOGVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTOPTIONSDIALOGVIEW_H

#include "IOptionsDialogView.h"

#include "Common/DllConfig.h"
#include "ui_QtOptionsDialogView.h"
#include <QDialog>
#include <QVariant>
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/// OptionsDialog : Provides a dialog for setting options.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtOptionsDialogView
    : public QDialog,
      public IOptionsDialogView {
  Q_OBJECT
public:
  explicit QtOptionsDialogView(QWidget *parent);
  ~QtOptionsDialogView() override;
  void getOptions(std::map<std::string, bool> &boolOptions,
                  std::map<std::string, int> &intOptions) override;
  void setOptions(std::map<std::string, bool> &boolOptions,
                  std::map<std::string, int> &intOptions) override;
  void show() override;
  void subscribe(OptionsDialogViewSubscriber *notifyee) override;

public slots:
  void onLoadOptions();
  void onSaveOptions();
  void closeEvent(QCloseEvent *event) override;

private:
  void initLayout();
  void initBindings();

private:
  Ui::QtOptionsDialogView m_ui;
  // subscribe updates from view
  OptionsDialogViewSubscriber *m_notifyee;
  // maps option names to widget names
  std::map<QString, QString> m_bindings;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_QTOPTIONSDIALOGVIEW_H */
