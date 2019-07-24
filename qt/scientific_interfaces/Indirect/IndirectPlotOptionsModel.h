// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_

#include "IndirectPlotter.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsModel {
public:
  IndirectPlotOptionsModel(IndirectTab *parentTab);
  /// Used by the unit tests so that m_plotter can be mocked
  IndirectPlotOptionsModel(std::unique_ptr<IndirectPlotter> plotter);
  virtual ~IndirectPlotOptionsModel();

  bool setWorkspace(std::string const &workspaceName);
  void removeWorkspace();

  boost::optional<std::string> workspace() const;

  void setFixedIndices(std::string const &indices);
  bool indicesFixed() const;

  std::string formatIndices(std::string const &indices) const;
  bool validateIndices(std::string const &indices,
                       MantidAxis const &axisType = MantidAxis::Spectrum) const;
  bool setIndices(std::string const &indices);

  boost::optional<std::string> indices() const;

  void plotSpectra();
  void plotBins();
  void plotContour();
  void plotTiled();

private:
  bool validateSpectra(Mantid::API::MatrixWorkspace_sptr workspace,
                       std::string const &spectra) const;
  bool validateBins(Mantid::API::MatrixWorkspace_sptr workspace,
                    std::string const &bins) const;

  bool m_fixedIndices;
  boost::optional<std::string> m_workspaceIndices;
  boost::optional<std::string> m_workspaceName;
  std::unique_ptr<IndirectPlotter> m_plotter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_ */
