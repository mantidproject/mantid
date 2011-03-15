#ifndef PARAVIEWPROGRESSACTION_H_
#define PARAVIEWPROGRESSACTION_H_

#include "MantidVatesAPI/ProgressAction.h"
#include "vtkRebinningCutter.h"

/** Adapter for action specific to ParaView RebinningCutter filter. Handles progress actions raised by underlying Mantid Algorithms.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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


namespace Mantid
{
namespace VATES
{

class ParaViewProgressAction : public ProgressAction
{

public:

  ParaViewProgressAction(vtkRebinningCutter* filter) : m_filter(filter)
  {
  }

  virtual void eventRaised(int progressPercent)
  {
    m_filter->UpdateAlgorithmProgress(progressPercent);
  }

private:

  vtkRebinningCutter* m_filter;
};

}
}

#endif /* PARAVIEWPROGRESSACTION_H_ */
