#ifndef MANTIDWIDGETS_MULTIDOMAINFUNCTIONBROWSER_H_
#define MANTIDWIDGETS_MULTIDOMAINFUNCTIONBROWSER_H_

#include "FunctionBrowser.h"
#include "MultiDomainFunctionBrowserSubscriber.h"

namespace MantidQt {
namespace MantidWidgets {

class MultiDomainFunctionBrowser : public FunctionBrowser {
public:
  MultiDomainFunctionBrowser();
  MultiDomainFunctionBrowser(QWidget *parent);
  
  void subscribeToMultiDomainBrowser(MultiDomainFunctionBrowserSubscriber *subscriber);
  
  std::unique_ptr<QtTreePropertyBrowser> createNewBrowser() const override;
  std::unique_ptr<QtAbstractEditorFactory<ParameterPropertyManager>>
  getParameterEditorFactory() const override;
  
protected slots:
  void globalChanged(QtProperty *, QString const &, bool);
  void parameterButtonClicked(QtProperty *);
  
private:
  MultiDomainFunctionBrowserSubscriber *m_multiDomainSubscriber;
};

}
}

#endif
