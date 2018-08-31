#ifndef MDFEDITLOCALPARAMETERMODEL_H_
#define MDFEDITLOCALPARAMETERMODEL_H_

#include "MDFLogValueFinder.h"

namespace MantidQt {
namespace MantidWidgets {
class MultiDomainFunctionModel;
}
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

class EditLocalParameterModel {
public:
  EditLocalParameterModel(MantidWidgets::MultiDomainFunctionModel &model,
                          std::string const &parameter);

  EditLocalParameterModel(std::string const &parameter,
                          std::vector<double> const &values,
                          std::vector<std::string> const &ties,
                          std::vector<bool> const &fixes,
                          std::vector<std::string> const &workspaceNames);

  std::string const &getParameterName() const;

  std::vector<std::string> const &getWorkspaceNames() const;
  std::vector<std::size_t> const &getWorkspaceIndices() const;

  std::size_t numberOfParameters() const;
  double getParameterValue(std::size_t index) const;
  std::string getTie(std::size_t index) const;

  std::string getDelimitedParameters(std::string const &delimiter) const;
  std::vector<std::string> getLogNames() const;

  bool isFixed(std::size_t index) const;
  bool isTied(std::size_t index) const;

  void setParameters(double value);
  void setFixed(bool fixed);
  void setTies(std::string const &tie);
  void setParameter(double value, std::size_t index);
  void fixParameter(bool fixed, std::size_t index);
  void setTie(std::string const &tie, std::size_t index);
  void setValuesToLog(std::string const &logName, std::string const &function);
  void setValueToLog(std::string const &logName, std::string const &function,
                     std::size_t index);

  void
  updateFunctionModel(MantidWidgets::MultiDomainFunctionModel &functionModel);

private:
  std::string const m_parameter;
  std::vector<double> m_values;
  std::vector<std::string> m_ties;
  std::vector<bool> m_fixed;
  std::vector<std::string> m_workspaceNames;
  std::vector<std::size_t> m_workspaceIndices;
  QENS::MDFLogValueFinder m_logFinder;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
