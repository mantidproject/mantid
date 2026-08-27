// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidMDAlgorithms/MDNormBase.h"

namespace Mantid {
namespace MDAlgorithms {

/** MDNormalization : Bin single crystal diffraction or direct geometry
 * inelastic data and calculate the corresponding statistical weight
 */
class MANTID_MDALGORITHMS_DLL MDNorm final : public MDNormBase {
public:
  MDNorm();
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspaceForMDNorm", "MDNormSCD", "MDNormDirectSC", "RecalculateTrajectoriesExtents",
            "ConvertHFIRSCDtoMDE"};
  }

private:
  void init() override;
  void exec() override;
  void validateBinningForTemporaryDataWorkspace(const std::map<std::string, std::string> &,
                                                const Mantid::API::IMDHistoWorkspace_sptr &);
  std::map<std::string, std::string> validateInputs() override final;
  std::string QDimensionName(std::vector<double> projection);
  std::string QDimensionNameQSample(int i);
  std::map<std::string, std::string> getBinParameters();
  void createNormalizationWS(const DataObjects::MDHistoWorkspace &dataWS);
  void createBackgroundNormalizationWS(const DataObjects::MDHistoWorkspace &dataWS);

  /// Bin(MD) input MDE workspace
  DataObjects::MDHistoWorkspace_sptr binInputWS(const std::vector<Geometry::SymmetryOperation> &symmetryOps);

  /// Bin(MD), per symmetry operation, an MDEventWorkspace using pre-computed bin parameters.
  /// Shared by binInputWS and (for monochromatic-SCD input) binMonoSCDNormalizationWS, so that data
  /// and normalization end up on identical grids.
  DataObjects::MDHistoWorkspace_sptr binMDEventWorkspace(const API::IMDEventWorkspace_sptr &ws,
                                                         const std::string &temporaryWSPropertyName,
                                                         const std::string &outputWSPropertyName,
                                                         const std::vector<Geometry::SymmetryOperation> &symmetryOps,
                                                         const std::map<std::string, std::string> &parameters);

  /// Bin(MD) input Background workspace
  DataObjects::MDHistoWorkspace_sptr binBackgroundWS(const std::vector<Geometry::SymmetryOperation> &symmetryOps);

  /// Bin(MD) MonoSCDNormalizationWorkspace (monochromatic single crystal diffraction)
  DataObjects::MDHistoWorkspace_sptr
  binMonoSCDNormalizationWS(const std::vector<Geometry::SymmetryOperation> &symmetryOps);

  /// build symmetry matrix
  Mantid::Kernel::DblMatrix buildSymmetryMatrix(const Geometry::SymmetryOperation &so);
  void determineBasisVector(const size_t &qindex, const std::string &value, const Kernel::DblMatrix &Qtransform,
                            std::vector<double> &projection, std::stringstream &basisVector,
                            std::vector<size_t> &qDimensionIndices);
  inline void setQUnit(const std::vector<size_t> &qDimensionIndices,
                       const Mantid::DataObjects::MDHistoWorkspace_sptr &outputMDHWS);

  std::vector<coord_t> getValuesFromOtherDimensions(bool &skipNormalization, uint16_t expInfoIndex = 0) const;

  void cacheDimensionXValues();
  void calculateNormalization(const std::vector<coord_t> &otherValues, const Geometry::SymmetryOperation &so,
                              uint16_t expInfoIndex, size_t soIndex);

  void calcDiffractionIntersectionIntegral(std::vector<std::array<double, 4>> &intersections,
                                           std::vector<double> &xValues, std::vector<double> &yValues,
                                           const API::MatrixWorkspace &integrFlux, const size_t &wsIdx);

  void calcSingleDetectorNorm(const std::vector<std::array<double, 4>> &intersections, const double &solid,
                              std::vector<double> &yValues, const size_t &vmdDims, std::vector<coord_t> &pos,
                              std::vector<coord_t> &posNew, std::vector<std::atomic<signal_t>> &signalArray,
                              const double &solidBkgd, std::vector<std::atomic<signal_t>> &bkgdSignalArray);

  API::IMDWorkspace_sptr divideMD(const API::IMDHistoWorkspace_sptr &lhs, const API::IMDHistoWorkspace_sptr &rhs,
                                  const std::string &outputwsname, const double &startProgress,
                                  const double &endProgress);

  /// Normalization workspace
  DataObjects::MDHistoWorkspace_sptr m_bkgdNormWS;
  /// Input background workspace
  API::IMDEventWorkspace_sptr m_backgroundWS;

  /// flag for reciprocal lattice units
  bool m_isRLU;
  /// The projection vectors
  std::vector<double> m_Q0Basis{1., 0., 0.}, m_Q1Basis{0., 1., 0.}, m_Q2Basis{0., 0., 1.};
  /** matrix for transforming from intersections to positions in the
  normalization workspace */
  Mantid::Kernel::Matrix<coord_t> m_transformation;
  /// number of symmetry operations
  size_t m_numSymmOps;
  /// Flag indicating a pre-computed MonoSCDNormalizationWorkspace was provided
  /// (monochromatic single crystal diffraction, e.g. WAND, DEMAND)
  bool m_monochromatic;
  /// Flag to accumulate normalization
  bool m_accumulate;
};

} // namespace MDAlgorithms
} // namespace Mantid
