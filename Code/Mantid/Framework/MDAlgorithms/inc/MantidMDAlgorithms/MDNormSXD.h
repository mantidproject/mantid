#ifndef MANTID_MDALGORITHMS_MDNORMSXD_H_
#define MANTID_MDALGORITHMS_MDNORMSXD_H_

#include "MantidKernel/System.h"
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

  /** MDNormSXD : Generate MD normalization for single crystal diffraction

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
  class DLLExport MDNormSXD  :public SlicingAlgorithm
  {
  public:
    MDNormSXD();
    virtual ~MDNormSXD();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

  private:
    void init();
    void exec();

    /// function to calculate intersections of teh trajectory with MDBoxes
    std::vector<Mantid::Kernel::VMD> calculateIntersections(Mantid::Geometry::IDetector_const_sptr detector);
    /// Integrate flux spectra
    void integrateFlux( const DataObjects::EventWorkspace& flux, API::MatrixWorkspace &integrFlux );
    /// Use interpolation to calculate integrals
    void calcIntegralsForIntersections( const std::vector<double> &xValues, const API::MatrixWorkspace &integrFlux, size_t sp, std::vector<double> &yValues ) const;

    /// number of MD dimensions
    size_t m_nDims;
    /// Normalization workspace
    Mantid::MDEvents::MDHistoWorkspace_sptr m_normWS;
    /// Input workspace
    Mantid::API::IMDEventWorkspace_sptr m_inputWS;
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

#endif  /* MANTID_MDALGORITHMS_MDNORMSXD_H_ */
