// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_

#include "IndirectToolsTab.h"
#include "ui_IndirectLoadILL.h"

#include <QMap>
#include <QSettings>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport IndirectLoadILL : public IndirectToolsTab {
  Q_OBJECT

public:
  IndirectLoadILL(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

protected:
  void setup() override;
  bool validate() override;
  void run() override;

private slots:
  /// Set the instrument based on the file name if possible
  void handleFilesFound();
  void runClicked();

private:
  void loadILLData(std::string const &filename, std::string const &outputName);

  void setRunIsRunning(bool running);
  void setRunEnabled(bool enabled);
  void setPlotOptionsEnabled(bool enabled);

  /// Map to store instrument analysers and reflections for this instrument
  QMap<QString, QStringList> m_paramMap;
  /// The ui form
  Ui::IndirectLoadILL m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
