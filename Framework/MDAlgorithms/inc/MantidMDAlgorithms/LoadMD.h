// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_LOADMD_H_
#define MANTID_MDALGORITHMS_LOADMD_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** Load a .nxs file into a MDEventWorkspace.

  @author Janik Zikovsky
  @date 2011-07-12
*/
class DLLExport LoadMD : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadMD();

  /// Algorithm's name for identification
  const std::string name() const override { return "LoadMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a MDEventWorkspace in .nxs format.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveMD"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\DataHandling";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;

  /// Helper method
  template <typename MDE, size_t nd>
  void doLoad(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  void loadExperimentInfos(
      boost::shared_ptr<Mantid::API::MultipleExperimentInfos> ws);

  void loadSlab(std::string name, void *data,
                DataObjects::MDHistoWorkspace_sptr ws,
                NeXus::NXnumtype dataType);
  void loadHisto();

  void loadDimensions();

  void loadDimensions2();

  void loadCoordinateSystem();

  void loadQConvention();

  void loadVisualNormalization(
      const std::string &key,
      boost::optional<Mantid::API::MDNormalization> &normalization);

  /// Load all the affine matricies
  void loadAffineMatricies(API::IMDWorkspace_sptr ws);
  /// Load a given affine matrix
  API::CoordTransform *loadAffineMatrix(std::string entry_name);

  /// Sets MDFrames for workspaces from legacy files
  void setMDFrameOnWorkspaceFromLegacyFile(API::IMDWorkspace_sptr ws);

  /// Checks if a worspace is a certain type of legacy file
  void checkForRequiredLegacyFixup(API::IMDWorkspace_sptr ws);

  /// Negative scaling for Q dimensions
  std::vector<double> qDimensions(API::IMDWorkspace_sptr ws);

  /// Open file handle
  // clang-format off
  boost::scoped_ptr< ::NeXus::File> m_file;
  // clang-format on

  /// Name of that file
  std::string m_filename;

  /// Number of dimensions in loaded file
  size_t m_numDims;

  /// Each dimension object loaded.
  std::vector<Mantid::Geometry::IMDDimension_sptr> m_dims;
  /// Coordinate system
  Kernel::SpecialCoordinateSystem m_coordSystem;
  /// QConvention
  std::string m_QConvention;
  /// load only the box structure with empty boxes but do not tload boxes events
  bool m_BoxStructureAndMethadata;

  /// Version of SaveMD used to save the file
  int m_saveMDVersion;

  /// Visual normalization
  boost::optional<Mantid::API::MDNormalization> m_visualNormalization;
  boost::optional<Mantid::API::MDNormalization> m_visualNormalizationHisto;

  /// Named entry
  static const std::string VISUAL_NORMALIZATION_KEY;
  static const std::string VISUAL_NORMALIZATION_KEY_HISTO;

  /// MDFrame correction flag
  bool m_requiresMDFrameCorrection;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADMD_H_ */
