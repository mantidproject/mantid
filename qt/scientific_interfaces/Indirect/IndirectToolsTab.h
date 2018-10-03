// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTTOOLSTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTTOOLSTAB_H_

#include "IndirectTab.h"
#include "MantidKernel/System.h"
#include <QSettings>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
/**
    This class defines a abstract base class for the different tabs of the
        Indirect Foreign interface.
    Any joint functionality shared between each of the tabs should be
        implemented here as well as defining
    shared member functions.

    @author Samuel Jackson, STFC
*/

class DLLExport IndirectToolsTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectToolsTab(QWidget *parent = nullptr);
  ~IndirectToolsTab() override;

  /// Base methods implemented in derived classes
  virtual void loadSettings(const QSettings &settings) = 0;

signals:
  /// Send signal to parent window to execute python script
  void executePythonScript(const QString &pyInput, bool output);

protected:
  void setup() override = 0;
  void run() override = 0;
  bool validate() override = 0;

  void runPythonScript(const QString &pyInput);
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
