// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class PreviewRow;

class MANTIDQT_ISISREFLECTOMETRY_DLL JobManagerSubscriber {
public:
  virtual ~JobManagerSubscriber() = default;

  virtual void notifyLoadWorkspaceCompleted() = 0;
  virtual void notifySumBanksCompleted() = 0;
  virtual void notifyReductionCompleted() = 0;

  virtual void notifyLoadWorkspaceAlgorithmError() = 0;
  virtual void notifySumBanksAlgorithmError() = 0;
  virtual void notifyReductionAlgorithmError() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IJobManager {
public:
  virtual ~IJobManager() = default;

  virtual void subscribe(JobManagerSubscriber *notifyee) = 0;
  virtual void startPreprocessing(PreviewRow &row) = 0;
  virtual void startSumBanks(PreviewRow &row) = 0;
  virtual void startReduction(PreviewRow &row) = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
