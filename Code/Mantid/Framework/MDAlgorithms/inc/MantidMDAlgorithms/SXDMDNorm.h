#ifndef MANTID_MDALGORITHMS_SXDMDNORM_H_
#define MANTID_MDALGORITHMS_SXDMDNORM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
     
     SXDMDNorm : Generate MD normalization for single crystal diffraction

     Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    
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
    class DLLExport SXDMDNorm  : public SlicingAlgorithm
    {
    public:
      SXDMDNorm();
      
      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;
      
    private:
      void init();
      void exec();

      /// Retrieve the energy transfer mode of the input workspace data
      std::string inputEnergyMode() const;
      
      /// function to calculate intersections of teh trajectory with MDBoxes
      std::vector<Kernel::VMD> calculateIntersections(Mantid::Geometry::IDetector_const_sptr detector);

      /// number of MD dimensions
      size_t m_nDims;
      /// Normalization workspace
      MDEvents::MDHistoWorkspace_sptr m_normWS;
      /// Input workspace
      API::IMDEventWorkspace_sptr m_inputWS;
      ///limits for h,k,l dimensions
      coord_t hMin,hMax,kMin,kMax,lMin,lMax;
      ///flag for integrated h,k,l dimensions
      bool hIntegrated,kIntegrated,lIntegrated;
      ///(2*PiRUBW)^-1
      Mantid::Kernel::DblMatrix transf;
      /// limits for momentum
      double KincidentMin,KincidentMax;
      ///index of h,k,l dimensions in the output workspaces
      size_t hIndex,kIndex,lIndex;
      /// cached x values along dimensions h,k,l
      std::vector<double> m_hX, m_kX, m_lX;
    };

  } // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_SXDMDNORM_H_ */
