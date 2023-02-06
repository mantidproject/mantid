// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt::API {

class IAsyncAlgorithmSubscriber;

class EXPORT_OPT_MANTIDQT_COMMON IAsyncAlgorithmRunner {

public:
  virtual void subscribe(IAsyncAlgorithmSubscriber *subscriber) = 0;

  // virtual void cancelRunningAlgorithm() = 0;

  // virtual void startAlgorithm(Mantid::API::IAlgorithm_sptr alg) = 0;
  // virtual Mantid::API::IAlgorithm_sptr getAlgorithm() const = 0;
};

} // namespace MantidQt::API
