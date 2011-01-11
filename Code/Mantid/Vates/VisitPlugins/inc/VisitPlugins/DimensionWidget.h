#ifndef DIMENSION_WIDGET_H
#define DIMENSION_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>
#include <memory>
#include "boost/shared_ptr.hpp"
//Foward decs
class QLabel;
class QComboBox;

namespace Mantid
{
 namespace MDGeometry
 {
 class IMDDimension;
 }
}


class DimensionWidget: public QWidget
{
Q_OBJECT
public:

  DimensionWidget(boost::shared_ptr<Mantid::MDGeometry::IMDDimension> spDimensionToRender);

  ~DimensionWidget();

};

#endif
