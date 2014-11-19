#include <iostream>
#include <QLineEdit>
#include <qwt_plot_canvas.h>

#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
using namespace SpectrumView;

/**
 * Construct the object that links the GUI components to the other specifed
 * higher level objects.  This class just keeps pointers to the parameters.
 * The objects passed in must be constructed elsewhere and must be deleted
 * elsewhere, when the SpectrumViewer is closed.
 *
 * @param ui             The object containing the gui components for
 *                       the ImageView viewer.
 * @param ivMainWindow   The main window.
 * @param imageDisplay   The SpectrumDisplay object that will dispaly the image
 * @param hGraphDisplay  The GraphDisplay object that will display
 *                       horizontal cuts through the image
 * @param vGraphDisplay  The GraphDisplay object that will display
 *                       vertical cuts through the image
 */
RefIVConnections::RefIVConnections( Ui_RefImageViewer*  ui,
                                    RefImageView*       ivMainWindow,
                                    RefImageDisplay*    imageDisplay,
                                    GraphDisplay*       hGraphDisplay,
                                    GraphDisplay*       vGraphDisplay ) :
  m_ivUI(ui),
  m_ivMainWindow(ivMainWindow),
  m_imageDisplay(imageDisplay),
  m_hGraphDisplay(hGraphDisplay),
  m_vGraphDisplay(vGraphDisplay)
{
  // First disable a few un-implemented controls
  m_ivUI->menuGraph_Selected->setDisabled(true);
  m_ivUI->actionClear_Selections->setDisabled(true);
  m_ivUI->actionOverlaid->setDisabled(true);
  m_ivUI->actionOffset_Vertically->setDisabled(true);
  m_ivUI->actionOffset_Diagonally->setDisabled(true);
  m_ivUI->actionGraph_Rebinned_Data->setDisabled(true);
  m_ivUI->menuHelp->setDisabled(true);

  QObject::connect( m_ivUI->actionClose, SIGNAL(triggered()),
      this, SLOT(closeViewer()) );

  // Now set up the GUI components
  QList<int> image_sizes;
  image_sizes.append( 500 );
  image_sizes.append( 250 );
  m_ivUI->imageSplitter->setSizes( image_sizes );
  QList<int> vgraph_sizes;
  vgraph_sizes.append( 500 );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( 220 );
  m_ivUI->vgraphSplitter->setSizes( vgraph_sizes );

  QList<int> horiz_sizes;
  horiz_sizes.append( 250 );
  horiz_sizes.append( 750 );
  horiz_sizes.append( 150 );
  m_ivUI->left_right_splitter->setSizes( horiz_sizes );

  m_ivUI->imageHorizontalScrollBar->setFocusPolicy( Qt::StrongFocus );
  m_ivUI->imageHorizontalScrollBar->setMinimum(20);
  m_ivUI->imageHorizontalScrollBar->setMaximum(2000);
  m_ivUI->imageHorizontalScrollBar->setPageStep(30);
  m_ivUI->imageHorizontalScrollBar->setSingleStep(30/2);

  m_ivUI->imageVerticalScrollBar->setFocusPolicy( Qt::StrongFocus );
  m_ivUI->imageVerticalScrollBar->setMinimum(0);
  m_ivUI->imageVerticalScrollBar->setMaximum(10000000);
  m_ivUI->imageVerticalScrollBar->setPageStep(500);
  m_ivUI->imageVerticalScrollBar->setSingleStep(500/2);

  m_ivUI->action_Hscroll->setCheckable(true);
  m_ivUI->action_Hscroll->setChecked(false);
  m_ivUI->imageHorizontalScrollBar->hide();
  m_ivUI->imageHorizontalScrollBar->setEnabled(false);

  m_ivUI->action_Vscroll->setCheckable(true);
  m_ivUI->action_Vscroll->setChecked(true);
  m_ivUI->imageVerticalScrollBar->show();
  m_ivUI->imageVerticalScrollBar->setEnabled(true);

  m_ivUI->intensity_slider->setTickInterval(10);
  m_ivUI->intensity_slider->setTickPosition(QSlider::TicksBelow);
  m_ivUI->intensity_slider->setSliderPosition(30);

  //  m_ivUI->graph_max_slider->setTickInterval(10);
  //  m_ivUI->graph_max_slider->setTickPosition(QSlider::TicksBelow);
  //  m_ivUI->graph_max_slider->setSliderPosition(100);

  m_imagePicker2 = new TrackingPicker( m_ivUI->imagePlot->canvas() );
  m_imagePicker2->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  m_imagePicker2->setTrackerMode(QwtPicker::ActiveOnly);
  m_imagePicker2->setRubberBandPen(QColor(Qt::gray));

  m_imagePicker = new TrackingPicker( m_ivUI->imagePlot->canvas() );
  m_imagePicker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
  m_imagePicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_imagePicker->setRubberBandPen(QColor(Qt::blue));

  // Point selections & connection works on mouse release
  m_imagePicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_imagePicker->setSelectionFlags(QwtPicker::PointSelection |
      QwtPicker::DragSelection  );

  m_imagePicker2->setRubberBand(QwtPicker::CrossRubberBand);
  m_imagePicker2->setSelectionFlags(QwtPicker::PointSelection |
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

  QObject::connect( m_imagePicker2, SIGNAL(mouseMoved()),
      this, SLOT(imagePicker2Moved()) );

  QObject::connect( m_imagePicker, SIGNAL(mouseMoved()),
      this, SLOT(imagePickerMoved()) );

  /*
   * Connections on the peak, back and TOF input boxes
   */
  QObject::connect(m_ivUI->lineEdit_peakLeft, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );
  QObject::connect(m_ivUI->lineEdit_peakRight, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );
  QObject::connect(m_ivUI->lineEdit_backLeft, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );
  QObject::connect(m_ivUI->lineEdit_backRight, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );
  QObject::connect(m_ivUI->lineEdit_TOFmin, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );
  QObject::connect(m_ivUI->lineEdit_TOFmax, SIGNAL(returnPressed()),
      this, SLOT(editManualInput()) );

  QObject::connect(m_ivUI->imageSplitter, SIGNAL(splitterMoved(int,int)),
      this, SLOT(imageSplitterMoved()) );

  QObject::connect(m_ivUI->x_min_input, SIGNAL( returnPressed() ),
      this, SLOT(imageHorizontalRangeChanged()) );

  QObject::connect(m_ivUI->x_max_input, SIGNAL( returnPressed() ),
      this, SLOT(imageHorizontalRangeChanged()) );

  //  QObject::connect(m_ivUI->step_input, SIGNAL( returnPressed() ),
  //                   this, SLOT(image_horizontal_range_changed()) );

  QObject::connect(m_ivUI->imageVerticalScrollBar, SIGNAL(valueChanged(int)),
      this, SLOT(vScrollBarMoved()) );

  QObject::connect(m_ivUI->imageHorizontalScrollBar, SIGNAL(valueChanged(int)),
      this, SLOT(hScrollBarMoved()) );

  QObject::connect(m_ivUI->action_Hscroll, SIGNAL(changed()),
      this, SLOT(toggleHScroll()) );

  QObject::connect(m_ivUI->action_Vscroll, SIGNAL(changed()),
      this, SLOT(toggleVScroll()) );

  QObject::connect(m_ivUI->intensity_slider, SIGNAL(valueChanged(int)),
      this, SLOT(intensitySliderMoved()) );

  //  QObject::connect(m_ivUI->graph_max_slider, SIGNAL(valueChanged(int)),
  //                   this, SLOT(graphRangeChanged()) );

  // color scale selections
  m_ivUI->actionHeat->setCheckable(true);
  m_ivUI->actionHeat->setChecked(true);
  m_ivUI->actionGray->setCheckable(true);
  m_ivUI->actionNegative_Gray->setCheckable(true);
  m_ivUI->actionGreen_Yellow->setCheckable(true);
  m_ivUI->actionRainbow->setCheckable(true);
  m_ivUI->actionOptimal->setCheckable(true);
  m_ivUI->actionMulti->setCheckable(true);
  m_ivUI->actionSpectrum->setCheckable(true);
  // set up initial color
  // scale display
  m_ivUI->color_scale->setScaledContents(true);
  m_ivUI->color_scale->setMinimumHeight(15);
  m_ivUI->color_scale->setMinimumWidth(15);
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  showColorScale( positiveColorTable, negativeColorTable );


  m_colorGroup = new QActionGroup(this);
  m_colorGroup->addAction(m_ivUI->actionHeat);
  m_colorGroup->addAction(m_ivUI->actionGray);
  m_colorGroup->addAction(m_ivUI->actionNegative_Gray);
  m_colorGroup->addAction(m_ivUI->actionGreen_Yellow);
  m_colorGroup->addAction(m_ivUI->actionRainbow);
  m_colorGroup->addAction(m_ivUI->actionOptimal);
  m_colorGroup->addAction(m_ivUI->actionMulti);
  m_colorGroup->addAction(m_ivUI->actionSpectrum);

  QObject::connect(m_ivUI->actionHeat, SIGNAL(triggered()),
      this, SLOT(heatColorScale()) );

  QObject::connect(m_ivUI->actionGray, SIGNAL(triggered()),
      this, SLOT(grayColorScale()) );

  QObject::connect(m_ivUI->actionNegative_Gray, SIGNAL(triggered()),
      this, SLOT(negativeGrayColorScale()) );

  QObject::connect(m_ivUI->actionGreen_Yellow, SIGNAL(triggered()),
      this, SLOT(greenYellowColorScale()) );

  QObject::connect(m_ivUI->actionRainbow, SIGNAL(triggered()),
      this, SLOT(rainbowColorScale()) );

  QObject::connect(m_ivUI->actionOptimal, SIGNAL(triggered()),
      this, SLOT(optimalColorScale()) );

  QObject::connect(m_ivUI->actionMulti, SIGNAL(triggered()),
      this, SLOT(multiColorScale()) );

  QObject::connect(m_ivUI->actionSpectrum, SIGNAL(triggered()),
      this, SLOT(spectrumColorScale()) );

  m_hGraphPicker = new TrackingPicker( m_ivUI->h_graphPlot->canvas() );
  m_hGraphPicker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
  m_hGraphPicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_hGraphPicker->setRubberBandPen(QColor(Qt::gray));
  m_hGraphPicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_hGraphPicker->setSelectionFlags(QwtPicker::PointSelection |
      QwtPicker::DragSelection  );
  QObject::connect( m_hGraphPicker, SIGNAL(mouseMoved()),
      this, SLOT(hGraphPickerMoved()) );

  // NOTE: This initialization could be a (static?) method in TrackingPicker
  m_vGraphPicker = new TrackingPicker( m_ivUI->v_graphPlot->canvas() );
  m_vGraphPicker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
  m_vGraphPicker->setTrackerMode(QwtPicker::ActiveOnly);
  m_vGraphPicker->setRubberBandPen(QColor(Qt::gray));
  m_vGraphPicker->setRubberBand(QwtPicker::CrossRubberBand);
  m_vGraphPicker->setSelectionFlags(QwtPicker::PointSelection |
      QwtPicker::DragSelection  );
  QObject::connect( m_vGraphPicker, SIGNAL(mouseMoved()),
      this, SLOT(vGraphPickerMoved()) );
}


RefIVConnections::~RefIVConnections()
{
}


void RefIVConnections::closeViewer()
{
  m_ivMainWindow->close();
}


void RefIVConnections::toggleHScroll()
{
  bool is_on = m_ivUI->action_Hscroll->isChecked();
  m_ivUI->imageHorizontalScrollBar->setVisible( is_on );
  m_ivUI->imageHorizontalScrollBar->setEnabled( is_on );
  m_imageDisplay->updateImage();
}


void RefIVConnections::toggleVScroll()
{
  bool is_on = m_ivUI->action_Vscroll->isChecked();
  m_ivUI->imageVerticalScrollBar->setVisible( is_on );
  m_ivUI->imageVerticalScrollBar->setEnabled( is_on );
  m_imageDisplay->updateImage();
}


void RefIVConnections::imageHorizontalRangeChanged()
{
  m_imageDisplay->updateRange();
}


void RefIVConnections::graphRangeChanged()
{
  //  double value = (double)m_ivUI->graph_max_slider->value();
  //  double min   = (double)m_ivUI->graph_max_slider->minimum();
  //  double max   = (double)m_ivUI->graph_max_slider->maximum();
  //
  //  double range_scale = (value - min)/(max - min);
  //  if ( range_scale < 0.01 )
  //    range_scale = 0.01;
  //
  //  m_hGraphDisplay->setRangeScale( range_scale );
  //  m_vGraphDisplay->setRangeScale( range_scale );
}


void RefIVConnections::peakBackTofRangeUpdate()
{
  QLineEdit * peak_left_control = m_ivUI->lineEdit_peakLeft;
  double peakmin = peak_left_control->text().toDouble();

  QLineEdit * peak_right_control = m_ivUI->lineEdit_peakRight;
  double peakmax = peak_right_control->text().toDouble();

  QLineEdit * back_left_control = m_ivUI->lineEdit_backLeft;
  double backmin = back_left_control->text().toDouble();

  QLineEdit * back_right_control = m_ivUI->lineEdit_backRight;
  double backmax = back_right_control->text().toDouble();

  QLineEdit * tof_min_control = m_ivUI->lineEdit_TOFmin;
  double tofmin = tof_min_control->text().toDouble();

  QLineEdit * tof_max_control = m_ivUI->lineEdit_TOFmax;
  double tofmax = tof_max_control->text().toDouble();

  emit peakBackTofRangeUpdate(peakmin, peakmax, backmin, backmax, tofmin, tofmax);
}


void RefIVConnections::editManualInput()
{
  m_imageDisplay->updateImage();
  peakBackTofRangeUpdate();
}


void RefIVConnections::vScrollBarMoved()
{
  m_imageDisplay->updateImage();
}


void RefIVConnections::hScrollBarMoved()
{
  m_imageDisplay->updateImage();
}


void RefIVConnections::imageSplitterMoved()
{
  QList<int> sizes = m_ivUI->imageSplitter->sizes();
  QList<int> vgraph_sizes;
  vgraph_sizes.append( sizes[0] );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( sizes[1] );
  m_ivUI->vgraphSplitter->setSizes( vgraph_sizes );
  m_imageDisplay->updateImage();
}


// Right click
void RefIVConnections::imagePickerMoved()
{
  QwtPolygon selectedPoints = m_imagePicker->selection();
  if ( selectedPoints.size() >= 1 )
  {
    int index = selectedPoints.size() - 1;
    m_imageDisplay->setPointedAtPoint( selectedPoints[index] );
  }
}


// Left click
void RefIVConnections::imagePicker2Moved()
{
  QwtPolygon selectedPoints = m_imagePicker2->selection();
  if ( selectedPoints.size() >= 1 )
  {
    peakBackTofRangeUpdate();
    int index = selectedPoints.size() - 1;
    int mouseClick = 1;
    m_imageDisplay->setPointedAtPoint( selectedPoints[index], mouseClick );
    peakBackTofRangeUpdate();

  }
}


void RefIVConnections::hGraphPickerMoved()
{
  QwtPolygon selectedPoints = m_hGraphPicker->selection();
  if ( selectedPoints.size() >= 1 )
  {
    int index = selectedPoints.size() - 1;
    m_hGraphDisplay->setPointedAtPoint( selectedPoints[index]);
  }
}


void RefIVConnections::vGraphPickerMoved()
{
  QwtPolygon selectedPoints = m_vGraphPicker->selection();
  if ( selectedPoints.size() >= 1 )
  {
    int index = selectedPoints.size() - 1;
    m_vGraphDisplay->setPointedAtPoint( selectedPoints[index] );
  }
}


void RefIVConnections::intensitySliderMoved()
{
  double value = (double)m_ivUI->intensity_slider->value();
  double min   = (double)m_ivUI->intensity_slider->minimum();
  double max   = (double)m_ivUI->intensity_slider->maximum();

  double scaledValue = 100.0*(value - min)/(max - min);
  m_imageDisplay->setIntensity( scaledValue );
}


/* COLOUR MAP SLOTS */

void RefIVConnections::heatColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::grayColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::negativeGrayColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::NEGATIVE_GRAY,256, positiveColorTable);

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::greenYellowColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::GREEN_YELLOW, 256, positiveColorTable);

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::rainbowColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::RAINBOW, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::optimalColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::OPTIMAL, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::multiColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::MULTI, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


void RefIVConnections::spectrumColorScale()
{
  std::vector<QRgb> positiveColorTable;
  ColorMaps::GetColorMap( ColorMaps::SPECTRUM, 256, positiveColorTable );

  std::vector<QRgb> negativeColorTable;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negativeColorTable );

  m_imageDisplay->setColorScales( positiveColorTable, negativeColorTable );
  showColorScale( positiveColorTable, negativeColorTable );
}


/**
 *  Set the pix map that shows the color scale from the specified positive
 *  and negative color tables.
 *
 *  @param positiveColorTable  The new color table used to map positive data
 *                               values to an RGB color.
 *  @param negativeColorTable  The new color table used to map negative data
 *                               values to an RGB color.  This must have the
 *                               same number of entries as the positive
 *                               color table.
 */
void RefIVConnections::showColorScale( std::vector<QRgb> & positiveColorTable,
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
  m_ivUI->color_scale->setPixmap(pixmap);
}

} // namespace RefDetectorViewer
} // namespace MantidQt
