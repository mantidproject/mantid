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
  bool isResolutionHidden() const override;
  void setResolutionHidden(bool hide) override;
  void disableMultipleDataTab() override;

  virtual std::string getSelectedSample() const override;
  std::string getSelectedResolution() const override;

  virtual QStringList getSampleWSSuffixes() const override;
  virtual QStringList getSampleFBSuffixes() const override;
  QStringList getResolutionWSSuffixes() const override;
  QStringList getResolutionFBSuffixes() const override;

  virtual void setSampleWSSuffixes(const QStringList &suffices) override;
  virtual void setSampleFBSuffixes(const QStringList &suffices) override;
  virtual void setResolutionWSSuffixes(const QStringList &suffices) override;
  virtual void setResolutionFBSuffixes(const QStringList &suffices) override;

  bool isSampleWorkspaceSelectorVisible() const override;
  void setSampleWorkspaceSelectorIndex(const QString &workspaceName) override;

  void readSettings(const QSettings &settings) override;
  UserInputValidator &validate(UserInputValidator &validator) override;

  void setXRange(std::pair<double, double> const &range) override;
  std::pair<double, double> getXRange() const override;
  QComboBox *cbParameterType;
  QComboBox *cbParameter;
  QLabel *lbParameter;
  QLabel *lbParameterType;

public slots:
  void displayWarning(const std::string &warning) override;

protected slots:
  void emitViewSelected(int index);
  void setStartX(double) override;
  void setEndX(double) override;

private:
  UserInputValidator &validateMultipleData(UserInputValidator &validator);
  UserInputValidator &validateSingleData(UserInputValidator &validator);
  UserInputValidator &validateSample(UserInputValidator &validator);
  UserInputValidator &validateResolution(UserInputValidator &validator);

  std::unique_ptr<Ui::IndirectFitDataForm> m_dataForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
