// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"
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

class MANTIDQT_INDIRECT_DLL ToolsTab : public InelasticTab {
  Q_OBJECT

public:
  ToolsTab(QWidget *parent = nullptr);
  ~ToolsTab() override;

  /// Base methods implemented in derived classes
  virtual void loadSettings(const QSettings &settings) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
