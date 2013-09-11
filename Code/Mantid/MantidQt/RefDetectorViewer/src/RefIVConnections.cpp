
#include <iostream>
#include <QLineEdit>
#include <qwt_plot_canvas.h>

#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"
#include "MantidQtSpectrumViewer/IVUtils.h"
#include "MantidQtSpectrumViewer/ErrorHandler.h"

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
 * @param ui               The object containing the gui components for 
 *                         the ImageView viewer.
 * @param iv_main_window   The main window.
 * @param image_display    The SpectrumDisplay object that will dispaly the
 *                         image
 * @param h_graph_display  The GraphDisplay object that will display 
 *                         horizontal cuts through the image
 * @param v_graph_display  The GraphDisplay object that will display 
 *                         vertical cuts through the image
 *
 */
RefIVConnections::RefIVConnections( Ui_RefImageViewer* ui, 
                              RefImageView*     iv_main_window,
                              RefImageDisplay*  image_display,
                              GraphDisplay*  h_graph_display,
                              GraphDisplay*  v_graph_display )
{
  iv_ui = ui;
                              // first disable a few un-implemented controls
  iv_ui->menuGraph_Selected->setDisabled(true);
  iv_ui->actionClear_Selections->setDisabled(true);
  iv_ui->actionOverlaid->setDisabled(true);
  iv_ui->actionOffset_Vertically->setDisabled(true);
  iv_ui->actionOffset_Diagonally->setDisabled(true);
  iv_ui->actionGraph_Rebinned_Data->setDisabled(true);
  iv_ui->menuHelp->setDisabled(true);
 
  this->iv_main_window = iv_main_window;
  QObject::connect( iv_ui->actionClose, SIGNAL(triggered()),
                    this, SLOT(close_viewer()) );
 
                              // now set up the gui components
  this->image_display   = image_display;
  this->h_graph_display = h_graph_display;
  this->v_graph_display = v_graph_display;

  QList<int> image_sizes;
  image_sizes.append( 500 );
  image_sizes.append( 250 );
  iv_ui->imageSplitter->setSizes( image_sizes );
  QList<int> vgraph_sizes;
  vgraph_sizes.append( 500 );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( 220 );
  iv_ui->vgraphSplitter->setSizes( vgraph_sizes );

  QList<int> horiz_sizes;
  horiz_sizes.append( 250 );
  horiz_sizes.append( 750 );
  horiz_sizes.append( 150 );
  iv_ui->left_right_splitter->setSizes( horiz_sizes );

  iv_ui->imageHorizontalScrollBar->setFocusPolicy( Qt::StrongFocus );
  iv_ui->imageHorizontalScrollBar->setMinimum(20);
  iv_ui->imageHorizontalScrollBar->setMaximum(2000);
  iv_ui->imageHorizontalScrollBar->setPageStep(30);
  iv_ui->imageHorizontalScrollBar->setSingleStep(30/2);

  iv_ui->imageVerticalScrollBar->setFocusPolicy( Qt::StrongFocus );
  iv_ui->imageVerticalScrollBar->setMinimum(0);
  iv_ui->imageVerticalScrollBar->setMaximum(10000000);
  iv_ui->imageVerticalScrollBar->setPageStep(500);
  iv_ui->imageVerticalScrollBar->setSingleStep(500/2);

  iv_ui->action_Hscroll->setCheckable(true);
  iv_ui->action_Hscroll->setChecked(false);
  iv_ui->imageHorizontalScrollBar->hide();
  iv_ui->imageHorizontalScrollBar->setEnabled(false);

  iv_ui->action_Vscroll->setCheckable(true);
  iv_ui->action_Vscroll->setChecked(true);
  iv_ui->imageVerticalScrollBar->show();
  iv_ui->imageVerticalScrollBar->setEnabled(true);

  iv_ui->intensity_slider->setTickInterval(10);
  iv_ui->intensity_slider->setTickPosition(QSlider::TicksBelow);
  iv_ui->intensity_slider->setSliderPosition(30);

//  iv_ui->graph_max_slider->setTickInterval(10);
//  iv_ui->graph_max_slider->setTickPosition(QSlider::TicksBelow);
//  iv_ui->graph_max_slider->setSliderPosition(100);

    image_picker2 = new TrackingPicker( iv_ui->imagePlot->canvas() );
    image_picker2->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
    image_picker2->setTrackerMode(QwtPicker::ActiveOnly);
    image_picker2->setRubberBandPen(QColor(Qt::gray));

    image_picker = new TrackingPicker( iv_ui->imagePlot->canvas() );
    image_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
    image_picker->setTrackerMode(QwtPicker::ActiveOnly);
    image_picker->setRubberBandPen(QColor(Qt::blue));

    
/* // point selections & connection works on mouse release
*/
  image_picker->setRubberBand(QwtPicker::CrossRubberBand);
  image_picker->setSelectionFlags(QwtPicker::PointSelection | 
                                  QwtPicker::DragSelection  );

    image_picker2->setRubberBand(QwtPicker::CrossRubberBand);
    image_picker2->setSelectionFlags(QwtPicker::PointSelection | 
                                    QwtPicker::DragSelection  );

    /*
  QObject::connect( image_picker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*  // point selection works on mouse click, NO CROSSHAIRS...

  image_picker->setRubberBand(QwtPicker::CrossRubberBand);
  image_picker->setSelectionFlags(QwtPicker::PointSelection | 
                                  QwtPicker::ClickSelection  );
  QObject::connect( image_picker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*  // rect selection calls SLOT on mouse release
  
  image_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::MidButton);
  image_picker->setRubberBand(QwtPicker::RectRubberBand);
  image_picker->setSelectionFlags(QwtPicker::RectSelection | 
                                  QwtPicker::DragSelection  );
  QObject::connect( image_picker, SIGNAL(selected(const QwtPolygon &)),
                    this, SLOT(imagePickerSelectedPoint()) );
*/

/*
  image_picker->setRubberBand(QwtPicker::CrossRubberBand);
  image_picker->setSelectionFlags(QwtPicker::PointSelection | 
                                  QwtPicker::ClickSelection  );
*/

    QObject::connect( image_picker2, SIGNAL(mouseMoved()),
                     this, SLOT(imagePicker2_moved()) );

    QObject::connect( image_picker, SIGNAL(mouseMoved()),
                     this, SLOT(imagePicker_moved()) );

    /*
     * Connections on the peak, back and TOF input boxes
     */
    QObject::connect(iv_ui->lineEdit_peakLeft, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
    QObject::connect(iv_ui->lineEdit_peakRight, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
    QObject::connect(iv_ui->lineEdit_backLeft, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
    QObject::connect(iv_ui->lineEdit_backRight, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
    QObject::connect(iv_ui->lineEdit_TOFmin, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
    QObject::connect(iv_ui->lineEdit_TOFmax, SIGNAL(returnPressed()),
                     this, SLOT(edit_manual_input()) );
                     
  QObject::connect(iv_ui->imageSplitter, SIGNAL(splitterMoved(int,int)), 
                   this, SLOT(imageSplitter_moved()) );

  QObject::connect(iv_ui->x_min_input, SIGNAL( returnPressed() ),
                   this, SLOT(image_horizontal_range_changed()) );

  QObject::connect(iv_ui->x_max_input, SIGNAL( returnPressed() ),
                   this, SLOT(image_horizontal_range_changed()) );

//  QObject::connect(iv_ui->step_input, SIGNAL( returnPressed() ),
//                   this, SLOT(image_horizontal_range_changed()) );

  QObject::connect(iv_ui->imageVerticalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(v_scroll_bar_moved()) );

  QObject::connect(iv_ui->imageHorizontalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(h_scroll_bar_moved()) );

  QObject::connect(iv_ui->action_Hscroll, SIGNAL(changed()),
                   this, SLOT(toggle_Hscroll()) );

  QObject::connect(iv_ui->action_Vscroll, SIGNAL(changed()),
                   this, SLOT(toggle_Vscroll()) );

  QObject::connect(iv_ui->intensity_slider, SIGNAL(valueChanged(int)),
                   this, SLOT(intensity_slider_moved()) );

//  QObject::connect(iv_ui->graph_max_slider, SIGNAL(valueChanged(int)),
//                   this, SLOT(graph_range_changed()) );

                                                     // color scale selections 
  iv_ui->actionHeat->setCheckable(true);
  iv_ui->actionHeat->setChecked(true);
  iv_ui->actionGray->setCheckable(true);
  iv_ui->actionNegative_Gray->setCheckable(true);
  iv_ui->actionGreen_Yellow->setCheckable(true);
  iv_ui->actionRainbow->setCheckable(true);
  iv_ui->actionOptimal->setCheckable(true);
  iv_ui->actionMulti->setCheckable(true);
  iv_ui->actionSpectrum->setCheckable(true);
                                                    // set up initial color
                                                    // scale display
  iv_ui->color_scale->setScaledContents(true);
  iv_ui->color_scale->setMinimumHeight(15);
  iv_ui->color_scale->setMinimumWidth(15);
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  ShowColorScale( positive_color_table, negative_color_table );


  color_group = new QActionGroup(this);
  color_group->addAction(iv_ui->actionHeat);
  color_group->addAction(iv_ui->actionGray);
  color_group->addAction(iv_ui->actionNegative_Gray);
  color_group->addAction(iv_ui->actionGreen_Yellow);
  color_group->addAction(iv_ui->actionRainbow);
  color_group->addAction(iv_ui->actionOptimal);
  color_group->addAction(iv_ui->actionMulti);
  color_group->addAction(iv_ui->actionSpectrum);

  QObject::connect(iv_ui->actionHeat, SIGNAL(triggered()),
                   this, SLOT(heat_color_scale()) );

  QObject::connect(iv_ui->actionGray, SIGNAL(triggered()),
                   this, SLOT(gray_color_scale()) );

  QObject::connect(iv_ui->actionNegative_Gray, SIGNAL(triggered()),
                   this, SLOT(negative_gray_color_scale()) );

  QObject::connect(iv_ui->actionGreen_Yellow, SIGNAL(triggered()),
                   this, SLOT(green_yellow_color_scale()) );

  QObject::connect(iv_ui->actionRainbow, SIGNAL(triggered()),
                   this, SLOT(rainbow_color_scale()) );

  QObject::connect(iv_ui->actionOptimal, SIGNAL(triggered()),
                   this, SLOT(optimal_color_scale()) );

  QObject::connect(iv_ui->actionMulti, SIGNAL(triggered()),
                   this, SLOT(multi_color_scale()) );

  QObject::connect(iv_ui->actionSpectrum, SIGNAL(triggered()),
                   this, SLOT(spectrum_color_scale()) );

  h_graph_picker = new TrackingPicker( iv_ui->h_graphPlot->canvas() );
  h_graph_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
  h_graph_picker->setTrackerMode(QwtPicker::ActiveOnly);
  h_graph_picker->setRubberBandPen(QColor(Qt::gray));
  h_graph_picker->setRubberBand(QwtPicker::CrossRubberBand);
  h_graph_picker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::DragSelection  );
  QObject::connect( h_graph_picker, SIGNAL(mouseMoved()),
                    this, SLOT(h_graphPicker_moved()) );

  // NOTE: This initialization could be a (static?) method in TrackingPicker
  v_graph_picker = new TrackingPicker( iv_ui->v_graphPlot->canvas() );
  v_graph_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::RightButton);
  v_graph_picker->setTrackerMode(QwtPicker::ActiveOnly);
  v_graph_picker->setRubberBandPen(QColor(Qt::gray));
  v_graph_picker->setRubberBand(QwtPicker::CrossRubberBand);
  v_graph_picker->setSelectionFlags(QwtPicker::PointSelection |
                                    QwtPicker::DragSelection  );
  QObject::connect( v_graph_picker, SIGNAL(mouseMoved()),
                    this, SLOT(v_graphPicker_moved()) );
}


RefIVConnections::~RefIVConnections()
{
  // std::cout << "IVConnections destructor called" << std::endl;

  delete image_picker;
  delete image_picker2;
  delete h_graph_picker;
  delete v_graph_picker;
  delete color_group;
}


void RefIVConnections::close_viewer()
{
  iv_main_window->close();
}


void RefIVConnections::toggle_Hscroll()
{
  bool is_on = iv_ui->action_Hscroll->isChecked();
  iv_ui->imageHorizontalScrollBar->setVisible( is_on );
  iv_ui->imageHorizontalScrollBar->setEnabled( is_on );
  image_display->UpdateImage();
}


void RefIVConnections::toggle_Vscroll()
{
  bool is_on = iv_ui->action_Vscroll->isChecked();
  iv_ui->imageVerticalScrollBar->setVisible( is_on );
  iv_ui->imageVerticalScrollBar->setEnabled( is_on );
  image_display->UpdateImage();
}


void RefIVConnections::image_horizontal_range_changed()
{
  image_display->UpdateRange();
}


void RefIVConnections::graph_range_changed()
{
//  double value = (double)iv_ui->graph_max_slider->value();
//  double min   = (double)iv_ui->graph_max_slider->minimum();
//  double max   = (double)iv_ui->graph_max_slider->maximum();
//
//  double range_scale = (value - min)/(max - min);
//  if ( range_scale < 0.01 )
//    range_scale = 0.01;
//
//  h_graph_display->SetRangeScale( range_scale );
//  v_graph_display->SetRangeScale( range_scale );
}

void RefIVConnections::peak_back_tof_range_update()
{
    QLineEdit * peak_left_control = iv_ui->lineEdit_peakLeft;
    double peakmin = peak_left_control->text().toDouble();

    QLineEdit * peak_right_control = iv_ui->lineEdit_peakRight;
    double peakmax = peak_right_control->text().toDouble();
    
    QLineEdit * back_left_control = iv_ui->lineEdit_backLeft;
    double backmin = back_left_control->text().toDouble();
    
    QLineEdit * back_right_control = iv_ui->lineEdit_backRight;
    double backmax = back_right_control->text().toDouble();
    
    QLineEdit * tof_min_control = iv_ui->lineEdit_TOFmin;
    double tofmin = tof_min_control->text().toDouble();
    
    QLineEdit * tof_max_control = iv_ui->lineEdit_TOFmax;
    double tofmax = tof_max_control->text().toDouble();
    
    emit peak_back_tof_range_update(peakmin, peakmax, backmin, backmax, tofmin, tofmax);
}

void RefIVConnections::edit_manual_input()
{
    image_display->UpdateImage();
    peak_back_tof_range_update();

}
    
    
void RefIVConnections::v_scroll_bar_moved()
{
  image_display->UpdateImage();
}


void RefIVConnections::h_scroll_bar_moved()
{
  image_display->UpdateImage();
}


void RefIVConnections::imageSplitter_moved()
{
  QList<int> sizes = iv_ui->imageSplitter->sizes();
  QList<int> vgraph_sizes;
  vgraph_sizes.append( sizes[0] );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( sizes[1] );
  iv_ui->vgraphSplitter->setSizes( vgraph_sizes );
  image_display->UpdateImage();
}


  //Right click
void RefIVConnections::imagePicker_moved()
{
  QwtPolygon selected_points = image_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    image_display->SetPointedAtPoint( selected_points[index] );
  }
}

  //Left click
void RefIVConnections::imagePicker2_moved()
{
  QwtPolygon selected_points = image_picker2->selection();
    if ( selected_points.size() >= 1 )
    {
      peak_back_tof_range_update();
        int index = selected_points.size() - 1;
        int mouseClick = 1; 
        image_display->SetPointedAtPoint( selected_points[index], mouseClick );
        peak_back_tof_range_update();

    }
}


void RefIVConnections::h_graphPicker_moved()
{
  QwtPolygon selected_points = h_graph_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    h_graph_display->SetPointedAtPoint( selected_points[index]);
  }
}


void RefIVConnections::v_graphPicker_moved()
{
  QwtPolygon selected_points = v_graph_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    v_graph_display->SetPointedAtPoint( selected_points[index] );
  }
}

void RefIVConnections::intensity_slider_moved()
{
  double value = (double)iv_ui->intensity_slider->value();
  double min   = (double)iv_ui->intensity_slider->minimum();
  double max   = (double)iv_ui->intensity_slider->maximum();

  double scaled_value = 100.0*(value - min)/(max - min);
  image_display->SetIntensity( scaled_value );
}

void RefIVConnections::heat_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::gray_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::negative_gray_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::NEGATIVE_GRAY,256, positive_color_table);

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::HEAT, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::green_yellow_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::GREEN_YELLOW, 256, positive_color_table);

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::rainbow_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::RAINBOW, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::optimal_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::OPTIMAL, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::multi_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::MULTI, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}

void RefIVConnections::spectrum_color_scale()
{
  std::vector<QRgb> positive_color_table;
  ColorMaps::GetColorMap( ColorMaps::SPECTRUM, 256, positive_color_table );

  std::vector<QRgb> negative_color_table;
  ColorMaps::GetColorMap( ColorMaps::GRAY, 256, negative_color_table );

  image_display->SetColorScales( positive_color_table, negative_color_table );
  ShowColorScale( positive_color_table, negative_color_table );
}


/**
 *  Set the pix map that shows the color scale from the specified positive
 *  and negative color tables.
 *
 *  @param positive_color_table  The new color table used to map positive data 
 *                               values to an RGB color.
 *  @param negative_color_table  The new color table used to map negative data 
 *                               values to an RGB color.  This must have the
 *                               same number of entries as the positive
 *                               color table.
 */
void RefIVConnections::ShowColorScale( std::vector<QRgb> & positive_color_table,
                                    std::vector<QRgb> & negative_color_table )
{
  size_t total_colors = positive_color_table.size() + 
                        negative_color_table.size();

  unsigned int *rgb_data = new unsigned int[ total_colors ];

  size_t index = 0;
  size_t n_colors = negative_color_table.size();
  for ( size_t i = 0; i < n_colors; i++ )
  {
    rgb_data[index] = negative_color_table[ n_colors - 1 - i ];
    index++;
  }

  n_colors = positive_color_table.size();
  for ( size_t i = 0; i < n_colors; i++ )
  {
    rgb_data[index] = positive_color_table[i];
    index++;
  }

  uchar *buffer = (uchar*)rgb_data;
  QImage image( buffer, (int)total_colors, 1, QImage::Format_RGB32 );
  QPixmap pixmap = QPixmap::fromImage(image);
  iv_ui->color_scale->setPixmap( pixmap );

  delete[] rgb_data;
}

} // namespace RefDetectorViewer
} // namespace MantidQt 
