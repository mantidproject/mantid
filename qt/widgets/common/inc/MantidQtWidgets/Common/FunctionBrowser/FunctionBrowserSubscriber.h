#ifndef MANTIDWIDGETS_FUNCTIONBROWSERSUBSCRIBER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSERSUBSCRIBER_H_

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class FunctionBrowserSubscriber {
public:
  virtual void setFunction(std::string const &functionString) = 0;
  virtual void addFunction(std::string const &name,
                           std::vector<std::size_t> const &position) = 0;
  virtual void removeFunction(std::vector<std::size_t> position) = 0;
  virtual void parameterChanged(std::string const &name, double value) = 0;
  virtual void fixParameter(std::string const &name) = 0;
  virtual void removeTie(std::string const &name) = 0;
  virtual void setTie(std::string const &name,
                      std::string const &expression) = 0;
  virtual void tieChanged(std::string const &name,
                          std::string const &expression) = 0;
  virtual void addConstraints(std::string const &name, double lowerBound,
                              double upperBound) = 0;
  virtual void addConstraints10(std::string const &name) = 0;
  virtual void addConstraints50(std::string const &name) = 0;
  virtual void removeConstraint(std::string const &name,
                                std::string const &type) = 0;
  virtual void removeConstraints(std::string const &name) = 0;

  virtual void stringAttributeChanged(std::string const &name,
                                      std::string const &value) = 0;
  virtual void doubleAttributeChanged(std::string const &name,
                                      double value) = 0;
  virtual void intAttributeChanged(std::string const &name, int value) = 0;
  virtual void boolAttributeChanged(std::string const &name, bool value) = 0;
  virtual void
  vectorDoubleAttributeChanged(std::string const &name,
                               const std::vector<double> &value) = 0;
  virtual void vectorSizeAttributeChanged(std::string const &name,
                                          std::size_t size) = 0;
  virtual void copyFunctionToClipboard() = 0;
  virtual void editParameter(std::string const &name) = 0;
  virtual void
  displayFunctionMenu(std::vector<std::size_t> const &position) = 0;
  virtual void displayParameterMenu(std::string const &parameter) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif