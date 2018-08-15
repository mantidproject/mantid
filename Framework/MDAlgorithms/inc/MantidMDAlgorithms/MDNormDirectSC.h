#ifndef MANTID_MDALGORITHMS_MDNORMDIRECTSC_H_
#define MANTID_MDALGORITHMS_MDNORMDIRECTSC_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid {
namespace DataObjects {
class EventWorkspace;
}
namespace MDAlgorithms {

/** MDNormSCD : Generate MD normalization for single crystal diffraction

   Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

   This file is part of Mantid.

   Mantid is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Mantid is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   File change history is stored at: <https://github.com/mantidproject/mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport MDNormDirectSC : public SlicingAlgorithm {
public:
  MDNormDirectSC();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MDNormSCD", "MDNormSCDPreprocessIncoherent"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  void cacheInputs();
  std::string inputEnergyMode() const;

  DataObjects::MDHistoWorkspace_sptr binInputWS();
  void createNormalizationWS(const DataObjects::MDHistoWorkspace &dataWS);
  std::vector<coord_t>
  getValuesFromOtherDimensions(bool &skipNormalization,
                               uint16_t expInfoIndex = 0) const;
  Kernel::Matrix<coord_t>
  findIntergratedDimensions(const std::vector<coord_t> &otherDimValues,
                            bool &skipNormalization);
  void cacheDimensionXValues();
  void calculateNormalization(const std::vector<coord_t> &otherValues,
                              const Kernel::Matrix<coord_t> &affineTrans,
                              uint16_t expInfoIndex);

  void calculateIntersections(std::vector<std::array<double, 4>> &intersections,
                              const double theta, const double phi);

  /// Normalization workspace
  DataObjects::MDHistoWorkspace_sptr m_normWS;
  /// Input workspace
  API::IMDEventWorkspace_sptr m_inputWS;
  /// limits for h,k,l, dE dimensions
  coord_t m_hmin, m_hmax, m_kmin, m_kmax, m_lmin, m_lmax, m_dEmin, m_dEmax;
  /// cached values for incident energy and momentum, final momentum min/max
  double m_Ei, m_ki, m_kfmin, m_kfmax;
  /// flag for integrated h,k,l, dE dimensions
  bool m_hIntegrated, m_kIntegrated, m_lIntegrated, m_dEIntegrated;
  /// (2*PiRUBW)^-1
  Mantid::Kernel::DblMatrix m_rubw;
  /// index of h,k,l, dE dimensions in the output workspaces
  size_t m_hIdx, m_kIdx, m_lIdx, m_eIdx;
  /// cached X values along dimensions h,k,l. dE
  std::vector<double> m_hX, m_kX, m_lX, m_eX;
  /// Sample position
  Kernel::V3D m_samplePos;
  /// Beam direction
  Kernel::V3D m_beamDir;
  /// ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;
  /// internal flag to accumulate to an existing workspace
  bool m_accumulate{false};
  /// number of experiment infos
  uint16_t m_numExptInfos;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MDNORMDIRECTSC_H_ */
