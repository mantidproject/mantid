#include "MantidQtMantidWidgets/SlitCalculator.h"
#include "MantidAPI/AlgorithmManager.h"
#include <math.h>

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
      const double s1s2 = ui.spinSlit1Slit2->value();
      const double s2sa = ui.spinSlit2Sample->value();
      const double res = ui.spinResolution->value();
      const double footprint = ui.spinFootprint->value();
      const double angle = ui.spinAngle->value();

      //Calculate values
      Mantid::API::IAlgorithm_sptr algSlit = Mantid::API::AlgorithmManager::Instance().create("CalculateSlits");
      algSlit->initialize();
      algSlit->setChild(true);
      algSlit->setProperty("Slit1Slit2", s1s2);
      algSlit->setProperty("Slit2SA", s2sa);
      algSlit->setProperty("Resolution", res);
      algSlit->setProperty("Footprint", footprint);
      algSlit->setProperty("Angle", angle);
      algSlit->execute();

      const double s1 = algSlit->getProperty("Slit1");
      const double s2 = algSlit->getProperty("Slit2");

      //Update output
      ui.slit1Text->setText(QString::number(s1,'f',3));
      ui.slit2Text->setText(QString::number(s2,'f',3));
    }
  }
}
