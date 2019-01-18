// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSVIEW_H_

#include "ui_IndirectFitOutputOptions.h"

#include "DllConfig.h"

#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsView
    : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectFitOutputOptionsView(QWidget *parent = nullptr);
  ~IndirectFitOutputOptionsView() override;

  void setAsPlotting(bool plotting);

  void
  setAvailablePlotParameters(std::vector<std::string> const &parameterNames);

  void setPlotEnabled(bool enable);
  void setSaveEnabled(bool enable);

  std::string getPlotType() const;
  void displayWarning(std::string const &message);

signals:
  void plotClicked();
  void saveClicked();

private slots:
  void emitPlotClicked();
  void emitSaveClicked();

private:
  void clearPlotComboBox();
  void setButtonText(QPushButton *button, QString const &text);
  void setButtonsEnabled(bool enable);

  std::unique_ptr<Ui::IndirectFitOutputOptions> m_outputOptions;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
