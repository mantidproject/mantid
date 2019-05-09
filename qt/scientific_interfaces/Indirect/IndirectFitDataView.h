// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_

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

  virtual QStringList getSampleWSSuffices() const override;
  virtual QStringList getSampleFBSuffices() const override;
  QStringList getResolutionWSSuffices() const override;
  QStringList getResolutionFBSuffices() const override;

  virtual void setSampleWSSuffices(const QStringList &suffices) override;
  virtual void setSampleFBSuffices(const QStringList &suffices) override;
  virtual void setResolutionWSSuffices(const QStringList &suffices) override;
  virtual void setResolutionFBSuffices(const QStringList &suffices) override;

  bool isSampleWorkspaceSelectorVisible() const override;
  void setSampleWorkspaceSelectorIndex(const QString &workspaceName) override;

  void readSettings(const QSettings &settings) override;
  UserInputValidator &validate(UserInputValidator &validator) override;

public slots:
  void displayWarning(const std::string &warning) override;

protected slots:
  void emitViewSelected(int index);

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

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_ */
