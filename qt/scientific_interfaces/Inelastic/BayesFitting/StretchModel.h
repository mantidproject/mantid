// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "StretchData.h"
#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IStretchModel {
public:
  virtual ~IStretchModel() = default;

  virtual MantidQt::API::IConfiguredAlgorithm_sptr stretchAlgorithm(const StretchRunData &algParams,
                                                                    const std::string &fitWorkspaceName,
                                                                    const std::string &contourWorkspaceName) const = 0;

  virtual API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName) const = 0;
};

class MANTIDQT_INELASTIC_DLL StretchModel : public IStretchModel {
public:
  StretchModel() = default;
  ~StretchModel() override = default;

  MantidQt::API::IConfiguredAlgorithm_sptr stretchAlgorithm(const StretchRunData &algParams,
                                                            const std::string &fitWorkspaceName,
                                                            const std::string &contourWorkspaceName) const override;

  API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName) const override;
};
} // namespace CustomInterfaces
} // namespace MantidQt
