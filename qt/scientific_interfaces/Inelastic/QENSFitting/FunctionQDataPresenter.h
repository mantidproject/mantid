// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitDataPresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"
#include "FunctionQAddWorkspaceDialog.h"
#include "FunctionQDataView.h"

namespace {
struct FunctionQParameters {
  FunctionQParameters() : m_widths(), m_widthSpectra(), m_eisf(), m_eisfSpectra() {}
  FunctionQParameters(std::pair<std::vector<std::string>, std::vector<std::size_t>> widths,
                      std::pair<std::vector<std::string>, std::vector<std::size_t>> eisfs)
      : m_widths(widths.first), m_widthSpectra(widths.second), m_eisf(eisfs.first), m_eisfSpectra(eisfs.second) {}

  std::vector<std::string> names(std::string const &parameterType) const {
    if (parameterType == "Width") {
      return m_widths;
    } else if (parameterType == "EISF") {
      return m_eisf;
    }
    return {};
  }

  std::vector<std::size_t> spectra(std::string const &parameterType) const {
    if (parameterType == "Width") {
      return m_widthSpectra;
    } else if (parameterType == "EISF") {
      return m_eisfSpectra;
    }
    throw std::logic_error("An unexpected parameter type '" + parameterType + "'is active.");
  }

  std::vector<std::string> types() const {
    std::vector<std::string> types;
    if (!m_widths.empty())
      types.emplace_back("Width");
    if (!m_eisf.empty())
      types.emplace_back("EISF");
    return types;
  }

  operator bool() const { return !m_widthSpectra.empty() || !m_eisfSpectra.empty(); }

private:
  std::vector<std::string> m_widths;
  std::vector<std::size_t> m_widthSpectra;
  std::vector<std::string> m_eisf;
  std::vector<std::size_t> m_eisfSpectra;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL IFunctionQDataPresenter {
public:
  virtual void handleAddClicked() = 0;
  virtual void handleWorkspaceChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &workspace) = 0;
  virtual void handleParameterTypeChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &type) = 0;
};

class MANTIDQT_INELASTIC_DLL FunctionQDataPresenter : public FitDataPresenter, public IFunctionQDataPresenter {

public:
  FunctionQDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view);
  bool addWorkspaceFromDialog(MantidWidgets::IAddWorkspaceDialog const *dialog) override;
  void addWorkspace(const std::string &workspaceName, const std::string &paramType, const int &spectrum_index) override;
  void setActiveSpectra(std::vector<std::size_t> const &activeParameterSpectra, std::size_t parameterIndex,
                        WorkspaceID dataIndex, bool single = true) override;

  void handleAddClicked() override;
  void handleWorkspaceChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &workspace) override;
  void handleParameterTypeChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &type) override;

protected:
  void addTableEntry(FitDomainIndex row) override;

private:
  void setActiveParameterType(const std::string &type);
  void updateActiveWorkspaceID(WorkspaceID index);
  void updateParameterOptions(FunctionQAddWorkspaceDialog *dialog, const FunctionQParameters &parameters);
  void updateParameterTypes(FunctionQAddWorkspaceDialog *dialog, FunctionQParameters const &parameters);
  std::map<std::string, std::string> chooseFunctionQFunctions(bool paramWidth) const;
  void setActiveWorkspaceIDToCurrentWorkspace(MantidWidgets::IAddWorkspaceDialog const *dialog);

  std::string m_activeParameterType;
  WorkspaceID m_activeWorkspaceID;

  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
