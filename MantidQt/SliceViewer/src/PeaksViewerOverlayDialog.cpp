#include <QDialogButtonBox>
#include <QUrl>
#include "MantidQtSliceViewer/PeaksViewerOverlayDialog.h"
#include "MantidQtAPI/MantidDesktopServices.h"
#include "ui_PeaksViewerOverlayDialog.h"

namespace MantidQt {
using API::MantidDesktopServices;

namespace SliceViewer {
/**
 * Calculate the conversion factor from slider position to fractional occupancy.
 * @return fractional occupancy factor.
 */
double toFractionalOccupancy() {
  const double yMax = 0.05; // 5% occupancy
  const double yMin = 0.00; // 0% occupancy
  const double xMax = 100;  // slider max
  const double xMin = 0;    // slider min
  const double m = (yMax - yMin) / (xMax - xMin);
  return m;
}

/**
 * Calculate the position of the slider.
 * @param fraction: fractional occupancy
 * @return position
 */
int calculatePosition(const double fraction) {
  return static_cast<int>((1 / toFractionalOccupancy()) * fraction);
}

/**
 * Calculate the fractional occupancy from the slider position.
 * @param sliderPosition : position of the slider
 * @return fractional occupancy
 */
double calculateFraction(const double sliderPosition) {
  return toFractionalOccupancy() * sliderPosition;
}

/**
 * Take a double and produce a formatted string with single decimal precision.
 * @param fraction : Fractional occupancy.
 * @return Formatted string.
 */
QString formattedPercentageValue(double fraction) {
  QString number;
  number.sprintf("%1.1f", fraction * 100);
  return number + QString(" %");
}

/**
 * Constructor
 * @param peaksPresenter : Peaks presenter to use
 * @param parent : Parent widget
 */
PeaksViewerOverlayDialog::PeaksViewerOverlayDialog(
    PeaksPresenter_sptr peaksPresenter, QWidget *parent)
    : QDialog(parent), ui(new Ui::PeaksViewerOverlayDialog),
      m_peaksPresenter(peaksPresenter) {
  ui->setupUi(this);

  m_originalOnProjectionFraction = peaksPresenter->getPeakSizeOnProjection();
  m_originalIntoProjectionFraction =
      peaksPresenter->getPeakSizeIntoProjection();

  ui->sliderOnProjection->setSliderPosition(
      calculatePosition(m_originalOnProjectionFraction));
  ui->sliderIntoProjection->setSliderPosition(
      calculatePosition(m_originalIntoProjectionFraction));

  ui->lblPercentageOnProjection->setText(
      formattedPercentageValue(m_originalOnProjectionFraction));
  ui->lblPercentageIntoProjection->setText(
      formattedPercentageValue(m_originalIntoProjectionFraction));

  connect(ui->sliderIntoProjection, SIGNAL(sliderMoved(int)), this,
          SLOT(onSliderIntoProjectionMoved(int)));
  connect(ui->sliderOnProjection, SIGNAL(sliderMoved(int)), this,
          SLOT(onSliderOnProjectionMoved(int)));
  connect(ui->btnReset, SIGNAL(clicked()), this, SLOT(onReset()));
  connect(ui->btnGroupControls, SIGNAL(clicked(QAbstractButton *)), this,
          SLOT(onCompleteClicked(QAbstractButton *)));
  connect(ui->btnHelp, SIGNAL(clicked()), this, SLOT(onHelp()));
}

/// Destructor
PeaksViewerOverlayDialog::~PeaksViewerOverlayDialog() { delete ui; }

/**
 * Handler for moving the slider associated with ON the current projection.
 * @param value: New slider position
 */
void PeaksViewerOverlayDialog::onSliderOnProjectionMoved(int value) {
  auto newFractionOccupancy = calculateFraction(value);
  m_peaksPresenter->setPeakSizeOnProjection(newFractionOccupancy);
  ui->lblPercentageOnProjection->setText(
      formattedPercentageValue(newFractionOccupancy));
}

/**
 * Handler for moving the slider associated with INTO the current projection.
 * @param value: New slider position
 */
void PeaksViewerOverlayDialog::onSliderIntoProjectionMoved(int value) {
  auto newFractionOccupancy = calculateFraction(value);
  m_peaksPresenter->setPeakSizeIntoProjection(newFractionOccupancy);
  ui->lblPercentageIntoProjection->setText(
      formattedPercentageValue(newFractionOccupancy));
}

/**
 * Handler for the reset event.
 */
void PeaksViewerOverlayDialog::onReset() {
  m_peaksPresenter->setPeakSizeOnProjection(m_originalOnProjectionFraction);
  m_peaksPresenter->setPeakSizeIntoProjection(m_originalIntoProjectionFraction);
  ui->sliderOnProjection->setSliderPosition(
      calculatePosition(m_originalOnProjectionFraction));
  ui->sliderIntoProjection->setSliderPosition(
      calculatePosition(m_originalIntoProjectionFraction));
  ui->lblPercentageOnProjection->setText(
      formattedPercentageValue(m_originalOnProjectionFraction));
  ui->lblPercentageIntoProjection->setText(
      formattedPercentageValue(m_originalIntoProjectionFraction));
}

/**
 * Handler for the on-complete click event of the button group.
 * @param button : Button clicked.
 */
void PeaksViewerOverlayDialog::onCompleteClicked(QAbstractButton *button) {
  QDialogButtonBox::ButtonRole role = ui->btnGroupControls->buttonRole(button);
  if (role == QDialogButtonBox::RejectRole) {
    onReset();
  }
}

/**
 * Handler for the on-close event.
 * @param event : Close event
 */
void PeaksViewerOverlayDialog::closeEvent(QCloseEvent *event) {
  onReset();
  QDialog::closeEvent(event);
}

/**
 * Handler for the reject event.
 */
void PeaksViewerOverlayDialog::reject() {
  onReset();
  QDialog::reject();
}

/**
 * Handler for the help-clicked event.
 */
void PeaksViewerOverlayDialog::onHelp() {
  QString helpPage = "PeaksViewer#Preference_Options";
  MantidDesktopServices::openUrl(
      QUrl(QString("http://www.mantidproject.org/") + helpPage));
}
}
}
