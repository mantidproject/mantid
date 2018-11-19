// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSIMULATIONTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSIMULATIONTAB_H_

#include "IndirectTab.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include <QSettings>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
/**
        This class defines a abstract base class for the different tabs of the
   Indirect Simulation interface.
        Any joint functionality shared between each of the tabs should be
   implemented here as well as defining
        shared member functions.

        @author Samuel Jackson, STFC
*/

class DLLExport IndirectSimulationTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectSimulationTab(QWidget *parent = nullptr);
  ~IndirectSimulationTab() override;

  virtual void loadSettings(const QSettings &settings) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
