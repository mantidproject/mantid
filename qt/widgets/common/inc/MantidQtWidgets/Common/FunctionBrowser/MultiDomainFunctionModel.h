#ifndef MANTIDWIDGETS_MULTIDOMAINFUNCTIONMODEL_H_
#define MANTIDWIDGETS_MULTIDOMAINFUNCTIONMODEL_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionProperties.h"
#include "MantidQtWidgets/Common/FunctionBrowser/IFunctionModel.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON MultiDomainFunctionModel
    : public IFunctionModel {
public:
  MultiDomainFunctionModel();

  virtual Mantid::API::IFunction_sptr getFitFunction() const;

  bool hasZeroDomains() const;

  std::size_t numberOfParameters() const override;
  std::size_t numberOfDomains() const;
  std::string getParameterName(std::size_t index) const override;
  double getParameterValue(std::string const &parameter) const override;
  boost::optional<double>
  getParameterError(std::string const &parameter) const override;
  boost::optional<std::string>
  getParameterTie(std::string const &parameter) const override;
  boost::optional<double>
  getParameterLowerBound(std::string const &name) const override;
  boost::optional<double>
  getParameterUpperBound(std::string const &name) const override;

  std::vector<std::string> getAttributeNames() const override;
  Mantid::API::IFunction::Attribute const &
  getAttribute(std::string const &name) const override;

  Mantid::API::MatrixWorkspace_const_sptr getWorkspace() const;
  std::string getWorkspaceName() const;
  boost::optional<std::size_t> getWorkspaceIndex() const;

  bool isComposite(std::vector<std::size_t> const &position) const override;
  std::size_t
  numberOfFunctionsAt(std::vector<std::size_t> const &position) const override;

  bool isParameterTied(std::string const &name) const override;
  bool isParameterFixed(std::string const &name) const override;
  bool isParameterConstrained(std::string const &name) const override;

  virtual std::string getLocalFunctionString() const override;
  std::size_t numberOfLocalParameters() const;
  std::size_t getActiveDomain() const;
  void setActiveDomain(std::size_t domain);

  void setFunction(std::string const &functionString) override;
  void setFunction(Mantid::API::IFunction_sptr function);
  std::size_t addFunction(std::string const &name,
                          std::vector<std::size_t> const &position) override;
  void removeFunction(std::vector<std::size_t> const &position) override;

  void setStringAttribute(std::string const &name,
                          std::string const &value) override;
  void setDoubleAttribute(std::string const &name, double value) override;
  void setIntAttribute(std::string const &name, int value) override;
  void setBoolAttribute(std::string const &name, bool value) override;
  void setVectorAttribute(std::string const &name,
                          std::vector<double> const &value) override;
  void setVectorAttributeSize(std::string const &name,
                              std::size_t size) override;

  void addDomains(Mantid::API::MatrixWorkspace_sptr workspace);
  void addDomains(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t from,
                  std::size_t to);
  void addDomain(Mantid::API::MatrixWorkspace_sptr workspace,
                 std::size_t workspaceIndex);
  void removeDomain(std::size_t domain);
  void clearDomains();
  void clear();

  template <typename BeginIterator, typename EndIterator>
  void addDomains(Mantid::API::MatrixWorkspace_sptr workspace,
                  BeginIterator startIndexIt, EndIterator endIndexIt);

  void addEqualityGlobalTie(std::string const &parameterName);
  void addGlobalTie(std::string const &parameterName,
                    std::string const &expression);
  void addGlobalTie(std::string const &parameterName,
                    std::string const &expression, std::size_t domain);
  void removeGlobalTies(std::string const &parameterName);
  void removeLocalTies(std::string const &parameterName);
  void clearTies();

  boost::shared_ptr<Mantid::API::IFunction>
  getLocalFunction(std::size_t index) const;

  void setParameterValue(std::string const &parameterName,
                         double value) override;
  void setParameterError(std::string const &parameterName, double value);
  void removeParameterError(std::string const &parameterName);
  void removeParameterErrors();
  void fixParameter(std::string const &parameterName) override;
  void unfixParameter(std::string const &parameterName) override;

  void setParameterTie(std::string const &parameterName,
                       std::string const &expression) override;
  void removeTie(std::string const &parameterName) override;
  void removeTies() override;

  void setParameterUpperBound(std::string const &parameterName,
                              double bound) override;
  void setParameterLowerBound(std::string const &parameterName,
                              double bound) override;
  void setParameterBounds(std::string const &parameterName, double lowerBound,
                          double upperBound) override;
  void setParameterBoundsWithinPercentile(std::string const &parameterName,
                                          double percentile) override;

  void removeConstraint(std::string const &parameterName,
                        std::string const &type) override;
  void removeConstraints(std::string const &parameterName) override;

protected:
  void fixParameterInDomain(std::string const &parameterName,
                            std::size_t domain);
  void unfixParameterInDomain(std::string const &parameterName,
                              std::size_t domain);

  void addLocalTieToDomain(std::string const &parameterName,
                           std::string const &expression, std::size_t domain);
  void removeLocalTieFromDomain(std::string const &parameterName,
                                std::size_t domain);
  void removeLocalTiesFromDomain(std::size_t domain);

  void addUpperBoundToDomain(std::string const &parameterName, double bound,
                             std::size_t domain);
  void addLowerBoundToDomain(std::string const &parameterName, double bound,
                             std::size_t domain);
  void addBoundsToDomain(std::string const &parameterName, double lowerBound,
                         double upperBound, std::size_t domain);
  void addBoundsToDomainWithinPercentile(std::string const &parameterName,
                                         double percentile, std::size_t domain);

  void removeLocalConstraintsFromDomain(std::string const &parameterName,
                                        std::size_t domain);
  void clearLocalConstraintsFromDomain(std::size_t domain);

private:
  Mantid::API::IFunction &getFunction() const;
  LocalFunctionProperties &getActiveProperties();
  LocalFunctionProperties const &getActiveProperties() const;
  boost::optional<std::string> getGlobalTie(std::string const &parameter) const;
  void addEmptyDomain();
  void removeTiesContainingParameter(std::string const &parameter);
  void removeConstraintsContainingParameter(std::string const &parameter);
  void removeParameterProperties(std::string const &parameterName);
  void addGlobalTie(std::string const &parameterName,
                    std::string const &expression, std::size_t fromDomain,
                    std::size_t toDomain);

  std::vector<LocalFunctionProperties> m_localFunctionProperties;
  FunctionProperties m_globalFunctionProperties;
  std::size_t m_activeDomain;
  boost::shared_ptr<Mantid::API::CompositeFunction> m_function;
};

template <typename BeginIterator, typename EndIterator>
void MultiDomainFunctionModel::addDomains(
    Mantid::API::MatrixWorkspace_sptr workspace, BeginIterator startIndexIt,
    EndIterator endIndexIt) {
  for (auto i = startIndexIt; i < endIndexIt; ++i)
    addDatasetDomain(workspace, *i);
}

} // namespace MantidWidgets
} // namespace MantidQt

#endif
