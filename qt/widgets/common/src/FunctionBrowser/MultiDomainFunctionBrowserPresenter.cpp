#include "MultiDomainFunctionBrowserPresenter.h"

#include "MDFEditLocalParameterModel.h"
#include "MDFEditLocalParameterPresenter.h"
#include "MultiDomainFunctionBrowser.h"
#include "MultiDomainFunctionModel.h"

namespace {
QWidget *getParent(QWidget *widget) {
  auto parent = widget->parentWidget();
  parent = nullptr == parent ? widget : parent;
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

MultiDomainFunctionBrowserPresenter::MultiDomainFunctionBrowserPresenter(
    MultiDomainFunctionBrowser *browser, MultiDomainFunctionModel *model)
    : FunctionBrowserPresenter(browser, model), m_multiDomainBrowser(browser),
      m_multiDomainModel(model) {}

void MultiDomainFunctionBrowserPresenter::globalChanged(
    std::string const &parameter, bool global) {
  if (global)
    m_multiDomainModel->addEqualityGlobalTie(parameter);
  else
    m_multiDomainModel->removeGlobalTies(parameter);
}

void MultiDomainFunctionBrowserPresenter::editParameter(
    std::string const &name) {
  MDF::EditLocalParameterPresenter editPresenter(
      MDF::EditLocalParameterModel(*m_multiDomainModel, name),
      getParent(m_multiDomainBrowser));
  editPresenter.executeDialog(*m_multiDomainModel);
}

} // namespace MantidWidgets
} // namespace MantidQt