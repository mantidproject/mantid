// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QObject>
#include <QTabWidget>
#include <QTableWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IIndirectFitDataPresenter;

struct FitDataRow {
  std::string name;
  std::string exclude;
  size_t workspaceIndex;
  double startX;
  double endX;
  std::string resolution;
  std::string parameter;
};

class MANTIDQT_INELASTIC_DLL IIndirectFitDataView : public QTabWidget {
  Q_OBJECT

public:
  IIndirectFitDataView(QWidget *parent = nullptr) : QTabWidget(parent){};
  virtual ~IIndirectFitDataView() = default;

  virtual void subscribePresenter(IIndirectFitDataPresenter *presenter) = 0;

  virtual QTableWidget *getDataTable() const = 0;

  virtual UserInputValidator &validate(UserInputValidator &validator) = 0;
  virtual void addTableEntry(size_t row, FitDataRow newRow) = 0;
  virtual void updateNumCellEntry(double numEntry, size_t row, size_t column) = 0;
  virtual int getColumnIndexFromName(QString ColName) = 0;
  virtual void clearTable() = 0;
  virtual QString getText(int row, int column) const = 0;
  virtual QModelIndexList getSelectedIndexes() const = 0;

  virtual void setSampleWSSuffices(const QStringList &suffices) = 0;
  virtual void setSampleFBSuffices(const QStringList &suffices) = 0;
  virtual void setResolutionWSSuffices(const QStringList &suffices) = 0;
  virtual void setResolutionFBSuffices(const QStringList &suffices) = 0;

public slots:
  virtual void displayWarning(std::string const &warning) = 0;

signals:
  void resolutionLoaded(QString const & /*_t1*/);
  void cellChanged(int, int);
  void addClicked();
  void removeClicked();
  void unifyClicked();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
