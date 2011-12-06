#ifndef MANTID_ALGORITHMS_VULCANFILTEREVENTS_H_
#define MANTID_ALGORITHMS_VULCANFILTEREVENTS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

  /** FilterEventsHighFrequency : TODO: DESCRIPTION
    
    @date 2011-11-29

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport FilterEventsHighFrequency : public API::Algorithm
  {
  public:
    FilterEventsHighFrequency();
    virtual ~FilterEventsHighFrequency();
    
    virtual const std::string name() const {return "FilterEventsHighFrequency"; };
    virtual int version() const {return 1; };
    virtual const std::string category() const {return "Diffraction"; };

  private:

    DataObjects::EventWorkspace_const_sptr eventWS;
    DataObjects::Workspace2D_const_sptr seWS;
    DataObjects::EventWorkspace_sptr outputWS;

    std::vector<detid_t> mCalibDetectorIDs;
    std::vector<double> mCalibOffsets;

    virtual void initDocs();

    void init();

    void exec();

    void importCalibrationFile(std::string calfilename);

    void createEventWorkspace();

    void filterEvents();

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_VULCANFILTEREVENTS_H_ */
