#ifndef MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_
#define MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidAPI/IMDWorkspace.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    class IPeak;
    class IMDHistoWorkspace;
    class IMDEventWorkspace;
    class PeakTransform;
  }
namespace Crystal
{

  /** PeakClusterProjection : Maps peaks onto IMDHistoWorkspaces and returns the signal value at the peak center.
    
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
  class DLLExport PeakClusterProjection 
  {
  public:
    /// Constructor
    PeakClusterProjection(boost::shared_ptr<Mantid::API::IMDWorkspace>& mdWS) ;
    /// Constructor
    PeakClusterProjection(boost::shared_ptr<Mantid::API::IMDHistoWorkspace>& mdWS) ;
    /// Constructor
    PeakClusterProjection(boost::shared_ptr<Mantid::API::IMDEventWorkspace>& mdWS) ;
    /// Get the signal value at the peak center
    Mantid::signal_t signalAtPeakCenter(const Mantid::API::IPeak& peak, Mantid::API::MDNormalization normalization = Mantid::API::NoNormalization) const;
    /// Get the peak center
    Mantid::Kernel::V3D peakCenter(const Mantid::API::IPeak& peak) const;
    /// Destructor
    virtual ~PeakClusterProjection();
  private:
    // Disabled copy and assignment.
    PeakClusterProjection(const PeakClusterProjection&);
    PeakClusterProjection& operator=(const PeakClusterProjection&);
    
    /// Image
    boost::shared_ptr<Mantid::API::IMDWorkspace> m_mdWS;

    /// Peak Transform
    boost::shared_ptr<Mantid::API::PeakTransform> m_peakTransform;

  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_ */
