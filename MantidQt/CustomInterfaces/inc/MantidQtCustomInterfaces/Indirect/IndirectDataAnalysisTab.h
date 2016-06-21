#ifndef MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "IndirectDataAnalysis.h"
#include "IndirectTab.h"

class QwtPlotCurve;
class QwtPlot;
class QSettings;
class QString;

namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}
}

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport IndirectDataAnalysisTab : public IndirectTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectDataAnalysisTab(QWidget *parent = 0);

  /// Loads the tab's settings.
  void loadTabSettings(const QSettings &settings);

protected:
  /// Function to run a string as python code
  void runPythonScript(const QString &pyInput);

  /// DoubleEditorFactory
  DoubleEditorFactory *m_dblEdFac;
  /// QtCheckBoxFactory
  QtCheckBoxFactory *m_blnEdFac;

protected slots:
  /// Slot that can be called when a user eidts an input.
  void inputChanged();

private:
  /// Overidden by child class.
  void setup() override = 0;
  /// Overidden by child class.
  void run() override = 0;
  /// Overidden by child class.
  bool validate() override = 0;

  /// Overidden by child class.
  virtual void loadSettings(const QSettings &settings) = 0;

  /// A pointer to the parent (friend) IndirectDataAnalysis object.
  IndirectDataAnalysis *m_parent;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_ */
