#include "MantidQtMantidWidgets/SlitCalculator.h"
#include <qmath.h>

namespace MantidQt
{
  namespace MantidWidgets
  {
    SlitCalculator::SlitCalculator(QWidget *parent)
    {
      Q_UNUSED(parent);
      ui.setupUi(this);
      on_recalculate_triggered();
    }

    SlitCalculator::~SlitCalculator()
    {
    }

    void SlitCalculator::on_recalculate_triggered()
    {
      //Gather input
      const double PI = 2*qAsin(1);
      const double s1s2 = ui.spinSlit1Slit2->value();
      const double s2sa = ui.spinSlit2Sample->value();
      const double res = ui.spinResolution->value();
      const double footprint = ui.spinFootprint->value();
      const double angle = ui.spinAngle->value();

      //Calculate values
      const double s1 = 2 * (s1s2 + s2sa) * qTan(res * angle * PI / 180) - footprint * qSin(angle * PI / 180);
      const double s2 = s1s2 * (footprint * qSin(angle * PI / 180) + s1) / (s1s2 + s2sa) - s1;

      //Update output
      ui.slit1Text->setText(QString::number(s1,'f',3));
      ui.slit2Text->setText(QString::number(s2,'f',3));
    }
  }
}
