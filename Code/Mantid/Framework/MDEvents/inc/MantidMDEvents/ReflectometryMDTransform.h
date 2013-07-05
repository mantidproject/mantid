#ifndef MANTID_MDEVENTS_REFLECTOMETRYMDTRANFORM_H_
#define MANTID_MDEVENTS_REFLECTOMETRYMDTRANFORM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidAPI/BoxController.h"

namespace Mantid
{
namespace MDEvents
{

  /** ReflectometryMDTransform : Abstract type for reflectometry transforms to MDWorkspaces. This is a Strategy Design Pattern. 
    
    @date 2012-05-29

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ReflectometryMDTransform 
  {

  private:

    Mantid::API::BoxController_sptr m_boxController;

  protected:

  const size_t m_nbinsx;
  const size_t m_nbinsz;



  boost::shared_ptr<MDEventWorkspace2Lean> createWorkspace(Mantid::Geometry::IMDDimension_sptr, Mantid::Geometry::IMDDimension_sptr) const;

  public:
    //Execute the strategy to produce the a transformed, output MDWorkspace
    virtual Mantid::API::IMDEventWorkspace_sptr execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const = 0;

    virtual ~ReflectometryMDTransform();
    ReflectometryMDTransform(Mantid::API::BoxController_sptr boxController);
  };
}
}
#endif
