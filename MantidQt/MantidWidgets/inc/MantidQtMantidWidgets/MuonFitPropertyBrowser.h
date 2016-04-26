#ifndef MUONFITPROPERTYBROWSER_H_
#define MUONFITPROPERTYBROWSER_H_

#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/IMuonFitFunctionControl.h"

/* Forward declarations */

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;
class QVBoxLayout;

namespace Mantid
{
  namespace API
  {
    class IFitFunction;
    class IPeakFunction;
    class CompositeFunction;
  }
}

namespace MantidQt
{
namespace MantidWidgets
{
class PropertyHandler;

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFitPropertyBrowser
    : public MantidQt::MantidWidgets::FitPropertyBrowser,
      public MantidQt::MantidWidgets::IMuonFitFunctionControl {
  Q_OBJECT

public:
  /// Constructor.
  MuonFitPropertyBrowser(QWidget *parent = NULL, QObject* mantidui = NULL);  
  /// Initialise the layout.
  void init() override;
  /// Set the input workspace name
  void setWorkspaceName(const QString &wsName) override;
  /// Called when the fit is finished
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  /// Add an extra widget into the browser
  void addExtraWidget(QWidget *widget);
  /// Set function externally
  void setFunction(const QString &funcString) override;
  /// Run a non-sequential fit
  void runFit() override;
  /// Run a sequential fit
  void runSequentialFit() override;

public slots:
  /// Perform the fit algorithm
  void fit() override;
  /// Open sequential fit dialog
  void sequentialFit() override;

signals:
  /// Emitted when sequential fit is requested by user
  void sequentialFitRequested();
  /// Emitted when a fit or sequential fit is requested
  void functionUpdateRequested(bool sequential) override;

protected:
  void showEvent(QShowEvent *e) override;

private slots:
  void doubleChanged(QtProperty *prop) override;

private:  
  /// Get the registered function names
  void populateFunctionNames() override;
  /// Check if the workspace can be used in the fit
  bool isWorkspaceValid(Mantid::API::Workspace_sptr) const override;
  /// Layout for extra widgets
  QVBoxLayout *m_additionalLayout;
};

} // MantidQt
} // API


#endif /*MUONFITPROPERTYBROWSER_H_*/
