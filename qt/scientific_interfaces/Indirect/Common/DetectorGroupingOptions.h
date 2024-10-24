// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "ui_DetectorGroupingOptions.h"

#include <memory>
#include <optional>
#include <string>

#include <QObject>
#include <QString>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

enum class GroupingMethod { Individual = 0, All = 0, IPF = 0, File = 1, Groups = 2, Custom = 3 };

class DetectorGroupingOptions : public QWidget {
  Q_OBJECT

public:
  DetectorGroupingOptions(QWidget *parent);

  void removeGroupingMethod(std::string const &option);
  void setGroupingMethod(std::string const &option);

  void setSaveCustomVisible(bool const visible);

  std::optional<std::string> validateGroupingProperties(std::size_t const &spectraMin,
                                                        std::size_t const &spectraMax) const;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> groupingProperties() const;

signals:
  void saveCustomGrouping(std::string const &customGrouping);

private slots:
  void handleGroupingMethodChanged(QString const &method);
  void emitSaveCustomGrouping();

private:
  std::string groupingMethod() const;
  std::string groupingFile() const;
  std::string customGrouping() const;
  int nGroups() const;

  int optionIndex(std::string const &option) const;

  Ui::DetectorGroupingWidget m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
