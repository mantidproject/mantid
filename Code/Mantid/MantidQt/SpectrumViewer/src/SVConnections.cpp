#include <iostream>
#include <qwt_plot_canvas.h>

#include <QDesktopServices>

#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/MantidColorMap.h"

#include "MantidQtSpectrumViewer/SVConnections.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct the object that links the GUI components to the other specifed
 * higher level objects.  This class just keeps pointers to the parameters.
 * The objects passed in must be constructed elsewhere and must be deleted
 * elsewhere, when the SpectrumViewer is closed.
 *
 * @param ui              The object containing the gui components for
 *                        the SpectrumView viewer.
 * @param spectrumView    The main window.
 * @param spectrumDisplay The SpectrumDisplay object that will display the
 *                        image
 * @param hGraphDisplay   The GraphDisplay object that will display
 *                        horizontal cuts through the image
 * @param vGraphDisplay   The GraphDisplay object that will display
 *                        vertical cuts through the image
 *
 */
SVConnections::SVConnections( Ui_SpectrumViewer* ui,
                              SpectrumView*      spectrumView,
                              SpectrumDisplay*   spectrumDisplay,
                              GraphDisplay*      hGraphDisplay,
                              GraphDisplay*      vGraphDisplay ) :
  m_svUI(ui),
  m_svMainWindow(spectrumView),
  m_spectrumDisplay(spectrumDisplay),
  m_hGraphDisplay(hGraphDisplay),
  m_vGraphDisplay(vGraphDisplay),
  m_pickerX(-1), m_pickerY(-1)
{
  // First disable a few un-implemented controls
  m_svUI->menuGraph_Selected->setDisabled(true);
  m_svUI->actionClear_Selections->setDisabled(true);
  m_svUI->actionOverlaid->setDisabled(true);
  m_svUI->actionOffset_Vertically->setDisabled(true);
  m_svUI->actionOffset_Diagonally->setDisabled(true);
  m_svUI->actionGraph_Rebinned_Data->setDisabled(true);
  m_svUI->menuHelp->setDisabled(false);

  QObject::connect( m_svUI->actionClose, SIGNAL(triggered()),
                    this, SLOT(closeViewer()) );

  // Now set up the GUI components
  QList<int> image_sizes;
  image_sizes.append( 500 );
  image_sizes.append( 250 );
  m_svUI->imageSplitter->setSizes( image_sizes );
  QList<int> vgraph_sizes;
  vgraph_sizes.append( 500 );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( 220 );
  m_svUI->vgraphSplitter->setSizes( vgraph_sizes );

  QList<int> horiz_sizes;
  horiz_sizes.append( 250 );
  horiz_sizes.append( 750 );
  horiz_sizes.append( 150 );
  m_svUI->left_right_splitter->setSizes( horiz_sizes );

  m_svUI->imageHorizontalScrollBar->setFocusPolicy( Qt::StrongFocus );
  m_svUI->imageHorizontalScrollBar->setMouseTracking(true);
  m_svUI->imageHorizontalScrollBar->setMinimum(20);
  m_svUI->imageHorizontalScrollBar->setMaximum(2000);
  m_svUI->imageHorizontalScrollBar->setPageStep(30);
  m_svUI->imageHorizontalScrollBar->setSingleStep(30/2);

  m_svUI->imageVerticalScrollBar->setFocusPolicy( Qt::StrongFocus );
  m_svUI->imageVerticalScrollBar->setMouseTracking(true);
  m_svUI->imageVerticalScrollBar->setMinimum(0);
  m_svUI->imageVerticalScrollBar->setMaximum(10000000);
  m_svUI->imageVerticalScrollBar->setPageStep(500);
  m_svUI->imageVerticalScrollBar->setSingleStep(500/2);

  // for forwarding scroll wheel events
  m_svUI->spectrumPlot->canvas()->installEventFilter(this);

  m_svUI->action_Hscroll->setCheckable(true);
  m_svUI->action_Hscroll->setChecked(false);
  m_svUI->imageHorizontalScrollBar->hide();
  m_svUI->imageHorizontalScrollBar->setEnabled(false);

  m_svUI->action_Vscroll->setCheckable(true);
  m_svUI->action_Vscroll->setChecked(true);
  m_svUI->imageVerticalScrollBar->show();
  m_svUI->imageVerticalScrollBar->setEnabled(true);

  m_svUI->intensity_slider->setTickInterval(10);
  m_svUI->intensity_slider->setTickPosition(QSlider::TicksBelow);
  m_svUI->intensity_slider->setSliderPosition(30);

  m_svUI->graph_max_slider->setTickInterval(10);
  m_svUI->graph_max_slider->setTickPosition(QSlider::TicksBelow);
  m_svUI->graph_max_slider->setSliderPosition(100);

  m_imagePicker = new TrackingPicker( m_svUI->spectrumPlot->canvas() );
  m_imagePicker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  m_imagePicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_imagePicker->setRubberBandPen(QColor(Qt::gray));

/* // point selections & connection works on mouse release
*/
  m_imagePicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_imagePicker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::DragSelection  );
/*
  QObject::connect( m_imagePicker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*  // point selection works on mouse click, NO CROSSHAIRS...

  m_imagePicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_imagePicker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::ClickSelection  );
  QObject::connect( m_imagePicker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*  // rect selection calls SLOT on mouse release

  m_imagePicker->setMousePattern(QwtPicker::MouseSelect1, Qt::MidButton);
  m_imagePicker->setRubberBand(QwtPicker::RectRubberBand);
  m_imagePicker->setSelectionFlags(QwtPicker::RectSelection |
                                  QwtPicker::DragSelection  );
  QObject::connect( m_imagePicker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*
  m_imagePicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_imagePicker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::ClickSelection  );
*/
  QObject::connect( m_imagePicker, SIGNAL(mouseMoved(const QPoint &)),
                    this, SLOT(imagePickerMoved(const QPoint &)) );

  QObject::connect(m_svUI->imageSplitter, SIGNAL(splitterMoved(int, int)),
                   this, SLOT(imageSplitterMoved()) );

  QObject::connect(m_svUI->vgraphSplitter, SIGNAL(splitterMoved(int, int)),
                   this, SLOT(vgraphSplitterMoved()) );

  QObject::connect(m_svUI->x_min_input, SIGNAL( returnPressed() ),
                   this, SLOT(imageHorizontalRangeChanged()) );

  QObject::connect(m_svUI->x_max_input, SIGNAL( returnPressed() ),
                   this, SLOT(imageHorizontalRangeChanged()) );

  QObject::connect(m_svUI->step_input, SIGNAL( returnPressed() ),
                   this, SLOT(imageHorizontalRangeChanged()) );

  QObject::connect(m_svUI->imageVerticalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(scrollBarMoved()) );

  QObject::connect(m_svUI->imageHorizontalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(scrollBarMoved()) );

  QObject::connect(m_svUI->action_Hscroll, SIGNAL(changed()),
                   this, SLOT(toggleHScroll()) );

  QObject::connect(m_svUI->action_Vscroll, SIGNAL(changed()),
                   this, SLOT(toggleVScroll()) );

  QObject::connect(m_svUI->intensity_slider, SIGNAL(valueChanged(int)),
                   this, SLOT(intensitySliderMoved()) );

  QObject::connect(m_svUI->graph_max_slider, SIGNAL(valueChanged(int)),
                   this, SLOT(graphRangeChanged()) );

  // Color scale selections
  m_svUI->actionHeat->setCheckable(true);
  m_svUI->actionHeat->setChecked(true);
  m_svUI->actionGray->setCheckable(true);
  m_svUI->actionNegative_Gray->setCheckable(true);
  m_svUI->actionGreen_Yellow->setCheckable(true);
  m_svUI->actionRainbow->setCheckable(true);
  m_svUI->actionOptimal->setCheckable(true);
  m_svUI->actionMulti->setCheckable(true);
  m_svUI->actionSpectrum->setCheckable(true);
  m_svUI->actionLoadColormap->setCheckable(true);
                                                    // set up initial color
                                                    // scale display
  m_svUI->color_scale->setScaledContents(true);
  m_svUI->color_scale->setMinimumHeight(15);
  m_svUI->color_scale->setMinimumWidth(15);
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  showColorScale( positive_color_table, negative_color_table );

  m_colorGroup = new QActionGroup(this);
  m_colorGroup->addAction(m_svUI->actionHeat);
  m_colorGroup->addAction(m_svUI->actionGray);
  m_colorGroup->addAction(m_svUI->actionNegative_Gray);
  m_colorGroup->addAction(m_svUI->actionGreen_Yellow);
  m_colorGroup->addAction(m_svUI->actionRainbow);
  m_colorGroup->addAction(m_svUI->actionOptimal);
  m_colorGroup->addAction(m_svUI->actionMulti);
  m_colorGroup->addAction(m_svUI->actionSpectrum);
  m_colorGroup->addAction(m_svUI->actionLoadColormap);

  QObject::connect(m_svUI->actionHeat, SIGNAL(triggered()),
                   this, SLOT(heatColorScale()) );

  QObject::connect(m_svUI->actionGray, SIGNAL(triggered()),
                   this, SLOT(grayColorScale()) );

  QObject::connect(m_svUI->actionNegative_Gray, SIGNAL(triggered()),
                   this, SLOT(negativeGrayColorScale()) );

  QObject::connect(m_svUI->actionGreen_Yellow, SIGNAL(triggered()),
                   this, SLOT(greenYellowColorScale()) );

  QObject::connect(m_svUI->actionRainbow, SIGNAL(triggered()),
                   this, SLOT(rainbowColorScale()) );

  QObject::connect(m_svUI->actionOptimal, SIGNAL(triggered()),
                   this, SLOT(optimalColorScale()) );

  QObject::connect(m_svUI->actionMulti, SIGNAL(triggered()),
                   this, SLOT(multiColorScale()) );

  QObject::connect(m_svUI->actionSpectrum, SIGNAL(triggered()),
                   this, SLOT(spectrumColorScale()) );

  QObject::connect(m_svUI->actionLoadColormap, SIGNAL(triggered()),
                   this, SLOT(loadColorMap()) );


  m_hGraphPicker = new TrackingPicker( m_svUI->h_graphPlot->canvas() );
  m_hGraphPicker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  m_hGraphPicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_hGraphPicker->setRubberBandPen(QColor(Qt::gray));
  m_hGraphPicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_hGraphPicker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::DragSelection  );
  QObject::connect( m_hGraphPicker, SIGNAL(mouseMoved(const QPoint &)),
                    this, SLOT(hGraphPickerMoved(const QPoint &)) );

  // NOTE: This initialization could be a (static?) method in TrackingPicker
  m_vGraphPicker = new TrackingPicker( m_svUI->v_graphPlot->canvas() );
  m_vGraphPicker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  m_vGraphPicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_vGraphPicker->setRubberBandPen(QColor(Qt::gray));
  m_vGraphPicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_vGraphPicker->setSelectionFlags(QwtPicker::PointSelection |
                                    QwtPicker::DragSelection  );
  QObject::connect( m_vGraphPicker, SIGNAL(mouseMoved(const QPoint &)),
                    this, SLOT(vGraphPickerMoved(const QPoint &)) );

  QObject::connect( m_svUI->actionOnline_Help_Page, SIGNAL(triggered()),
                    this, SLOT(openOnlineHelp()) );

}


SVConnections::~SVConnections()
{
}


/**
 * Handle events.
 *
 * @param object Object that the event came from.
 * @param event The event being filtered.
 * @return true if the event was consumed.
 */
bool SVConnections::eventFilter(QObject *object, QEvent *event)
{
  UNUSED_ARG(object);

  if (event->type() == QEvent::Wheel)
  {
    QWheelEvent *wheelEvent = dynamic_cast<QWheelEvent *>(event);
    if (wheelEvent)
    {
      if (wheelEvent->orientation() == Qt::Orientation::Vertical)
      {
        return m_svUI->imageVerticalScrollBar->event(wheelEvent);
      }
      else if (wheelEvent->orientation() == Qt::Orientation::Horizontal)
      {
        return m_svUI->imageHorizontalScrollBar->event(wheelEvent);
      }
    }
  }
  else if (event->type() == QEvent::KeyPress)
  {
    // don't bother if the values aren't set
    if (m_pickerX < 0) return false;
    if (m_pickerY < 0) return false;

    // Convert Y position to values so that a change of 1 corresponds to a change in spec. no by 1
    int newX = m_pickerX;
    double lastY = m_spectrumDisplay->getPointedAtY();

    QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
    if(!keyEvent) return false;
    int key = keyEvent->key();
    switch (key)
    {
    case Qt::Key_Up:
      lastY += 1.0;
      break;
    case Qt::Key_Down:
      lastY -= 1.0;
      break;
    case Qt::Key_Left:
      newX--;
      break;
    case Qt::Key_Right:
      newX++;
      break;
    default:
      // this is not the event we were looking for
      return false;
    }

    // Convert Y position back to unsigned pixel position
    QPoint newPoint = m_spectrumDisplay->getPlotTransform(qMakePair(0.0, lastY));
    int newY = newPoint.y();

    // Ignore the event if the position is outside of the plot area
    if (newX < 0) return false;
    if (newY < 0) return false;
    const QSize canvasSize = m_svUI->spectrumPlot->canvas()->size();
    if (newX > canvasSize.width()) return false;
    if (newY > canvasSize.height()) return false;

    // make the changes real
    m_pickerX = newX;
    m_pickerY = newY;

    // determine where the canvas is in global coords
    QPoint canvasPos = m_svUI->spectrumPlot->canvas()->mapToGlobal(QPoint(0,0));
    // move the cursor to the correct position
    m_svUI->spectrumPlot->canvas()->cursor().setPos(QPoint(canvasPos.x()+m_pickerX, canvasPos.y()+m_pickerY));

    QPair<double, double> transPoints = m_spectrumDisplay->getPlotInvTransform(QPoint(newX, newY));

    m_spectrumDisplay->setHGraph( lastY );
    m_spectrumDisplay->setVGraph( transPoints.first );

    m_spectrumDisplay->showInfoList( transPoints.first, lastY );

    // consume the event
    return true;
  }

  // don't filter the event
  return false;
}


/**
 * Slot to handle closing the window.
 */
void SVConnections::closeViewer()
{
  m_svMainWindow->close();
}


/**
 * Toggles the horizontal scroll bar.
 */
void SVConnections::toggleHScroll()
{
  bool is_on = m_svUI->action_Hscroll->isChecked();
  m_svUI->imageHorizontalScrollBar->setVisible( is_on );
  m_svUI->imageHorizontalScrollBar->setEnabled( is_on );
  m_spectrumDisplay->updateImage();
  m_spectrumDisplay->handleResize();
}


/**
 * Toggles the vertical scroll bar.
 */
void SVConnections::toggleVScroll()
{
  bool is_on = m_svUI->action_Vscroll->isChecked();
  m_svUI->imageVerticalScrollBar->setVisible( is_on );
  m_svUI->imageVerticalScrollBar->setEnabled( is_on );
  m_spectrumDisplay->updateImage();
  m_spectrumDisplay->handleResize();
}


/**
 * Update X range when range selection changed.
 */
void SVConnections::imageHorizontalRangeChanged()
{
  m_spectrumDisplay->updateRange();
}


void SVConnections::graphRangeChanged()
{
  double value = (double)m_svUI->graph_max_slider->value();
  double min   = (double)m_svUI->graph_max_slider->minimum();
  double max   = (double)m_svUI->graph_max_slider->maximum();

  double range_scale = (value - min)/(max - min);
  if ( range_scale < 0.01 )
    range_scale = 0.01;

  m_hGraphDisplay->setRangeScale( range_scale );
  m_vGraphDisplay->setRangeScale( range_scale );
}


/**
 * Handles updating the image when a scroll bar is moved.
 */
void SVConnections::scrollBarMoved()
{
  m_spectrumDisplay->updateImage();
}


/**
 * Handle the image splitter being moved.
 *
 * This moves the vertical graph slitter to the same position
 * in ordetr to keep the graphs in alignment.
 */
void SVConnections::imageSplitterMoved()
{
  QList<int> sizes = m_svUI->imageSplitter->sizes();

  QList<int> vgraph_sizes;
  vgraph_sizes.append( sizes[0] );
  vgraph_sizes.append( sizes[1] );

  m_svUI->vgraphSplitter->setSizes( vgraph_sizes );

  m_spectrumDisplay->updateImage();
  m_spectrumDisplay->handleResize();
}


/**
 * Handle the vertical graph splitter being moved.
 *
 * This moves the image slitter to the same position
 * in ordetr to keep the graphs in alignment.
 */
void SVConnections::vgraphSplitterMoved()
{
  QList<int> sizes = m_svUI->vgraphSplitter->sizes();

  QList<int> vgraph_sizes;
  vgraph_sizes.append( sizes[0] );
  vgraph_sizes.append( sizes[1] );

  m_svUI->imageSplitter->setSizes( vgraph_sizes );

  m_spectrumDisplay->updateImage();
  m_spectrumDisplay->handleResize();
}


/**
 * Update the pointed at position for the m_imagePicker.
 *
 * @param point The position moved to.
 */
void SVConnections::imagePickerMoved(const QPoint & point)
{
  m_pickerX = point.x();
  m_pickerY = point.y();
  m_spectrumDisplay->setPointedAtPoint( point );
}


/**
 * Update the pointed at position for the m_hGraphDisplay.
 *
 * @param point The position moved to.
 */
void SVConnections::hGraphPickerMoved(const QPoint & point)
{
  m_hGraphDisplay->setPointedAtPoint(point);
}


/**
 * Update the pointed at position for the m_vGraphDisplay.
 *
 * @param point The position moved to.
 */
void SVConnections::vGraphPickerMoved(const QPoint & point)
{
  m_vGraphDisplay->setPointedAtPoint(point);
}


/**
 * Slot to handle the intensity slider being moved.
 */
void SVConnections::intensitySliderMoved()
{
  double value = (double)m_svUI->intensity_slider->value();
  double min   = (double)m_svUI->intensity_slider->minimum();
  double max   = (double)m_svUI->intensity_slider->maximum();

  double scaled_value = 100.0*(value - min)/(max - min);
  m_spectrumDisplay->setIntensity( scaled_value );
}


/**
 * Set the heat color scale.
 */
void SVConnections::heatColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the gray color scale.
 */
void SVConnections::grayColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the inverse gray color scale.
 */
void SVConnections::negativeGrayColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::NEGATIVE_GRAY,256, positive_color_table);

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the green and yellow color scale.
 */
void SVConnections::greenYellowColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::GREEN_YELLOW, 256, positive_color_table);

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the rainbow color scale.
 */
void SVConnections::rainbowColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::RAINBOW, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the optimal color scale.
 */
void SVConnections::optimalColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::OPTIMAL, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the multi color scale.
 */
void SVConnections::multiColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::MULTI, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Set the spectrum color scale.
 */
void SVConnections::spectrumColorScale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::SPECTRUM, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 * Slot to handle loading a color map from file.
 */
void SVConnections::loadColorMap()
{
  QString file_name = MantidColorMap::loadMapDialog( "", m_svMainWindow );

  MantidColorMap* mantid_color_map = new MantidColorMap( file_name, GraphOptions::Linear );

  QwtDoubleInterval interval( 0.0, 255.0 );
  QVector<QRgb> mantid_color_table;
  mantid_color_table = mantid_color_map->colorTable( interval );
  std::vector<QRgb> positive_color_table;
  for ( int i = 1; i < mantid_color_table.size(); i++ )     // NO NaN Color
  {
    positive_color_table.push_back( mantid_color_table[i] );
  }

  int n_colors = (int)positive_color_table.size();

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, n_colors, negative_color_table );

  m_spectrumDisplay->setColorScales( positive_color_table, negative_color_table );
  showColorScale( positive_color_table, negative_color_table );
}


/**
 *  Set the pix map that shows the color scale from the specified positive
 *  and negative color tables.
 *
 *  @param positiveColorTable  The new color table used to map positive data
 *                             values to an RGB color.
 *  @param negativeColorTable  The new color table used to map negative data
 *                             values to an RGB color.  This must have the
 *                             same number of entries as the positive
 *                             color table.
 */
void SVConnections::showColorScale( std::vector<QRgb> & positiveColorTable,
                                    std::vector<QRgb> & negativeColorTable )
{
  size_t totalColors = positiveColorTable.size() + negativeColorTable.size();

  QImage image((int)totalColors, 1, QImage::Format_RGB32);
  int index = 0;

  size_t numColors = negativeColorTable.size();
  for(size_t i = 0; i < numColors; i++)
  {
    unsigned int pixel = static_cast<unsigned int>(negativeColorTable[numColors - 1 - i]);
    image.setPixel(index, 0, pixel);
    index++;
  }

  numColors = positiveColorTable.size();
  for(size_t i = 0; i < numColors; i++)
  {
    unsigned int pixel = static_cast<unsigned int>(positiveColorTable[i]);
    image.setPixel(index, 0, pixel);
    index++;
  }

  QPixmap pixmap = QPixmap::fromImage(image);
  m_svUI->color_scale->setPixmap( pixmap );
}


/**
 * Slot to open the online help webapge for the interface.
 */
void SVConnections::openOnlineHelp()
{
  MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("SpectrumViewer"));
}

} // namespace SpectrumView
} // namespace MantidQt
