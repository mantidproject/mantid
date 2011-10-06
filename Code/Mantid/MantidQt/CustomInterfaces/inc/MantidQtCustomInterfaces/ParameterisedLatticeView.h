#ifndef MANTIDQTCUSTOMINTERFACES_PARAMETERISED_LATTICE_VIEW_H_
#define MANTIDQTCUSTOMINTERFACES_PARAMETERISED_LATTICE_VIEW_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/LatticeView.h"
#include <boost/scoped_ptr.hpp>
#include <qwidget.h>

class QLineEdit;
namespace MantidQt
{
  namespace CustomInterfaces
  {
    class LatticePresenter;

    /** Concrete LatticeView as a QtWidget 
  
      @author Owen Arnold, RAL ISIS
      @date 06/Oct/2011

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
    class ParameterisedLatticeView : public QWidget, public LatticeView
    {
      Q_OBJECT

    private:

      QLineEdit* m_a1;
      QLineEdit* m_a2;
      QLineEdit* m_a3;
      QLineEdit* m_b1;
      QLineEdit* m_b2;
      QLineEdit* m_b3;

      /// MVP presenter
      boost::scoped_ptr<LatticePresenter> m_presenter;

      /// Helper method to create an edit box with standard format and signal-slots.
      QLineEdit* createEditBox(double value);

      /// Default/Cached palette.
      QPalette m_pal;

    private slots:

      void edited();

    public:

      /// Constructor
      ParameterisedLatticeView(LatticePresenter* presenter);

      /// Destructor
      ~ParameterisedLatticeView();

      /// Indicate that the view has been modified.
      void indicateModified();

      /// Indicate that the view is unmodified.
      void indicateDefault();

      /// Indicate that the view is invalid.
      void indicateInvalid();

      /// Initalization method.
      void initalize(double a1, double a2, double a3, double b1, double b2, double b3);

      virtual double getA1() const;
      virtual double getA2() const;
      virtual double getA3() const;
      virtual double getB1() const;
      virtual double getB2() const;
      virtual double getB3() const;

    };
  }
}

#endif