#include "MantidQtCustomInterfaces/ParameterisedLatticeView.h"
#include "MantidQtCustomInterfaces/LatticePresenter.h"
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpalette.h>
#include <QDoubleValidator>
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /// Constructor
    ParameterisedLatticeView::ParameterisedLatticeView(LatticePresenter* presenter) : m_presenter(presenter)
    {
      m_presenter->acceptView(this);
    }

    /**
    initalize the view with model data.
    @param a1: Lattice parameter indicating component in x
    @param a2: Lattice parameter indicating component in y
    @param a3: Lattice parameter indicating component in z
    @param b1: Lattice parameter giving alpha angle
    @param b2: Lattice parameter giving beta angle
    @param b3: Lattice parameter giving gamma angle
    @return false if invalid unit cell.
    */
    void ParameterisedLatticeView::initalize(double a1, double a2, double a3, double b1, double b2, double b3)
    {
      QGridLayout* layout = new QGridLayout();
      layout->addWidget(new QLabel("a"), 0, 0, Qt::AlignLeft);
      layout->addWidget(new QLabel("b"), 0, 2, Qt::AlignLeft);
      layout->addWidget(new QLabel("c"), 0, 4, Qt::AlignLeft);
      
      layout->addWidget(new QLabel("alpha"), 1, 0, Qt::AlignLeft);
      layout->addWidget(new QLabel("beta"), 1, 2, Qt::AlignLeft);
      layout->addWidget(new QLabel("gamma"), 1, 4, Qt::AlignLeft);

      m_a1 = createEditBox(a1);
      m_a2 = createEditBox(a2);
      m_a3 = createEditBox(a3);
      m_b1 = createEditBox(b1);
      m_b2 = createEditBox(b2);
      m_b3 = createEditBox(b3);

      layout->addWidget(m_a1, 0, 1, Qt::AlignLeft);
      layout->addWidget(m_a2, 0, 3, Qt::AlignLeft);
      layout->addWidget(m_a3, 0, 5, Qt::AlignLeft);
      layout->addWidget(m_b1, 1, 1, Qt::AlignLeft);
      layout->addWidget(m_b2, 1, 3, Qt::AlignLeft);
      layout->addWidget(m_b3, 1, 5, Qt::AlignLeft);

      m_pal = this->palette(); //Cache the default palette.
      this->setLayout(layout);
    }

    /**
    Create a standard edit box for each component.
    */
    QLineEdit* ParameterisedLatticeView::createEditBox(double value)
    {
      QLineEdit* box = new QLineEdit();
      box->setFixedWidth(50);
      box->setText(QString::number(value));
      QDoubleValidator* validator = new QDoubleValidator(0,100,4, box); //TODO 0 - 100 4dp. is this OK?
      box->setValidator(validator);
      connect(box, SIGNAL(editingFinished()), this, SLOT(edited()));
      return box;
    }

    /// Destructor
    ParameterisedLatticeView::~ParameterisedLatticeView()
    {
      this->layout();
    }

    /**
    Getter for a1
    @return a1
    */
    double ParameterisedLatticeView::getA1() const
    {
      return m_a1->text().toDouble();
    }

    /**
    Getter for a2
    @return a2
    */
    double ParameterisedLatticeView::getA2() const
    {
      return m_a2->text().toDouble();
    }

    /**
    Getter for a3
    @return a3
    */
    double ParameterisedLatticeView::getA3() const
    {
      return m_a3->text().toDouble();
    }

    /**
    Getter for b1
    @return b1
    */
    double ParameterisedLatticeView::getB1() const
    {
      return m_b1->text().toDouble();
    }

    /**
    Getter for b2
    @return b2
    */
    double ParameterisedLatticeView::getB2() const
    {
      return m_b2->text().toDouble();
    }

    /**
    Getter for b3
    @return a3
    */
    double ParameterisedLatticeView::getB3() const
    {
      return m_b3->text().toDouble();
    }

    /// Slot for edit box edited.
    void ParameterisedLatticeView::edited()
    {
      m_presenter->update(); // Feedback to the presenter
    }

    /// Indicate that a modification has been made
    void ParameterisedLatticeView::indicateModified()
    {
      QPalette pal = this->palette();
      pal.setColor(this->backgroundRole(), QColor(255, 191, 0));
      this->setAutoFillBackground(true);
      this->setPalette(pal);
    }

    /// Indicate that there are no pending modifications
    void ParameterisedLatticeView::indicateDefault()
    {
      this->setAutoFillBackground(true);
      this->setPalette(m_pal);
    }

    /// Indicate that the modifications/input are invalid.
    void ParameterisedLatticeView::indicateInvalid()
    {
      QPalette pal = this->palette();
      pal.setColor(this->backgroundRole(), QColor(255, 91, 0));
      this->setAutoFillBackground(true);
      this->setPalette(pal);
    }
  }
}