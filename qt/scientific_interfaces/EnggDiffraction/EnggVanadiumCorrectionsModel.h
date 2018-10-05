// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODEL_H_

#include "DllConfig.h"
#include "IEnggVanadiumCorrectionsModel.h"

#include <Poco/Path.h>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggVanadiumCorrectionsModel
    : public IEnggVanadiumCorrectionsModel {

public:
  EnggVanadiumCorrectionsModel(const EnggDiffCalibSettings &calibSettings,
                               const std::string &currentInstrument);

  std::pair<Mantid::API::ITableWorkspace_sptr,
            Mantid::API::MatrixWorkspace_sptr>
  fetchCorrectionWorkspaces(
      const std::string &vanadiumRunNumber) const override;

  void setCalibSettings(const EnggDiffCalibSettings &calibSettings) override;

  void setCurrentInstrument(const std::string &currentInstrument) override;

protected:
  const static std::string CURVES_WORKSPACE_NAME;

  const static std::string INTEGRATED_WORKSPACE_NAME;

private:
  const static std::string VANADIUM_INPUT_WORKSPACE_NAME;

  virtual std::pair<Mantid::API::ITableWorkspace_sptr,
                    Mantid::API::MatrixWorkspace_sptr>
  calculateCorrectionWorkspaces(const std::string &vanadiumRunNumber) const;

  Mantid::API::MatrixWorkspace_sptr
  fetchCachedCurvesWorkspace(const std::string &vanadiumRunNumber) const;

  Mantid::API::ITableWorkspace_sptr
  fetchCachedIntegratedWorkspace(const std::string &vanadiumRunNumber) const;

  std::string
  generateCurvesFilename(const std::string &vanadiumRunNumber) const;

  std::string
  generateIntegratedFilename(const std::string &vanadiumRunNumber) const;

  std::string
  generateVanadiumRunName(const std::string &vanadiumRunNumber) const;

  Mantid::API::MatrixWorkspace_sptr
  loadMatrixWorkspace(const std::string &filename,
                      const std::string &workspaceName) const;

  Mantid::API::ITableWorkspace_sptr
  loadTableWorkspace(const std::string &filename,
                     const std::string &workspaceName) const;

  void saveCorrectionsToCache(
      const std::string &runNumber,
      const Mantid::API::MatrixWorkspace_sptr curvesWorkspace,
      const Mantid::API::ITableWorkspace_sptr integratedWorkspace) const;

  void saveNexus(const std::string &filename,
                 const Mantid::API::Workspace_sptr workspace) const;

  EnggDiffCalibSettings m_calibSettings;

  std::string m_currentInstrument;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODEL_H_
