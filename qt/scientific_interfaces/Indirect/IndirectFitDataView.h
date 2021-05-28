// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_IndirectFitDataView.h"

#include "IIndirectFitDataView.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QTabWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitDataView : public IIndirectFitDataView {
  Q_OBJECT
public:
  IndirectFitDataView(QWidget *parent = nullptr);
  ~IndirectFitDataView() override = default;

  QTableWidget *getDataTable() const override;
  virtual bool isMultipleDataTabSelected() const override;

  UserInputValidator &validate(UserInputValidator &validator) override;

public slots:
  void displayWarning(const std::string &warning) override;

protected slots:
  void emitViewSelected(int index);

private:
  UserInputValidator &validateMultipleData(UserInputValidator &validator);

  std::unique_ptr<Ui::IndirectFitDataForm> m_dataForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
