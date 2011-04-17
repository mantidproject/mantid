#ifndef ESCALATING_REBINNINGACTIONMANAGER_H_
#define ESCALATING_REBINNINGACTIONMANAGER_H_

/**
 * Encapsulates knowledge about switching between rebinning action states.
 * Knows when to escalate a decision to a higher level (i.e, from effectivley do nothing to
 * trigger a full rebin based on the current state and the request made).
 * Simple stragegy pattern, may be one of may used.
 *
 @author Owen Arnold, Tessella ISIS
 @date 16/04/2011

 Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

#include "MantidVatesAPI/RebinningActionManager.h"
namespace Mantid
{
  namespace VATES
  {
    class DLLExport EscalatingRebinningActionManager : public RebinningActionManager
    {
    private:
      RebinningIterationAction m_currentAction;
    public:
      EscalatingRebinningActionManager();
      virtual ~EscalatingRebinningActionManager();
      virtual void ask(RebinningIterationAction requestedAction);
      virtual RebinningIterationAction action() const;
      virtual void reset();
    private:
      EscalatingRebinningActionManager(const EscalatingRebinningActionManager&);
      EscalatingRebinningActionManager& operator=(const EscalatingRebinningActionManager&);
    };
  }
}

#endif
