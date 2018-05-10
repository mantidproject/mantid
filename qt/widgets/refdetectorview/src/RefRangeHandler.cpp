#include <QLineEdit>

#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/RefDetectorView/RefRangeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/QtUtils.h"
#include "MantidQtWidgets/SpectrumViewer/SVUtils.h"

namespace {
Mantid::Kernel::Logger g_log("SpectrumView");
}

namespace MantidQt {
namespace RefDetectorViewer {

using namespace SpectrumView;

/**
 *  Construct a RefRangeHandler object to manage min, max and step controls
 *  in the specified UI
 */
RefRangeHandler::RefRangeHandler(Ui_RefImageViewer *ivUI)
    : m_ivUI(ivUI), m_totalMinX(0.0), m_totalMaxX(0.0), m_totalMinY(0.0),
      m_totalMaxY(0.0), m_totalNSteps(0) {}

/**
 * Configure the min, max and step controls for the specified data source.
 *
 * @param dataSource  SpectrumDataSource that provides the data to be drawn
 */
void RefRangeHandler::configureRangeControls(
    SpectrumDataSource_sptr dataSource) {
  // X axis
  m_totalMinX = dataSource->getXMin();
  m_totalMaxX = dataSource->getXMax();
  m_totalNSteps = dataSource->getNCols();

  double defaultStepX = (m_totalMaxX - m_totalMinX) / (double)m_totalNSteps;
  if (m_totalNSteps > 2000)
    defaultStepX = (m_totalMaxX - m_totalMinX) / 2000.0;

  setRange(m_totalMinX, m_totalMaxX, defaultStepX, 'x');

  // Y axis
  m_totalMinY = dataSource->getYMin();
  m_totalMaxY = dataSource->getYMax();
  m_totalNSteps = dataSource->getNCols();

  double defaultStepY = (m_totalMaxY - m_totalMinY) / (double)m_totalNSteps;
  if (m_totalNSteps > 2000)
    defaultStepY = (m_totalMaxY - m_totalMinY) / 2000.0;

  setRange(m_totalMinY, m_totalMaxY, defaultStepY, 'y');
}

/**
 * Get the interval of values and the step size to use for rebinning the
 * spectra.  The range values are validated and adjusted if needed.  The
 * range values that are returned by this method will also be displayed in
 * the controls.
 *
 * @param min     On input, this should be the default value that the
 *                min should be set to, if getting the range fails.
 *                On output this is will be set to the x value at the
 *                left edge of the first bin to display, if getting the
 *                range succeeds.
 * @param max     On input, this should be the default value that the
 *                max should be set to if getting the range fails.
 *                On output, if getting the range succeeds, this will
 *                be set an x value at the right edge of the last bin
 *                to display.  This will be adjusted so that it is larger
 *                than min by an integer number of steps.
 * @param step    On input this should be the default number of steps
 *                to use if getting the range information fails.
 *                On output, this is size of the step to use between
 *                min and max.  If it is less than zero, a log scale
 *                is requested.
 */
void RefRangeHandler::getRange(double &min, double &max, double &step) {
  double originalMin = min;
  double originalMax = max;
  double originalStep = step;

  QLineEdit *min_control = m_ivUI->x_min_input;
  QLineEdit *max_control = m_ivUI->x_max_input;

  bool minIsNumber = false;
  bool maxIsNumber = false;

  min = min_control->text().toDouble(&minIsNumber);
  max = max_control->text().toDouble(&maxIsNumber);

  if (!minIsNumber) {
    g_log.information("X Min is not a NUMBER! Value reset.");
    min = originalMin;
  }

  if (!maxIsNumber) {
    g_log.information("X Max is not a NUMBER! Value reset.");
    max = originalMax;
  }

  // Just require step to be non-zero, no other bounds. If zero, take a default
  // step size
  if (step == 0) {
    g_log.information("Step = 0, resetting to default step");
    step = originalStep;
  }

  if (step > 0) {
    if (!SVUtils::FindValidInterval(min, max)) {
      g_log.information(
          "In GetRange: [Min,Max] interval invalid, values adjusted");
      min = originalMin;
      max = originalMax;
      step = originalStep;
    }
  } else {
    if (!SVUtils::FindValidLogInterval(min, max)) {
      g_log.information(
          "In GetRange: [Min,Max] log interval invalid, values adjusted");
      min = originalMin;
      max = originalMax;
      step = originalStep;
    }
  }

  setRange(min, max, step, 'x');
}

/**
 * Adjust the values to be consistent with the available data and
 * diplay them in the controls.
 *
 * @param min     This is the x value at the left edge of the first bin.
 * @param max     This is an x value at the right edge of the last bin.
 * @param step    This is size of the step to use between min and max.
 *                If it is less than zero, a log scale is requested.
 * @param type    x or y
 */
void RefRangeHandler::setRange(double min, double max, double step, char type) {
  if (type == 'x') {

    if (!SVUtils::FindValidInterval(min, max))
      g_log.information(
          "In setRange: [XMin,XMax] interval invalid, values adjusted");

    if (min < m_totalMinX || min > m_totalMaxX) {
      g_log.information("X Min out of range, resetting to range min.");
      min = m_totalMinX;
    }

    if (max < m_totalMinX || max > m_totalMaxX) {
      g_log.information("X Max out of range, resetting to range max.");
      max = m_totalMaxX;
    }

    if (step == 0) {
      g_log.information("Step = 0, resetting to default step");
      step = (max - min) / 2000.0;
    }

    QtUtils::SetText(8, 2, min, m_ivUI->x_min_input);
    QtUtils::SetText(8, 2, max, m_ivUI->x_max_input);
    //  QtUtils::SetText( 8, 4, step, m_ivUI->step_input );
  }

  if (type == 'y') {

    if (!SVUtils::FindValidInterval(min, max))
      g_log.information(
          "In setRange: [YMin,YMax] interval invalid, values adjusted");

    if (min < m_totalMinY || min > m_totalMaxY) {
      g_log.information("Y Min out of range, resetting to range min.");
      min = m_totalMinY;
    }

    if (max < m_totalMinY || max > m_totalMaxY) {
      g_log.information("Y Max out of range, resetting to range max.");
      max = m_totalMaxY;
    }

    if (step == 0) {
      g_log.information("Step = 0, resetting to default step");
      step = (max - min) / 2000.0;
    }

    QtUtils::SetText(8, 2, min, m_ivUI->y_min_input);
    QtUtils::SetText(8, 2, max, m_ivUI->y_max_input);
    //  QtUtils::SetText( 8, 4, step, m_ivUI->step_input );
  }
}

} // namespace RefDetectorViewer
} // namespace MantidQt
