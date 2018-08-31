#include "FunctionBrowserPresenter.h"
#include "IFunctionBrowser.h"
#include "IFunctionModel.h"

#include "MDFEditLocalParameterPresenter.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <numeric>

using MantidQt::MantidWidgets::Function::IFunctionBrowser;
using MantidQt::MantidWidgets::IFunctionModel;

namespace {
Mantid::API::IFunction_sptr
createFunctionFromString(std::string const &functionString) {
  return Mantid::API::FunctionFactory::Instance().createInitialized(
      functionString);
}

std::string createFunctionIndex(std::size_t index) {
  return "f" + std::to_string(index) + ".";
}

class SetAttributePropertyInFunctionBrowser
    : public Mantid::API::IFunction::ConstAttributeVisitor<> {
public:
  SetAttributePropertyInFunctionBrowser(IFunctionBrowser *browser)
      : m_browser(browser), m_attributeName() {}

  void setAttributeName(std::string const &attributeName) {
    m_attributeName = attributeName;
  }

protected:
  void apply(std::string const &value) const override {
    if (m_attributeName == "FileName")
      m_browser->setFileAttribute(m_attributeName, value);
    else if (m_attributeName == "Formula")
      m_browser->setFormulaAttribute(m_attributeName, value);
    else if (m_attributeName == "Workspace")
      m_browser->setWorkspaceAttribute(m_attributeName, value);
    else
      m_browser->setStringAttribute(m_attributeName, value);
  }

  void apply(double const &value) const override {
    m_browser->setDoubleAttribute(m_attributeName, value);
  }

  void apply(int const &value) const override {
    m_browser->setIntAttribute(m_attributeName, value);
  }

  void apply(bool const &value) const override {
    m_browser->setBoolAttribute(m_attributeName, value);
  }

  void apply(std::vector<double> const &value) const override {
    m_browser->setVectorAttribute(m_attributeName, value);
  }

private:
  IFunctionBrowser *m_browser;
  std::string m_attributeName;
};

class AddAttributePropertyToFunctionBrowser
    : public Mantid::API::IFunction::ConstAttributeVisitor<> {
public:
  AddAttributePropertyToFunctionBrowser(IFunctionBrowser *browser)
      : m_browser(browser), m_attributeName() {}

  void setAttributeName(std::string const &attributeName) {
    m_attributeName = attributeName;
  }

protected:
  void apply(std::string const &value) const override {
    if (m_attributeName == "FileName")
      m_browser->addFileAttributeToSelectedFunction(m_attributeName, value);
    else if (m_attributeName == "Formula")
      m_browser->addFormulaAttributeToSelectedFunction(m_attributeName, value);
    else if (m_attributeName == "Workspace")
      m_browser->addWorkspaceAttributeToSelectedFunction(m_attributeName,
                                                         value);
    else
      m_browser->addStringAttributeToSelectedFunction(m_attributeName, value);
  }

  void apply(double const &value) const override {
    m_browser->addDoubleAttributeToSelectedFunction(m_attributeName, value);
  }

  void apply(int const &value) const override {
    m_browser->addIntAttributeToSelectedFunction(m_attributeName, value);
  }

  void apply(bool const &value) const override {
    m_browser->addBoolAttributeToSelectedFunction(m_attributeName, value);
  }

  void apply(std::vector<double> const &value) const override {
    m_browser->addVectorAttributeToSelectedFunction(m_attributeName, value);
  }

private:
  IFunctionBrowser *m_browser;
  std::string m_attributeName;
};

void addAttributesToBrowser(IFunctionBrowser *browser,
                            Mantid::API::IFunction const &function) {
  AddAttributePropertyToFunctionBrowser addAttributeToBrowser(browser);

  auto const attributeNames = function.getAttributeNames();
  for (auto const &attributeName : attributeNames) {
    addAttributeToBrowser.setAttributeName(attributeName);
    function.getAttribute(attributeName).apply(addAttributeToBrowser);
  }
}

void addParametersToBrowser(IFunctionBrowser *browser,
                            Mantid::API::IFunction const &function) {
  for (auto i = 0u; i < function.nParams(); ++i)
    browser->addParameterToSelectedFunction(function.parameterName(i),
                                            function.parameterDescription(i),
                                            function.getParameter(i));
}

void addParametersAndAttributesToBrowser(IFunctionBrowser *browser,
                                         Mantid::API::IFunction_sptr function) {
  addAttributesToBrowser(browser, *function);
  addParametersToBrowser(browser, *function);
}

void addParametersAndAttributesToBrowser(IFunctionBrowser *browser,
                                         std::string const &functionName) {
  addParametersAndAttributesToBrowser(
      browser,
      Mantid::API::FunctionFactory::Instance().createFunction(functionName));
}

void addFunctionToBrowser(IFunctionBrowser &browser,
                          Mantid::API::IFunction const &function,
                          std::string const &functionIndex) {
  browser.addFunctionToSelectedFunctionAndSelect(function.name());
  browser.addIndexToSelectedFunction(functionIndex);
}

void setFunctionInBrowser(IFunctionBrowser *browser,
                          Mantid::API::IFunction_sptr function,
                          std::string const &functionIndex);

void setFunctionInBrowser(IFunctionBrowser *browser,
                          Mantid::API::CompositeFunction_sptr composite,
                          std::string const &functionIndex) {
  addFunctionToBrowser(*browser, *composite, functionIndex);
  addAttributesToBrowser(browser, *composite);

  auto const position = browser->getSelectedFunctionPosition();
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    setFunctionInBrowser(browser, composite->getFunction(i),
                         functionIndex + createFunctionIndex(i));
    browser->selectFunctionAt(position);
  }
}

void setFunctionInBrowser(IFunctionBrowser *browser,
                          Mantid::API::IFunction_sptr function,
                          std::string const &functionIndex) {
  if (auto const composite =
          boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function))
    setFunctionInBrowser(browser, composite, functionIndex);
  else {
    addFunctionToBrowser(*browser, *function, functionIndex);
    addParametersAndAttributesToBrowser(browser, function);
  }
}

void setFunctionInBrowser(IFunctionBrowser *browser,
                          std::string const &functionString) {
  setFunctionInBrowser(browser, createFunctionFromString(functionString),
                       createFunctionIndex(0));
}

void removeConstraintInBrowser(IFunctionBrowser *browser,
                               std::string const &parameter,
                               std::string const &boundType) {
  if (boundType == "UpperBound")
    browser->removeParameterUpperBound(parameter);
  else
    browser->removeParameterLowerBound(parameter);
}

std::string positionToFunctionIndex(std::vector<std::size_t> const &indices) {
  return std::accumulate(indices.begin(), indices.end(), std::string(),
                         [](std::string const &accumulated, std::size_t index) {
                           return accumulated + createFunctionIndex(index);
                         });
}

template <typename F>
void forEachParameterIn(IFunctionModel *model, F const &functor) {
  for (auto i = 0u; i < model->numberOfParameters(); ++i)
    functor(model->parameterName(i));
}

template <typename F>
void forEachAttributeIn(IFunctionModel *model, F const &functor) {
  for (auto const &name : model->getAttributeNames())
    functor(name, model->getAttribute(name));
}

std::vector<std::string> createFunctionIndices(std::string const &prefix,
                                               std::size_t from,
                                               std::size_t to) {
  std::vector<std::string> indices;
  indices.reserve(to - from + 1);
  for (auto i = from; i < to; ++i)
    indices.emplace_back(prefix + createFunctionIndex(i));
  return indices;
}

void setFunctionIndicesInBrowser(IFunctionBrowser *browser,
                                 std::string const &prefix,
                                 std::vector<std::size_t> const &position,
                                 std::size_t from, std::size_t to) {
  browser->setIndicesOfFunctionsAt(createFunctionIndices(prefix, from, to),
                                   position);
}

void setFunctionIndicesInBrowser(IFunctionBrowser *browser,
                                 std::vector<std::size_t> const &position,
                                 std::size_t from, std::size_t to) {
  setFunctionIndicesInBrowser(browser, positionToFunctionIndex(position),
                              position, from, to);
}

void setFunctionIndicesInBrowser(IFunctionBrowser *browser,
                                 IFunctionModel *model,
                                 std::vector<std::size_t> &position,
                                 std::size_t from);

void setFunctionIndicesInBrowser(IFunctionBrowser *browser,
                                 IFunctionModel *model,
                                 std::vector<std::size_t> &position,
                                 std::size_t from, std::size_t to) {
  setFunctionIndicesInBrowser(browser, position, from, to);

  position.emplace_back(0);
  for (auto i = from; i < to; ++i) {
    position.back() = i;
    setFunctionIndicesInBrowser(browser, model, position, 0);
  }
  position.pop_back();
}

void setFunctionIndicesInBrowser(IFunctionBrowser *browser,
                                 IFunctionModel *model,
                                 std::vector<std::size_t> &position,
                                 std::size_t from) {
  auto const numberOfFunctions = model->numberOfFunctionsAt(position);
  if (numberOfFunctions > from)
    setFunctionIndicesInBrowser(browser, model, position, from,
                                numberOfFunctions);
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FunctionBrowserPresenter::FunctionBrowserPresenter(
    IFunctionBrowser *browser, IFunctionModel *model)
    : m_browser(browser), m_model(model),
      m_subscriber(&FunctionBrowserPresenter::g_defaultSubscriber) {
  browser->subscribe(this);
}

void FunctionBrowserPresenter::subscribe(
    FunctionBrowserPresenterSubscriber *subscriber) {
  m_subscriber = subscriber;
}

void FunctionBrowserPresenter::setFunction(std::string const &functionString) {
  m_model->setFunction(functionString);
  m_browser->clear();
  setFunctionInBrowser(m_browser, functionString);
  updateTiesInBrowser();
  updateConstraintsInBrowser();
  m_subscriber->functionChanged();
}

void FunctionBrowserPresenter::addFunction(
    std::string const &name, std::vector<std::size_t> const &position) {
  auto const index = m_model->addFunction(name, position);
  auto const functionIndex =
      positionToFunctionIndex(position) + createFunctionIndex(index);

  if (position.empty() && m_model->numberOfFunctionsAt(position) == 1)
    setFunction(m_model->getLocalFunctionString());
  else
    addFunctionToSelectedInBrowser(name, functionIndex);
  m_subscriber->functionChanged();
}

void FunctionBrowserPresenter::removeFunction(
    std::vector<std::size_t> position) {
  m_model->removeFunction(position);
  m_browser->removeSelectedFunction();
  updateFunctionIndicesInBrowser(position);
  m_subscriber->functionChanged();
}

void FunctionBrowserPresenter::parameterChanged(std::string const &name,
                                                double value) {
  m_model->setLocalParameterValue(name, value);
  m_subscriber->parameterValueChanged(name, value);
}

void FunctionBrowserPresenter::fixParameter(std::string const &name) {
  m_model->fixLocalParameter(name);
  m_browser->setParameterTie(name,
                             std::to_string(m_model->parameterValue(name)));
}

void FunctionBrowserPresenter::removeTie(std::string const &name) {
  m_model->removeLocalTie(name);
  m_browser->removeParameterTie(name);
}

void FunctionBrowserPresenter::setTie(std::string const &name,
                                      std::string const &expression) {
  m_model->setLocalTie(name, expression);
  m_browser->setParameterTie(name, expression);
}

void FunctionBrowserPresenter::tieChanged(std::string const &name,
                                          std::string const &expression) {
  m_model->setLocalTie(name, expression);
}

void FunctionBrowserPresenter::addConstraints(std::string const &name,
                                              double lowerBound,
                                              double upperBound) {
  m_model->addLocalBounds(name, lowerBound, upperBound);
  m_browser->setParameterBounds(name, lowerBound, upperBound);
}

void FunctionBrowserPresenter::addConstraints10(std::string const &name) {
  auto const value = m_model->parameterValue(name);
  m_model->addLocalBoundsWithinPercentile(name, 0.1);
  m_browser->setParameterBounds(name, value * 0.9, value * 1.1);
}

void FunctionBrowserPresenter::addConstraints50(std::string const &name) {
  auto const value = m_model->parameterValue(name);
  m_model->addLocalBoundsWithinPercentile(name, 0.5);
  m_browser->setParameterBounds(name, value * 0.5, value * 1.5);
}

void FunctionBrowserPresenter::removeConstraint(std::string const &name,
                                                std::string const &type) {
  m_model->removeLocalConstraint(name, type);
  removeConstraintInBrowser(m_browser, name, type);
}

void FunctionBrowserPresenter::removeConstraints(std::string const &name) {
  m_model->removeLocalConstraints(name);
  m_browser->removeParameterConstraints(name);
}

void FunctionBrowserPresenter::stringAttributeChanged(
    std::string const &name, std::string const &value) {
  m_model->setStringAttribute(name, value);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::doubleAttributeChanged(std::string const &name,
                                                      double value) {
  m_model->setDoubleAttribute(name, value);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::intAttributeChanged(std::string const &name,
                                                   int value) {
  m_model->setIntAttribute(name, value);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::boolAttributeChanged(std::string const &name,
                                                    bool value) {
  m_model->setBoolAttribute(name, value);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::vectorDoubleAttributeChanged(
    std::string const &name, std::vector<double> const &value) {
  m_model->setVectorAttribute(name, value);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::vectorSizeAttributeChanged(
    std::string const &name, std::size_t size) {
  m_model->setVectorAttributeSize(name, size);
  m_subscriber->attributeChanged(name);
}

void FunctionBrowserPresenter::copyFunctionToClipboard() {
  m_browser->copyToClipboard(m_model->getLocalFunctionString());
}

void FunctionBrowserPresenter::displayFunctionMenu(
    std::vector<std::size_t> const &position) {
  if (m_model->isComposite(position) || position.empty())
    m_browser->displayCompositeMenu();
  else
    m_browser->displayFunctionMenu();
}

void FunctionBrowserPresenter::displayParameterMenu(
    std::string const &parameter) {
  bool const isTied = m_model->isParameterTied(parameter);
  bool const isConstrained = m_model->isParameterConstrained(parameter);
  m_browser->displayParameterMenu(isTied, isConstrained);
}

void FunctionBrowserPresenter::updateAttributesInBrowser() {
  SetAttributePropertyInFunctionBrowser updateAttribute(m_browser);
  forEachAttributeIn(m_model,
                     [&](std::string const &name,
                         Mantid::API::IFunction::Attribute const &attribute) {
                       updateAttribute.setAttributeName(name);
                       attribute.apply(updateAttribute);
                     });
}

void FunctionBrowserPresenter::updateAttributeInBrowser(
    std::string const &name) {
  SetAttributePropertyInFunctionBrowser updateAttribute(m_browser);
  updateAttribute.setAttributeName(name);
  m_model->getAttribute(name).apply(updateAttribute);
}

void FunctionBrowserPresenter::updateParametersInBrowser() {
  forEachParameterIn(m_model, [&](std::string const &parameter) {
    updateParameterValueInBrowser(parameter);
    updateTieInBrowser(parameter);
    updateConstraintsInBrowser(parameter);
  });
}

void FunctionBrowserPresenter::updateParameterValuesInBrowser() {
  forEachParameterIn(m_model, [&](std::string const &parameter) {
    updateParameterValueInBrowser(parameter);
  });
}

void FunctionBrowserPresenter::updateParameterValueInBrowser(
    std::string const &parameter) {
  m_browser->setParameterValue(parameter, m_model->parameterValue(parameter));

  if (auto const error = m_model->parameterError(parameter))
    m_browser->setParameterError(parameter, *error);
  else
    m_browser->removeParameterError(parameter);
}

void FunctionBrowserPresenter::updateTiesInBrowser() {
  forEachParameterIn(m_model, [&](std::string const &parameter) {
    updateTieInBrowser(parameter);
  });
}

void FunctionBrowserPresenter::updateTieInBrowser(
    std::string const &parameter) {
  if (auto const tie = m_model->parameterTie(parameter))
    m_browser->setParameterTie(parameter, *tie);
  else
    m_browser->removeParameterTie(parameter);
}

void FunctionBrowserPresenter::updateConstraintsInBrowser() {
  forEachParameterIn(m_model, [&](std::string const &parameter) {
    updateConstraintsInBrowser(parameter);
  });
}

void FunctionBrowserPresenter::updateConstraintsInBrowser(
    std::string const &parameter) {
  updateLowerBoundInBrowser(parameter);
  updateUpperBoundInBrowser(parameter);
}

void FunctionBrowserPresenter::updateLowerBoundInBrowser(
    std::string const &parameter) {
  if (auto const lowerBound = m_model->parameterLowerBound(parameter))
    m_browser->setParameterLowerBound(parameter, *lowerBound);
  else
    m_browser->removeParameterLowerBound(parameter);
}

void FunctionBrowserPresenter::updateUpperBoundInBrowser(
    std::string const &parameter) {
  if (auto const upperBound = m_model->parameterUpperBound(parameter))
    m_browser->setParameterUpperBound(parameter, *upperBound);
  else
    m_browser->removeParameterUpperBound(parameter);
}

void FunctionBrowserPresenter::addFunctionToSelectedInBrowser(
    std::string const &name, std::string const &functionIndex) {
  m_browser->addFunctionToSelectedFunctionAndSelect(name);
  m_browser->addIndexToSelectedFunction(functionIndex);
  addParametersAndAttributesToBrowser(m_browser, name);
}

void FunctionBrowserPresenter::updateFunctionIndicesInBrowser(
    std::vector<std::size_t> &position) {
  if (!position.empty()) {
    auto const from = position.back();
    position.pop_back();
    updateFunctionIndicesInBrowser(position, from);
  }
}

void FunctionBrowserPresenter::updateFunctionIndicesInBrowser(
    std::vector<std::size_t> &position, std::size_t from) {
  setFunctionIndicesInBrowser(m_browser, m_model, position, from);
}

} // namespace MantidWidgets
} // namespace MantidQt
