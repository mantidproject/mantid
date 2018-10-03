// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
