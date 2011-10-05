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