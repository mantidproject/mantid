// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidQtWidgets/Common/IConfiguredAlgorithm.h>
#include <string>

namespace MantidQt::CustomInterfaces {
class IContainerSubtractionModel {
public:
  virtual ~IContainerSubtractionModel() = default;
  virtual void setSampleWS(const std::string &name) = 0;
  virtual void setCanWS(const std::string &name) = 0;
  virtual void setSubtractedWS(const std::string &name) = 0;
  virtual void removeSubtractedWS() = 0;
  virtual const Mantid::API::MatrixWorkspace_sptr &sampleWS() const = 0;
  virtual const Mantid::API::MatrixWorkspace_sptr &canWS() const = 0;
  virtual const Mantid::API::MatrixWorkspace_sptr &subtractedWS() const = 0;
  virtual const Mantid::API::MatrixWorkspace_sptr &modCanWS() const = 0;
  virtual API::IConfiguredAlgorithm_sptr prepareSubtraction(double shiftX = 0.0, double scale = 1.0,
                                                            bool doRebin = false) = 0;
  virtual void updateContainer(double shiftX, double scale) = 0;
  virtual std::vector<std::string> getAllValidWorkspaceNames() const = 0;
  virtual void addShiftLog(double shiftX) = 0;
};

class MANTIDQT_INELASTIC_DLL ContainerSubtractionModel final : public IContainerSubtractionModel {

public:
  ContainerSubtractionModel();
  ~ContainerSubtractionModel() override;

  void setSampleWS(const std::string &name) override;
  void setCanWS(const std::string &name) override;
  void setSubtractedWS(const std::string &name) override;
  void removeSubtractedWS() override;

  const Mantid::API::MatrixWorkspace_sptr &sampleWS() const override;
  const Mantid::API::MatrixWorkspace_sptr &canWS() const override;
  const Mantid::API::MatrixWorkspace_sptr &subtractedWS() const override;
  const Mantid::API::MatrixWorkspace_sptr &modCanWS() const override;

  void updateContainer(double shiftX, double scale) override;
  std::vector<std::string> getAllValidWorkspaceNames() const override;
  API::IConfiguredAlgorithm_sptr prepareSubtraction(double shiftX = 0.0, double scale = 1.0,
                                                    bool doRebin = false) override;

  API::IConfiguredAlgorithm_sptr minusWorkspace(const Mantid::API::MatrixWorkspace_sptr &lhsWorkspace,
                                                const Mantid::API::MatrixWorkspace_sptr &rhsWorkspace) const;
  Mantid::API::MatrixWorkspace_sptr rebinToWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspaceToRebin,
                                                     const Mantid::API::MatrixWorkspace_sptr &workspaceToMatch) const;

  void addShiftLog(double shiftX) override;

private:
  /// Loaded workspaces
  Mantid::API::MatrixWorkspace_sptr newWorkspace(const std::string &wsName) const;
  Mantid::API::MatrixWorkspace_sptr m_csSampleWS;
  Mantid::API::MatrixWorkspace_sptr m_csContainerWS;
  Mantid::API::MatrixWorkspace_sptr m_csSubtractedWS;
  Mantid::API::MatrixWorkspace_sptr m_csModContainerWS;
};
} // namespace MantidQt::CustomInterfaces
