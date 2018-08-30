#ifndef MANTIDWIDGETS_IFUNCTIONBROWSER_H_
#define MANTIDWIDGETS_IFUNCTIONBROWSER_H_

#include "DllConfig.h"

#include "FunctionBrowserSubscriber.h"

namespace MantidQt {
namespace MantidWidgets {
namespace QENS {

class MANTIDQT_INDIRECT_DLL IFunctionBrowser {
public:
  virtual ~IFunctionBrowser() = default;
  virtual void subscribe(FunctionBrowserSubscriber *subscriber) = 0;

  virtual void setParameterValue(std::string const &name, double value) = 0;
  virtual void setParameterError(std::string const &name, double value) = 0;
  virtual void removeParameterError(std::string const &name) = 0;
  virtual void setParameterTie(std::string const &name,
                               std::string const &tie) = 0;
  virtual void removeParameterTie(std::string const &name) = 0;
  virtual void setParameterUpperBound(std::string const &name,
                                      double bound) = 0;
  virtual void setParameterLowerBound(std::string const &name,
                                      double bound) = 0;
  virtual void setParameterBounds(std::string const &name, double lowerBound,
                                  double upperBound) = 0;
  virtual void removeParameterUpperBound(std::string const &name) = 0;
  virtual void removeParameterLowerBound(std::string const &name) = 0;
  virtual void removeParameterConstraints(std::string const &name) = 0;
  virtual std::vector<std::size_t> getSelectedFunctionPosition() const = 0;
  virtual void selectFunctionAt(std::vector<std::size_t> const &position) = 0;
  virtual void addFunctionToSelectedFunction(std::string const &name) = 0;
  virtual void
  addFunctionToSelectedFunctionAndSelect(std::string const &name) = 0;
  virtual void removeSelectedFunction() = 0;
  virtual void addIndexToSelectedFunction(std::string const &index) = 0;
  virtual void setIndexOfSelectedFunction(std::string const &index) = 0;
  virtual void
  setIndicesOfFunctionsAt(std::vector<std::string> const &indices,
                          std::vector<std::size_t> const &position) = 0;
  virtual void addParameterToSelectedFunction(std::string const &name,
                                              std::string const &description,
                                              double value) = 0;
  virtual void addIntAttributeToSelectedFunction(std::string const &name,
                                                 int value) = 0;
  virtual void addBoolAttributeToSelectedFunction(std::string const &name,
                                                  bool value) = 0;
  virtual void addDoubleAttributeToSelectedFunction(std::string const &name,
                                                    double value) = 0;
  virtual void
  addStringAttributeToSelectedFunction(std::string const &name,
                                       std::string const &value) = 0;
  virtual void addFileAttributeToSelectedFunction(std::string const &name,
                                                  std::string const &value) = 0;
  virtual void
  addFormulaAttributeToSelectedFunction(std::string const &name,
                                        std::string const &value) = 0;
  virtual void
  addWorkspaceAttributeToSelectedFunction(std::string const &name,
                                          std::string const &value) = 0;
  virtual void
  addVectorAttributeToSelectedFunction(std::string const &name,
                                       std::vector<double> const &value) = 0;
  virtual void setIntAttribute(std::string const &name, int value) = 0;
  virtual void setBoolAttribute(std::string const &name, bool value) = 0;
  virtual void setDoubleAttribute(std::string const &name, double value) = 0;
  virtual void setStringAttribute(std::string const &name,
                                  std::string const &value) = 0;
  virtual void setFileAttribute(std::string const &name,
                                std::string const &value) = 0;
  virtual void setFormulaAttribute(std::string const &name,
                                   std::string const &value) = 0;
  virtual void setWorkspaceAttribute(std::string const &name,
                                     std::string const &value) = 0;
  virtual void setVectorAttribute(std::string const &name,
                                  std::vector<double> const &value) = 0;
  virtual void clear() = 0;
  virtual void clearErrors() = 0;
  virtual void copyToClipboard(std::string const &str) = 0;
  virtual void displayCompositeMenu() = 0;
  virtual void displayFunctionMenu() = 0;
  virtual void displayParameterMenu(bool isTied, bool isConstrained) = 0;
};

} // namespace QENS
} // namespace MantidWidgets
} // namespace MantidQt

#endif
