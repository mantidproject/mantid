#ifndef MANTIDQTCUSTOMINTERFACES_LATTICE_PRESENTER_H
#define MANTIDQTCUSTOMINTERFACES_LATTICE_PRESENTER_H

#include "MantidQtCustomInterfaces/Updateable.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LatticeView;
    class LoanedMemento;
    class LatticePresenter : public Updateable
    {
    public:
      LatticePresenter(LoanedMemento& memento);
      ~LatticePresenter();
      void update();
      void acceptView(LatticeView* view);
    private:
      void checkInput(double a1, double a2, double a3, double b1, double b2, double b3);
      LatticeView* m_view;
      LoanedMemento& m_WsMemento;
    };
  }
}
#endif