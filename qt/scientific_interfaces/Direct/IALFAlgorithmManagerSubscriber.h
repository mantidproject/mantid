// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt::CustomInterfaces {

enum class ALFDataType { SAMPLE, VANADIUM };

class MANTIDQT_DIRECT_DLL IALFAlgorithmManagerSubscriber {

public:
  virtual ~IALFAlgorithmManagerSubscriber() = default;

  virtual void notifyAlgorithmError(std::string const &message) = 0;

  virtual void notifyLoadAndNormaliseComplete(ALFDataType const &dataType,
                                              Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
};

} // namespace MantidQt::CustomInterfaces
