// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"

namespace MantidQt::API {

class EXPORT_OPT_MANTIDQT_COMMON ConfiguredAlgorithm : public IConfiguredAlgorithm {
public:
  ConfiguredAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                      std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps> properties);
  virtual ~ConfiguredAlgorithm() = default;

  Mantid::API::IAlgorithm_sptr algorithm() const override;
  const MantidQt::API::IAlgorithmRuntimeProps &getAlgorithmRuntimeProps() const noexcept override;

protected:
  Mantid::API::IAlgorithm_sptr m_algorithm;

private:
  std::unique_ptr<IAlgorithmRuntimeProps> m_properties;
};
} // namespace MantidQt::API
