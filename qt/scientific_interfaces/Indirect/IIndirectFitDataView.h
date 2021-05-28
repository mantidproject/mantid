// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  virtual ~IIndirectFitDataView() = default;

  virtual QTableWidget *getDataTable() const = 0;
  virtual bool isMultipleDataTabSelected() const = 0;

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
  void startXChanged(double);
  void endXChanged(double);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
