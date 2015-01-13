#ifndef MANTIDQTCUSTOMINTERFACES_LATTICE_PRESENTER_H
#define MANTIDQTCUSTOMINTERFACES_LATTICE_PRESENTER_H

#include "MantidQtCustomInterfaces/Updateable.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LatticeView;
    class LoanedMemento;


    /** Presenter of MVP type for controlling interaction of lattice view with WorkspaceMementos.
  
      @author Owen Arnold, RAL ISIS
      @date 06/Oct/2011

      Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport LatticePresenter : public Updateable
    {
    public:
      LatticePresenter(LoanedMemento& memento);
      ~LatticePresenter();
      void update();
      void acceptView(LatticeView* view);
    private:
      bool checkInput(double a1, double a2, double a3, double b1, double b2, double b3);
      LatticeView* m_view;
      LoanedMemento& m_WsMemento;
    };
  }
}
#endif