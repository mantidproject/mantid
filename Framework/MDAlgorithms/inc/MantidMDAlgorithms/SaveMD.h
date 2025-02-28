// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {

namespace MDAlgorithms {

/** Save a MDEventWorkspace to a .nxs file.

  @author Janik Zikovsky
  @date 2011-07-11
*/
class MANTID_MDALGORITHMS_DLL SaveMD final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a MDEventWorkspace or MDHistoWorkspace to a .nxs file."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\DataHandling"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Helper method
  template <typename MDE, size_t nd> void doSaveEvents(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Save the MDHistoWorkspace.
  void doSaveHisto(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws);

  /// Save all the affine matricies
  void saveAffineTransformMatricies(::NeXus::File *const file, API::IMDWorkspace_const_sptr ws);
  /// Save a given affine matrix
  void saveAffineTransformMatrix(::NeXus::File *const file, API::CoordTransform *transform, std::string entry_name);
  /// Save a generic matrix
  template <typename T>
  void saveMatrix(::NeXus::File *const file, std::string name, Kernel::Matrix<T> &m, NXnumtype type,
                  std::string tag = "");
};

} // namespace MDAlgorithms
} // namespace Mantid
