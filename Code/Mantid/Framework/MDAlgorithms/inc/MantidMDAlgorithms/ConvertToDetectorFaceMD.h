#ifndef MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMD_H_
#define MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDLeanEvent.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Convert a MatrixWorkspace containing to a MD workspace for
   * viewing the detector face.
    
    @date 2012-03-08

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ConvertToDetectorFaceMD  :
                  public Mantid::MDEvents::BoxControllerSettingsAlgorithm
  {
  public:
    ConvertToDetectorFaceMD();
    virtual ~ConvertToDetectorFaceMD();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Convert a MatrixWorkspace containing to a MD workspace for viewing the detector face.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    void init();
    void exec();

    std::map<int, Geometry::RectangularDetector_const_sptr> getBanks();

    template <class T, class MDE, size_t nd>
    void convertEventList(boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<MDE, nd> > outWS,
        size_t workspaceIndex, coord_t x, coord_t y, coord_t bankNum,
        uint16_t runIndex, int32_t detectorID);

    /// The input event workspace
    Mantid::DataObjects::EventWorkspace_sptr in_ws;

    /// Width in pixels of the widest detector
    int m_numXPixels;
    /// Height in pixels of the widest detector
    int m_numYPixels;

    // Map between the detector ID and the workspace index
    std::vector<size_t> m_detID_to_WI;
    detid_t m_detID_to_WI_offset;
  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMD_H_ */
