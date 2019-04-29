// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITDATAVIEW_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITDATAVIEW_H_

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QObject>
#include <QTabWidget>
#include <QTableWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IIndirectFitDataView : public QTabWidget {
  Q_OBJECT

public:
  IIndirectFitDataView(QWidget *parent = nullptr) : QTabWidget(parent){};
  virtual ~IIndirectFitDataView(){};

  virtual QTableWidget *getDataTable() const = 0;
  virtual bool isMultipleDataTabSelected() const = 0;
  virtual bool isResolutionHidden() const = 0;
  virtual void setResolutionHidden(bool hide) = 0;
  virtual void disableMultipleDataTab() = 0;

  virtual std::string getSelectedSample() const = 0;
  virtual std::string getSelectedResolution() const = 0;

  virtual QStringList getSampleWSSuffices() const = 0;
  virtual QStringList getSampleFBSuffices() const = 0;
  virtual QStringList getResolutionWSSuffices() const = 0;
  virtual QStringList getResolutionFBSuffices() const = 0;

  virtual void setSampleWSSuffices(QStringList const &suffices) = 0;
  virtual void setSampleFBSuffices(QStringList const &suffices) = 0;
  virtual void setResolutionWSSuffices(QStringList const &suffices) = 0;
  virtual void setResolutionFBSuffices(QStringList const &suffices) = 0;

  virtual void readSettings(QSettings const &settings) = 0;
  virtual UserInputValidator &validate(UserInputValidator &validator) = 0;

public slots:
  virtual void displayWarning(std::string const &warning) = 0;

signals:
  void sampleLoaded(QString const & /*_t1*/);
  void resolutionLoaded(QString const & /*_t1*/);
  void addClicked();
  void removeClicked();
  void multipleDataViewSelected();
  void singleDataViewSelected();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
