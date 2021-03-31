// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtCustomInterfaces/Updateable.h"

namespace MantidQt {
namespace CustomInterfaces {
class LatticeView;
class LoanedMemento;

/** Presenter of MVP type for controlling interaction of lattice view with
  WorkspaceMementos.

  @author Owen Arnold, RAL ISIS
  @date 06/Oct/2011
 */
class DLLExport LatticePresenter : public Updateable {
public:
  LatticePresenter(LoanedMemento &memento);
  ~LatticePresenter();
  void update();
  void acceptView(LatticeView *view);

private:
  bool checkInput(double a1, double a2, double a3, double b1, double b2, double b3);
  LatticeView *m_view;
  LoanedMemento &m_WsMemento;
};
} // namespace CustomInterfaces
} // namespace MantidQt