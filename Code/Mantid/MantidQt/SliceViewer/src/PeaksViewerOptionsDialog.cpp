#include <QDialogButtonBox>
#include "MantidQtSliceViewer/PeaksViewerOptionsDialog.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "ui_PeaksViewerOptionsDialog.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
     * Calculate the conversion factor from slider position to fractional occupancy.
     * @return fractional occupancy factor.
     */
    double toFractionalOccupancy()
    {
      const double yMax = 0.05; // 5% occupancy
      const double yMin = 0.00; // 0% occupancy
      const double xMax = 100; // slider max
      const double xMin = 0; // slider min
      const double m = (yMax - yMin) / (xMax - xMin);
      return m;
    }

    double calculatePosition(const double fraction)
    {
      return (1/toFractionalOccupancy()) * fraction;
    }

    double calculateFraction(const double sliderPosition)
    {
      return toFractionalOccupancy() * sliderPosition;
    }

    QString formattedPercentageValue(double fraction)
    {
      QString number;
      number.sprintf("%1.1f", fraction*100);
      return number + QString(" %");
    }


    PeaksViewerOptionsDialog::PeaksViewerOptionsDialog(PeaksPresenter_sptr peaksPresenter, QWidget *parent) :
        QDialog(parent), ui(new Ui::PeaksViewerOptionsDialog), m_peaksPresenter(peaksPresenter)
    {
    ui->setupUi(this);

    m_originalOnProjectionFraction = peaksPresenter->getPeakSizeOnProjection();
    m_originalIntoProjectionFraction = peaksPresenter->getPeakSizeIntoProjection();

    ui->sliderOnProjection->setSliderPosition( calculatePosition( m_originalOnProjectionFraction ) );
    ui->sliderIntoProjection->setSliderPosition( calculatePosition( m_originalIntoProjectionFraction ) );

    ui->lblPercentageOnProjection->setText( formattedPercentageValue( m_originalOnProjectionFraction ) );
    ui->lblPercentageIntoProjection->setText( formattedPercentageValue( m_originalIntoProjectionFraction ) );

    connect(ui->sliderIntoProjection, SIGNAL(sliderMoved(int)), this, SLOT(onSliderIntoProjectionMoved(int)));
    connect(ui->sliderOnProjection, SIGNAL(sliderMoved(int)), this, SLOT(onSliderOnProjectionMoved(int)));
    connect(ui->btnReset, SIGNAL(clicked()), this, SLOT(onReset()));
    connect(ui->btnGroupControls, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onCompleteClicked(QAbstractButton*)));
  }

  PeaksViewerOptionsDialog::~PeaksViewerOptionsDialog()
  {
    delete ui;
  }

  void PeaksViewerOptionsDialog::onSliderOnProjectionMoved(int value)
  {
    auto newFractionOccupancy = calculateFraction(value);
    m_peaksPresenter->setPeakSizeOnProjection(newFractionOccupancy);
    ui->lblPercentageOnProjection->setText( formattedPercentageValue(newFractionOccupancy) );
  }

  void PeaksViewerOptionsDialog::onSliderIntoProjectionMoved(int value)
  {
    auto newFractionOccupancy = calculateFraction(value);
    m_peaksPresenter->setPeakSizeIntoProjection(newFractionOccupancy);
    ui->lblPercentageIntoProjection->setText( formattedPercentageValue(newFractionOccupancy) );
  }

  void PeaksViewerOptionsDialog::onReset()
  {
    m_peaksPresenter->setPeakSizeOnProjection( m_originalOnProjectionFraction );
    m_peaksPresenter->setPeakSizeIntoProjection( m_originalIntoProjectionFraction );
    ui->sliderOnProjection->setSliderPosition( calculatePosition( m_originalOnProjectionFraction ) );
    ui->sliderIntoProjection->setSliderPosition( calculatePosition( m_originalIntoProjectionFraction ) );
    ui->lblPercentageOnProjection->setText( formattedPercentageValue(m_originalOnProjectionFraction) );
    ui->lblPercentageIntoProjection->setText( formattedPercentageValue(m_originalIntoProjectionFraction) );
  }

  void PeaksViewerOptionsDialog::onCompleteClicked(QAbstractButton* button)
  {
    QDialogButtonBox::ButtonRole role = ui->btnGroupControls->buttonRole(button);
    if(role == QDialogButtonBox::RejectRole)
    {
      onReset();
    }
  }

  void PeaksViewerOptionsDialog::closeEvent(QCloseEvent *event)
  {
    onReset();
    QDialog::closeEvent(event);
  }

  void PeaksViewerOptionsDialog::reject()
  {
    onReset();
    QDialog::reject();
  }

}
}



