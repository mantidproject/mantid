#ifndef MANTID_MDALGORITHMS_MDNORMSCD_H_
#define MANTID_MDALGORITHMS_MDNORMSCD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid
{
namespace DataObjects
{
  class EventWorkspace;
}
  namespace MDAlgorithms
  {
     
  /** MDNormSCD : Generate MD normalization for single crystal diffraction

     Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
    
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
  class DLLExport MDNormSCD  :public SlicingAlgorithm
    {
    public:
    MDNormSCD();
      
      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;
      
    private:
      void init();
      void exec();

      void cacheInputs();
      std::string inputEnergyMode() const;

      MDEvents::MDHistoWorkspace_sptr binInputWS();
      void createNormalizationWS(const MDEvents::MDHistoWorkspace & dataWS);
      std::vector<coord_t> getValuesFromOtherDimensions(bool & skipNormalization) const;
      Kernel::Matrix<coord_t> findIntergratedDimensions(const std::vector<coord_t> & otherDimValues,
                                                        bool & skipNormalization);
      void cacheDimensionXValues();
      void calculateNormalization(const std::vector<coord_t> &otherValues,
                                  const Kernel::Matrix<coord_t> &affineTrans);
      void calcIntegralsForIntersections( const std::vector<double> &xValues, const API::MatrixWorkspace &integrFlux, 
                                          size_t sp, std::vector<double> &yValues ) const;
      std::vector<detid_t> removeGroupedIDs(const API::ExperimentInfo & exptInfo,
                                            const std::vector<detid_t> &detIDs);
      Geometry::IDetector_const_sptr getThetaPhi(const detid_t detID,
                       const API::ExperimentInfo & exptInfo,
                       double &theta, double &phi);
      std::vector<Kernel::VMD> calculateIntersections(const double theta, const double phi);

      /// Normalization workspace
      MDEvents::MDHistoWorkspace_sptr m_normWS;
      /// Input workspace
      API::IMDEventWorkspace_sptr m_inputWS;
      /// limits for h,k,l dimensions
      coord_t m_hmin, m_hmax, m_kmin, m_kmax, m_lmin, m_lmax;
      /// flag for integrated h,k,l dimensions
      bool m_hIntegrated, m_kIntegrated, m_lIntegrated;
      /// (2*PiRUBW)^-1
      Mantid::Kernel::DblMatrix m_rubw;
      /// limits for momentum
      double m_kiMin, m_kiMax;
      ///index of h,k,l dimensions in the output workspaces
      size_t m_hIdx, m_kIdx, m_lIdx;
      /// cached X values along dimensions h,k,l
      std::vector<double> m_hX, m_kX, m_lX;
      /// Sample position
      Kernel::V3D m_samplePos;
      /// Beam direction
      Kernel::V3D m_beamDir;
    };

  } // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDNORMSCD_H_ */
