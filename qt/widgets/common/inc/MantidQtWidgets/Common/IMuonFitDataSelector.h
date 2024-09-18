// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QString>
#include <QStringList>
#include <optional>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Interface for MuonFitDataSelector
 *
 * This abstract base class can be used for mocking purposes
 */
class EXPORT_OPT_MANTIDQT_COMMON IMuonFitDataSelector {
public:
  enum class FitType { Single, CoAdd, Simultaneous };
  virtual ~IMuonFitDataSelector() = default;
  virtual QStringList getFilenames() const = 0;
  virtual double getStartTime() const = 0;
  virtual double getEndTime() const = 0;
  virtual QStringList getPeriodSelections() const = 0;
  virtual void setWorkspaceDetails(const QString &runNumbers, const QString &instName,
                                   const std::optional<QString> &filePath) = 0;
  virtual QStringList getChosenGroups() const = 0;
  virtual void setStartTime(double start) = 0;
  virtual void setEndTime(double end) = 0;
  virtual void setStartTimeQuietly(double start) = 0;
  virtual void setEndTimeQuietly(double end) = 0;
  virtual FitType getFitType() const = 0;
  virtual QString getInstrumentName() const = 0;
  virtual QString getRuns() const = 0;
  virtual QString getSimultaneousFitLabel() const = 0;
  virtual void setSimultaneousFitLabel(const QString &label) = 0;
  virtual int getDatasetIndex() const = 0;
  virtual void setDatasetNames(const QStringList &datasetNames) = 0;
  virtual QString getDatasetName() const = 0;
  virtual bool askUserWhetherToOverwrite() = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
