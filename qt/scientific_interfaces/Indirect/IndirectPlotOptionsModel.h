// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectPlotOptionsModel {
public:
  IndirectPlotOptionsModel();
  virtual ~IndirectPlotOptionsModel();

  bool setWorkspace(std::string const &workspaceName);
  void removeWorkspace();

  std::string formatSpectra(std::string const &spectra) const;
  bool setSpectra(std::string const &spectra);

  boost::optional<std::string> spectra() const;

  boost::optional<std::string> getPlotSpectraString(bool errorBars) const;
  boost::optional<std::string> getPlotContourString() const;
  boost::optional<std::string> getPlotTiledString() const;

  void plotSpectra(bool errorBars);
  void plotContour();
  void plotTiled();

private:
  bool validateSpectra(std::string const &spectra) const;
  bool validateSpectra(Mantid::API::MatrixWorkspace_sptr workspace,
                       std::string const &spectra) const;

  boost::optional<std::string> workspace() const;

  boost::optional<std::string> m_spectra;
  boost::optional<std::string> m_workspaceName;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTPLOTOPTIONSMODEL_H_ */
