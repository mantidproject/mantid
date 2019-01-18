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

	void clearPlotTypes();
  void
  setAvailablePlotTypes(std::vector<std::string> const &parameterNames);
  void setPlotTypeIndex(int index);
  std::string getSelectedPlotType() const;

	void setPlotText(QString const &text);
  void setSaveText(QString const &text);

  void setPlotEnabled(bool enable);
  void setSaveEnabled(bool enable);

  void displayWarning(std::string const &message);

signals:
  void plotClicked();
  void saveClicked();

private slots:
  void emitPlotClicked();
  void emitSaveClicked();

private:
  std::unique_ptr<Ui::IndirectFitOutputOptions> m_outputOptions;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
