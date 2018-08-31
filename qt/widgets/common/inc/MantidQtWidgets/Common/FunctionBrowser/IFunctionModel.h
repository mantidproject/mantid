#ifndef MANTIDWIDGETS_IFUNCTIONMODEL_H_
#define MANTIDWIDGETS_IFUNCTIONMODEL_H_

#include "MantidAPI/IFunction.h"

#include <boost/optional.hpp>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class IFunctionModel {
public:
  virtual std::size_t numberOfParameters() const = 0;
  virtual std::string getParameterName(std::size_t index) const = 0;
  virtual double getParameterValue(std::string const &name) const = 0;
  virtual boost::optional<double>
  getParameterError(std::string const &name) const = 0;
  virtual boost::optional<std::string>
  getParameterTie(std::string const &name) const = 0;
  virtual boost::optional<double>
  getParameterLowerBound(std::string const &name) const = 0;
  virtual boost::optional<double>
  getParameterUpperBound(std::string const &name) const = 0;
  virtual std::vector<std::string> getAttributeNames() const = 0;
  virtual Mantid::API::IFunction::Attribute const &
  getAttribute(std::string const &name) const = 0;
  virtual bool isComposite(std::vector<std::size_t> const &position) const = 0;
  virtual std::size_t
  numberOfFunctionsAt(std::vector<std::size_t> const &position) const = 0;
  virtual bool isParameterTied(std::string const &name) const = 0;
  virtual bool isParameterFixed(std::string const &name) const = 0;
  virtual bool isParameterConstrained(std::string const &name) const = 0;
  virtual std::string getLocalFunctionString() const = 0;
  virtual void setFunction(std::string const &functionString) = 0;
  virtual std::size_t addFunction(std::string const &name,
                                  std::vector<std::size_t> const &position) = 0;
  virtual void removeFunction(std::vector<std::size_t> const &position) = 0;
  virtual void setParameterValue(std::string const &parameterName,
                                 double value) = 0;
  virtual void fixParameter(std::string const &parameterName) = 0;
  virtual void unfixParameter(std::string const &parameterName) = 0;
  virtual void setParameterTie(std::string const &parameterName,
                               std::string const &expression) = 0;
  virtual void removeTie(std::string const &parameterName) = 0;
  virtual void removeTies() = 0;
  virtual void setParameterUpperBound(std::string const &parameterName,
                                      double bound) = 0;
  virtual void setParameterLowerBound(std::string const &parameterName,
                                      double bound) = 0;
  virtual void setParameterBounds(std::string const &parameterName,
                                  double lowerBound, double upperBound) = 0;
  virtual void
  setParameterBoundsWithinPercentile(std::string const &parameterName,
                                     double percentile) = 0;
  virtual void removeConstraint(std::string const &parameterName,
                                std::string const &type) = 0;
  virtual void removeConstraints(std::string const &parameterName) = 0;
  virtual void setStringAttribute(std::string const &name,
                                  std::string const &value) = 0;
  virtual void setDoubleAttribute(std::string const &name, double value) = 0;
  virtual void setIntAttribute(std::string const &name, int value) = 0;
  virtual void setBoolAttribute(std::string const &name, bool value) = 0;
  virtual void setVectorAttribute(std::string const &name,
                                  std::vector<double> const &value) = 0;
  virtual void setVectorAttributeSize(std::string const &name,
                                      std::size_t size) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif
