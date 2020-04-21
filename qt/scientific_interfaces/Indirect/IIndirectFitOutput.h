// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "Indextypes.h"
#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFitOutput {
public:
  virtual void
  addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fitAlgorithm,
                     TableDatasetIndex index) = 0;
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt