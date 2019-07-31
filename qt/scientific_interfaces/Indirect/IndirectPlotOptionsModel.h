// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_

#include "IPythonRunner.h"
#include "IndirectPlotter.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsModel {
public:
  IndirectPlotOptionsModel(
      IPyRunner *pythonRunner = nullptr,
      boost::optional<std::map<std::string, std::string>> const
          &availableActions = boost::none);
  /// Used by the unit tests so that m_plotter can be mocked
  IndirectPlotOptionsModel(
      IndirectPlotter *plotter,
      boost::optional<std::map<std::string, std::string>> const
          &availableActions = boost::none);
  virtual ~IndirectPlotOptionsModel();

  virtual bool setWorkspace(std::string const &workspaceName);
  virtual void removeWorkspace();

  virtual std::vector<std::string>
  getAllWorkspaceNames(std::vector<std::string> const &workspaceNames) const;

  boost::optional<std::string> workspace() const;

  virtual void setFixedIndices(std::string const &indices);
  virtual bool indicesFixed() const;

  virtual std::string formatIndices(std::string const &indices) const;
  virtual bool
  validateIndices(std::string const &indices,
                  MantidAxis const &axisType = MantidAxis::Spectrum) const;
  virtual bool setIndices(std::string const &indices);

  boost::optional<std::string> indices() const;

  virtual void plotSpectra();
  virtual void plotBins(std::string const &binIndices);
  virtual void plotContour();
  virtual void plotTiled();

  boost::optional<std::string>
  singleDataPoint(MantidAxis const &axisType) const;

  std::map<std::string, std::string> availableActions() const;

private:
  bool validateSpectra(Mantid::API::MatrixWorkspace_sptr workspace,
                       std::string const &spectra) const;
  bool validateBins(Mantid::API::MatrixWorkspace_sptr workspace,
                    std::string const &bins) const;

  boost::optional<std::string>
  checkWorkspaceSize(std::string const &workspaceName,
                     MantidAxis const &axisType) const;

  std::map<std::string, std::string> m_actions;
  bool m_fixedIndices;
  boost::optional<std::string> m_workspaceIndices;
  boost::optional<std::string> m_workspaceName;
  std::unique_ptr<IndirectPlotter> m_plotter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_ */
