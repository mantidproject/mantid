#include "MantidQtCustomInterfaces/LatticePresenter.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"
#include "MantidQtCustomInterfaces/LatticeView.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"

namespace MantidQt {
namespace CustomInterfaces {
/// Constructor
LatticePresenter::LatticePresenter(LoanedMemento &memento)
    : m_WsMemento(memento) {}

/// Destructor
LatticePresenter::~LatticePresenter() {
  // Explicitly DO NOT manage/release view as view owns presenter. Presenter is
  // view visitor.
}

/**
Accept the lattice view and configure it.
*/
void LatticePresenter::acceptView(LatticeView *view) {
  m_view = view;
  double a1, a2, a3, b1, b2, b3;

  WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
  a1 = service.getA1();
  a2 = service.getA2();
  a3 = service.getA3();
  b1 = service.getB1();
  b2 = service.getB2();
  b3 = service.getB3();

  checkInput(a1, a2, a3, b1, b2, b3);
  m_view->initalize(a1, a2, a3, b1, b2, b3);
}

/**
Check that the inputs are okay. If not then pass up to view and return false.
@param a1: Lattice parameter indicating component in x
@param a2: Lattice parameter indicating component in y
@param a3: Lattice parameter indicating component in z
@param b1: Lattice parameter giving alpha angle
@param b2: Lattice parameter giving beta angle
@param b3: Lattice parameter giving gamma angle
@return false if invalid unit cell.
*/
bool LatticePresenter::checkInput(double a1, double a2, double a3, double b1,
                                  double b2, double b3) {
  bool parametersOk = false;
  try {
    Mantid::Geometry::OrientedLattice lattice(a1, a2, a3, b1, b2, b3);
    parametersOk = true;
  } catch (std::range_error &) {
    m_view->indicateInvalid();
  }
  return parametersOk;
}

/**
Update method, externally triggered.
*/
void LatticePresenter::update() {
  double a1 = m_view->getA1();
  double a2 = m_view->getA2();
  double a3 = m_view->getA3();
  double b1 = m_view->getB1();
  double b2 = m_view->getB2();
  double b3 = m_view->getB3();

  WorkspaceMementoService<LoanedMemento> service(m_WsMemento);
  service.setLatticeParameters(a1, a2, a3, b1, b2, b3);

  if (checkInput(a1, a2, a3, b1, b2, b3)) {
    if (m_WsMemento->hasChanged()) {
      m_view->indicateModified();
    } else {
      m_view->indicateDefault();
    }
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt