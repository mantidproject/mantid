// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBALGORITHM_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBALGORITHM_H_

#include "Common/DllConfig.h"
#include "IBatchJobAlgorithm.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
 * The BatchJobAlgorithm class overrides ConfiguredAlgorithm so that
 * we can add our own data to it.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobAlgorithm
    : public IBatchJobAlgorithm,
      public MantidQt::API::ConfiguredAlgorithm {
public:
  BatchJobAlgorithm(
      Mantid::API::IAlgorithm_sptr algorithm,
      MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps properties,
      std::vector<std::string> outputWorkspaceProperties, Item *item);

  Item *item() override;
  std::vector<std::string> outputWorkspaceNames() const override;
  std::map<std::string, Mantid::API::Workspace_sptr>
  outputWorkspaceNameToWorkspace() const override;

private:
  // The data is an item in the table (i.e. a row or group)
  Item *m_item;
  std::vector<std::string> m_outputWorkspaceProperties;
};

using BatchJobAlgorithm_sptr = boost::shared_ptr<BatchJobAlgorithm>;
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBALGORITHM_H_
