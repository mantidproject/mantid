#ifndef MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_

#include "MantidQtCustomInterfaces/IndirectDataAnalysis.h"
#include "MantidAPI/MatrixWorkspace.h"

class QwtPlotCurve;
class QwtPlot;
class QString;

namespace MantidQt
{
  namespace MantidWidgets
  {
    class RangeSelector;
  }
}

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
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
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class IDATab : public QWidget
  {
    Q_OBJECT

  public:
    /// Constructor
    IDATab(QWidget * parent = 0);

    /// Sets up the tab.
    void setupTab();
    /// Runs the tab.
    void runTab();
    /// Loads the tab's settings.
    void loadTabSettings(const QSettings & settings);
    /// Returns the URL of the Mantid Wiki webpage for the tab.
    QString tabHelpURL();

  protected:
    /// Displays the given message in a dialog box.
    void showInformationBox(const QString & message);
    /// Run a piece of python code and return any output that was written to stdout
    QString runPythonCode(const QString & code, bool no_output = false);
    /// Run load nexus and return the workspace.
    Mantid::API::MatrixWorkspace_const_sptr runLoadNexus(const QString & filename, const QString & wsname);

    /// Creates and returns a "mini plot" looking up the workspace from the ADS
    QwtPlotCurve* plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const QString & workspace, size_t index);
    /// Creates and returns a "mini plot".
    QwtPlotCurve* plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const  Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t index);

    /// Returns the range of the given curve data.
    std::pair<double,double> getCurveRange(QwtPlotCurve* curve);

    /// Returns a handle to the UI form object stored in the IndirectDataAnalysis class.
    Ui::IndirectDataAnalysis & uiForm();
    /// Returns a const handle to the UI form object stored in the IndirectDataAnalysis class.
    const Ui::IndirectDataAnalysis & uiForm() const;
    /// Returns a handle to the DoubleEditorFactory object stored in the IndirectDataAnalysis class.
    DoubleEditorFactory * doubleEditorFactory();
    /// Returns a handle to the QtCheckBoxFactory object stored in the IndirectDataAnalysis class.
    QtCheckBoxFactory * qtCheckBoxFactory();

  protected slots:
    /// Slot that can be called when a user eidts an input.
    void inputChanged();

  private:
    /// Overidden by child class.
    virtual void setup() = 0;
    /// Overidden by child class.
    virtual void run() = 0;
    /// Overidden by child class.
    virtual QString validate() = 0;
    /// Overidden by child class.
    virtual void loadSettings(const QSettings & settings) = 0;
    /// Overidden by child class.
    virtual QString helpURL() = 0;

    /// A pointer to the parent (friend) IndirectDataAnalysis object.
    IndirectDataAnalysis * m_parent;
  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IDATAB_H_ */
