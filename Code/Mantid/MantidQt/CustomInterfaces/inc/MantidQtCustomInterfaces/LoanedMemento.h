#ifndef MANTID_CUSTOMINTERFACES_LOANEDMEMENTO_H_
#define MANTID_CUSTOMINTERFACES_LOANEDMEMENTO_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class WorkspaceMemento;

    /** @class LoanedMemento

    Smart pointer acting as API for WorkspaceMemento. Has objectives.

    i) Provide automatic locking/unlocking
    ii) Ensure that the objects handed out are treated as loans. They opt out of RAII and are not deleted on destruction.
    iii) Expose full API of underlying WorkspaceMemento.

    @author Owen Arnold
    @date 30/08/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoanedMemento
    {
    public:
      LoanedMemento (WorkspaceMemento *memento);
      LoanedMemento(const LoanedMemento& other);
      LoanedMemento& operator=(LoanedMemento& other);
      WorkspaceMemento * operator -> () ;
      ~LoanedMemento () ;
    private:
      WorkspaceMemento* m_memento;
    };
  }
}

#endif