
#include <iostream>
#include <qwt_plot_canvas.h>

#include "MantidQtImageViewer/IVConnections.h"

namespace MantidQt
{
namespace ImageView
{


IVConnections::IVConnections( Ui_MainWindow* ui, 
                              ImageDisplay*  image_display,
                              GraphDisplay*  h_graph_display,
                              GraphDisplay*  v_graph_display )
{
  iv_ui = ui;
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

  image_picker = new TrackingPicker( iv_ui->imagePlot->canvas() );
  image_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  image_picker->setTrackerMode(QwtPicker::ActiveOnly);
  image_picker->setRubberBandPen(QColor(Qt::gray));

/* // point selections & connection works on mouse release
*/
  image_picker->setRubberBand(QwtPicker::CrossRubberBand);
  image_picker->setSelectionFlags(QwtPicker::PointSelection | 
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
  QObject::connect( image_picker, SIGNAL(mouseMoved()),
                    this, SLOT(imagePickerMoved()) );


  QObject::connect(iv_ui->imageSplitter, SIGNAL(splitterMoved(int,int)), 
                   this, SLOT(imageSplitterMoved()));

  QObject::connect(iv_ui->imageVerticalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(v_scroll_bar_moved() ) );

  QObject::connect(iv_ui->imageHorizontalScrollBar, SIGNAL(valueChanged(int)),
                   this, SLOT(h_scroll_bar_moved() ) );

  QObject::connect(iv_ui->action_Hscroll, SIGNAL(changed()),
                   this, SLOT(toggle_Hscroll()) );

  QObject::connect(iv_ui->action_Vscroll, SIGNAL(changed()),
                   this, SLOT(toggle_Vscroll()) );

  h_graph_picker = new TrackingPicker( iv_ui->h_graphPlot->canvas() );
  h_graph_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  h_graph_picker->setTrackerMode(QwtPicker::ActiveOnly);
  h_graph_picker->setRubberBandPen(QColor(Qt::gray));
  h_graph_picker->setRubberBand(QwtPicker::CrossRubberBand);
  h_graph_picker->setSelectionFlags(QwtPicker::PointSelection |
                                  QwtPicker::DragSelection  );
  QObject::connect( h_graph_picker, SIGNAL(mouseMoved()),
                    this, SLOT(h_graphPickerMoved()) );

  // NOTE: This initialization could be (static?) method ih TrackingPicker
  v_graph_picker = new TrackingPicker( iv_ui->v_graphPlot->canvas() );
  v_graph_picker->setMousePattern(QwtPicker::MouseSelect1, Qt::LeftButton);
  v_graph_picker->setTrackerMode(QwtPicker::ActiveOnly);
  v_graph_picker->setRubberBandPen(QColor(Qt::gray));
  v_graph_picker->setRubberBand(QwtPicker::CrossRubberBand);
  v_graph_picker->setSelectionFlags(QwtPicker::PointSelection |
                                    QwtPicker::DragSelection  );
  QObject::connect( v_graph_picker, SIGNAL(mouseMoved()),
                    this, SLOT(v_graphPickerMoved()) );
}

IVConnections::~IVConnections()
{
  delete image_picker;
  delete h_graph_picker;
  delete v_graph_picker;
}


void IVConnections::somethingChanged()
{
  std::cout << "somethingChanged() called" << std::endl;
}


void IVConnections::toggle_Hscroll()
{
  bool is_on = iv_ui->action_Hscroll->isChecked();
  iv_ui->imageHorizontalScrollBar->setVisible( is_on );
  iv_ui->imageHorizontalScrollBar->setEnabled( is_on );
  image_display->UpdateImage();
}


void IVConnections::toggle_Vscroll()
{
  bool is_on = iv_ui->action_Vscroll->isChecked();
  iv_ui->imageVerticalScrollBar->setVisible( is_on );
  iv_ui->imageVerticalScrollBar->setEnabled( is_on );
  image_display->UpdateImage();
}


void IVConnections::v_scroll_bar_moved()
{
  image_display->UpdateImage();
}


void IVConnections::h_scroll_bar_moved()
{
  image_display->UpdateImage();
}


void IVConnections::imageSplitterMoved()
{
  QList<int> sizes = iv_ui->imageSplitter->sizes();
/*
  std::cout << "height 0 = " << sizes[0] << 
               " height 1 = " << sizes[1] << std::endl;
*/
  QList<int> vgraph_sizes;
  vgraph_sizes.append( sizes[0] );
  vgraph_sizes.append( 30 );
  vgraph_sizes.append( sizes[1] );
  iv_ui->vgraphSplitter->setSizes( vgraph_sizes );
  image_display->UpdateImage();
}


void IVConnections::imagePickerMoved()
{
  QwtPolygon selected_points = image_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    image_display->SetPointedAtPoint( selected_points[index] );
  }
}


void IVConnections::h_graphPickerMoved()
{
  QwtPolygon selected_points = h_graph_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    h_graph_display->SetPointedAtPoint( selected_points[index] );
  }
}


void IVConnections::v_graphPickerMoved()
{
  QwtPolygon selected_points = v_graph_picker->selection();
  if ( selected_points.size() >= 1 )
  {
    int index = selected_points.size() - 1;
    v_graph_display->SetPointedAtPoint( selected_points[index] );
  }
}


} // namespace MantidQt 
} // namespace ImageView 

