// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "ui_DataProcessorOptionsDialog.h"
#include <QDialog>
#include <memory>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class DataProcessorView;
class DataProcessorPresenter;

/** QtDataProcessorOptionsDialog : Provides a dialog for setting DataProcessorUI
UI
options.
*/

class DLLExport QtDataProcessorOptionsDialog : public QDialog {
  Q_OBJECT
public:
  QtDataProcessorOptionsDialog(DataProcessorView *view, DataProcessorPresenter *presenter);
  ~QtDataProcessorOptionsDialog() override;

protected:
  void initLayout();
  void initBindings();
protected slots:
  void saveOptions();
  void loadOptions();

protected:
  // the interface
  Ui::DataProcessorOptionsDialog ui;
  // the presenter
  DataProcessorPresenter *m_presenter;
  // maps option names to widget names
  std::map<QString, QString> m_bindings;
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
