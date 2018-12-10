// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_

#include "ui_IndirectFitDataView.h"

#include "../General/UserInputValidator.h"

#include <QTabWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitDataView : public QTabWidget {
  Q_OBJECT
public:
  IndirectFitDataView(QWidget *parent);
  ~IndirectFitDataView() override = default;

  QTableWidget *getDataTable() const;
  bool isMultipleDataTabSelected() const;
  bool isResolutionHidden() const;
  void setResolutionHidden(bool hide);
  void disableMultipleDataTab();

  std::string getSelectedSample() const;
  std::string getSelectedResolution() const;

  QStringList getSampleWSSuffices() const;
  QStringList getSampleFBSuffices() const;
  QStringList getResolutionWSSuffices() const;
  QStringList getResolutionFBSuffices() const;

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void readSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

public slots:
  void displayWarning(const std::string &warning);

signals:
  void sampleLoaded(const QString &);
  void resolutionLoaded(const QString &);
  void addClicked();
  void removeClicked();
  void multipleDataViewSelected();
  void singleDataViewSelected();

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
