#ifndef MANTIDWIDGETS_FUNCTIONBROWSERPRESENTER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSERPRESENTER_H_

#include "FunctionBrowserSubscriber.h"

namespace MantidQt {
namespace MantidWidgets {

namespace QENS {
class IFunctionBrowser;
}
class IFunctionModel;

class FunctionBrowserPresenterSubscriber {
public:
  virtual void functionChanged() = 0;
  virtual void parameterValueChanged(std::string const &parameter,
                                     double value) = 0;
  virtual void attributeChanged(std::string const &attribute) = 0;
  virtual void editParameter(std::string const &parameter) = 0;
};

class FunctionBrowserPresenter : public FunctionBrowserSubscriber {
public:
  FunctionBrowserPresenter(QENS::IFunctionBrowser *browser,
                           IFunctionModel *model);

  void subscribe(FunctionBrowserPresenterSubscriber *subscriber);

  void setFunction(std::string const &functionString) override;
  void addFunction(std::string const &name,
                   std::vector<std::size_t> const &position) override;
  void removeFunction(std::vector<std::size_t> position) override;
  void parameterChanged(std::string const &name, double value) override;
  void fixParameter(std::string const &name) override;
  void removeTie(std::string const &name) override;
  void setTie(std::string const &name, std::string const &expression) override;
  void tieChanged(std::string const &name,
                  std::string const &expression) override;
  void addConstraints(std::string const &name, double upperBound,
                      double lowerBound) override;
  void addConstraints10(std::string const &name) override;
  void addConstraints50(std::string const &name) override;
  void removeConstraint(std::string const &name,
                        std::string const &type) override;
  void removeConstraints(std::string const &name) override;

  void stringAttributeChanged(std::string const &name,
                              std::string const &value) override;
  void doubleAttributeChanged(std::string const &name, double value) override;
  void intAttributeChanged(std::string const &name, int value) override;
  void boolAttributeChanged(std::string const &name, bool value) override;
  void vectorDoubleAttributeChanged(std::string const &name,
                                    const std::vector<double> &value) override;
  void vectorSizeAttributeChanged(std::string const &name,
                                  std::size_t size) override;
  void copyFunctionToClipboard() = 0;
  void displayFunctionMenu(std::vector<std::size_t> const &position) override;
  void displayParameterMenu(std::string const &parameter) override;
  void editParameter(std::string const &parameter) override;

  void updateAttributesInBrowser();
  void updateAttributeInBrowser(std::string const &name);
  void updateParametersInBrowser();
  void updateParameterValuesInBrowser();
  void updateParameterValueInBrowser(std::string const &parameter);
  void updateTiesInBrowser();
  void updateTieInBrowser(std::string const &parameter);
  void updateConstraintsInBrowser();
  void updateConstraintsInBrowser(std::string const &parameter);

private:
  void updateLowerBoundInBrowser(std::string const &parameter);
  void updateUpperBoundInBrowser(std::string const &parameter);
  void addFunctionToSelectedInBrowser(std::string const &name,
                                      std::string const &functionIndex);
  void updateFunctionIndicesInBrowser(std::vector<std::size_t> &position);
  void updateFunctionIndicesInBrowser(std::vector<std::size_t> &position,
                                      std::size_t from);

  FunctionBrowserPresenterSubscriber *m_subscriber;
  IFunctionModel *m_model;
  QENS::IFunctionBrowser *m_browser;

  class EmptySubscriber;
  static EmptySubscriber g_defaultSubscriber;
};

class FunctionBrowserPresenter::EmptySubscriber
    : public FunctionBrowserPresenterSubscriber {
public:
  void functionChanged() override {}
  void parameterValueChanged(std::string const &, double) override {}
  void attributeChanged(std::string const &) override {}
  void editParameter(std::string const &) override {}
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif
