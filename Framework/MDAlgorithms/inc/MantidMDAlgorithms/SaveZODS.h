// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_SAVEZODS_H_
#define MANTID_MDALGORITHMS_SAVEZODS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Save a MDHistoWorkspace to a HDF5 format for use with the ZODS
 * analysis software.

  @date 2012-01-27
*/
class DLLExport SaveZODS : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a MDHistoWorkspace in HKL space to a HDF5 format for use with "
           "the ZODS analysis software.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"SaveMD"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SAVEZODS_H_ */
