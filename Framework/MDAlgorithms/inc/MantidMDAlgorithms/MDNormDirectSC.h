// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/MDNormBase.h"

namespace Mantid::MDAlgorithms {

/** MDNormSCD : Generate MD normalization for single crystal diffraction
 */
class MANTID_MDALGORITHMS_DLL MDNormDirectSC : public MDNormBase {
public:
  MDNormDirectSC() {};

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MDNormSCD", "MDNormSCDPreprocessIncoherent"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  void cacheInputs();
};

} // namespace Mantid::MDAlgorithms
