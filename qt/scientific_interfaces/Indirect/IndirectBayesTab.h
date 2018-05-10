#ifndef MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_

#include "IndirectTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/LegacyQwt/QwtWorkspaceSpectrumData.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtDoublePropertyManager>
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtIntPropertyManager>
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser>

#include <QDoubleValidator>
#include <QMap>
#include <QSettings>
#include <QWidget>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
/**
This class defines a abstract base class for the different tabs of the Indirect
Bayes interface.
Any joint functionality shared between each of the tabs should be implemented
here as well as defining shared member functions.

@author Samuel Jackson, STFC

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// precision of double properties in bayes tabs
static const unsigned int NUM_DECIMALS = 6;
/// precision for integer properties in bayes tabs
static const unsigned int INT_DECIMALS = 0;

class DLLExport IndirectBayesTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectBayesTab(QWidget *parent = nullptr);
  ~IndirectBayesTab() override;

  /// Base methods implemented in derived classes
  virtual void loadSettings(const QSettings &settings) = 0;

protected slots:
  /// Slot to update the guides when the range properties change
  virtual void updateProperties(QtProperty *prop, double val) = 0;

protected:
  /// Function to run a string as python code
  void runPythonScript(const QString &pyInput);
  /// Tree of the properties
  QtTreePropertyBrowser *m_propTree;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
