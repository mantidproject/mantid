
#include <iostream>

#include <qapplication.h>                                                        
#include <QMainWindow>
#include <QtGui>

#include "MantidQtImageViewer/ImageView.h"
#include "MantidQtImageViewer/TestDataSource.h"

using namespace MantidQt;
using namespace ImageView;

int main( int argc, char **argv )
{
  QApplication a( argc, argv );

  TestDataSource* source = new TestDataSource( 10, 110, 220, 320, 3000, 2000 );
  MantidQt::ImageView::ImageView image_view( source );

  return a.exec();
}

