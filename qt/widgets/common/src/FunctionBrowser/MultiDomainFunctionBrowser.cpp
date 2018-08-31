#include "MultiDomainFunctionBrowser.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

namespace {
constexpr std::string GLOBAL_OPTION_NAME = "Global";
}

namespace MantidQt {
namespace MantidWidgets {

MultiDomainFunctionBrowser::MultiDomainFunctionBrowser() : FunctionBrowser() {}

MultiDomainFunctionBrowser::MultiDomainFunctionBrowser(QWidget *parent)
    : FunctionBrowser(parent) {}

void MultiDomainFunctionBrowser::subscribeToMultiDomainBrowser(
    MultiDomainFunctionBrowserSubscriber *subscriber) {
  m_multiDomainSubscriber = subscriber;
}

std::unique_ptr<QtTreePropertyBrowser>
MultiDomainFunctionBrowser::createNewBrowser() const {
  QStringList options = {GLOBAL_OPTION_NAME};
  return Mantid::Kernel::make_unique<QtTreePropertyBrowser>(nullptr, options);
}

std::unique_ptr<QtAbstractEditorFactory<ParameterPropertyManager>>
MultiDomainFunctionBrowser::getParameterEditorFactory() const {
  auto buttonFactory = new DoubleDialogEditorFactory(this);
  auto compositeFactory =
      new CompositeEditorFactory<ParameterPropertyManager>(this, buttonFactory);
  compositeFactory->setSecondaryFactory(GLOBAL_OPTION_NAME, paramEditorFactory);
  parameterEditorFactory = compositeFactory;
  connect(buttonFactory, SIGNAL(buttonClicked(QtProperty *)), this,
          SLOT(parameterButtonClicked(QtProperty *)));
  connectEditorCloseToBrowser(buttonFactory);
}

void MultiDomainFunctionBrowser::globalChanged(QtProperty *,
                                               QString const &parameter,
                                               bool global) {
  m_subscriber->globalChanged(parameter.toStdString(), global);
}

void MultiDomainFunctionBrowser::parameterButtonClicked(QtProperty *prop) {
  m_subscriber->editParameter(getParameterName(prop).toStdString());
}

} // namespace MantidWidgets
} // namespace MantidQt