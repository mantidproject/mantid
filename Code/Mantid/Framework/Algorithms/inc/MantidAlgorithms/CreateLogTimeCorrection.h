#ifndef MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_
#define MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid
{
namespace Algorithms
{

  /** CreateLogTimeCorrection : Create correction file and workspace to correct event time against
    recorded log time for each pixel.

    It is assumed that the log time will be the same time as neutron arrives sample,
    and the input event workspace contains the neutron with time recorded at the detector.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport CreateLogTimeCorrection : public API::Algorithm
  {
  public:
    CreateLogTimeCorrection();
    virtual ~CreateLogTimeCorrection();

    virtual const std::string name() const {return "CreateLogTimeCorrection"; }
    virtual int version() const {return 1; }
    virtual const std::string category() const {return "Events\\EventFiltering"; }

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Implement abstract Algorithm methods
    void init();
    /// Implement abstract Algorithm methods
    void exec();

    API::MatrixWorkspace_sptr m_dataWS;
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CREATELOGTIMECORRECTION_H_ */
