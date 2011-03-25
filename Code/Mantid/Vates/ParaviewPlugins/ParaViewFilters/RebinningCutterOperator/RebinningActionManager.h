#ifndef REBINNINGACTIONMANAGER_H_
#define REBINNINGACTIONMANAGER_H_

/**
 * Handling changes that may or may not trigger rebinning is getting out of hand. A better approach is for the code to naively request a particular action
 * this type will handle most, severe wins. This reduces the amount of state checking and overwriting required in the visualisation plugins.

 @author Owen Arnold, Tessella ISIS
 @date 18/03/2011

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

#include "MantidVatesAPI/RebinningCutterPresenter.h"
namespace Mantid
{
  namespace VATES
  {
    class DLLExport RebinningActionManger
    {
    private:
      RebinningIterationAction m_currentAction;
      bool m_forced;
    public:
      RebinningActionManger() : m_currentAction(UseCache), m_forced(false)
      {
      }
      void ask(RebinningIterationAction requestedAction)
      {
        //Very simply, only allow escalation if the requested action is more 'severe' than the current one.
        if((false == m_forced) && (requestedAction > m_currentAction))
        {
          m_currentAction = requestedAction;
        }
      }
      void force(RebinningIterationAction requestedAction)
      {
        m_currentAction = requestedAction;
        m_forced = true;
      }
      RebinningIterationAction action() const
      {
        return m_currentAction;
      }
      void reset()
      {
        m_currentAction = UseCache;
        m_forced = false;
      }
    };
  }
}

#endif /* REBINNINGACTIONMANAGER_H_ */
