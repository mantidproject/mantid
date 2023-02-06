// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt::API {

class EXPORT_OPT_MANTIDQT_COMMON IAsyncAlgorithmSubscriber {

public:
  virtual void notifyAlgorithmFinished(bool success) = 0;
  virtual void notifyAlgorithmProgress(double const progress, std::string const &message) = 0;
};

} // namespace MantidQt::API
