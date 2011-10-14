#ifndef MANTIDQTCUSTOMINTERFACES_LOG_PRESENTER_H
#define MANTIDQTCUSTOMINTERFACES_LOG_PRESENTER_H

#include "MantidQtCustomInterfaces/Updateable.h"
#include "MantidQtCustomInterfaces/LogView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** Presenter of MVP type for controlling interaction of log view with WorkspaceMementos.
  
      @author Owen Arnold, RAL ISIS
      @date 14/Oct/2011

      Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class LoanedMemento;
    class DLLExport LogPresenter : public Updateable
    {
    public:
      LogPresenter(LoanedMemento& memento);
      ~LogPresenter();
      void update();
      void acceptView(LogView* view);
    private:
      LogView* m_view;
      LoanedMemento& m_WsMemento;
    };
  }
}
#endif