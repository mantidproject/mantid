
#include "MantidQtImageViewer/QtUtils.h" 
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

void QtUtils::SetTableEntry(       int           row, 
                                   int           col, 
                             const std::string & string,
                                   QTableWidget* table )
{
  QString q_string = QString::fromStdString( string );
  QTableWidgetItem *item = new QTableWidgetItem( q_string );
  table->setItem( row, col, item );
}


void  QtUtils::SetTableEntry( int           row, 
                              int           col, 
                              int           width,
                              int           precision,
                              double        value,
                              QTableWidget* table )
{
  std::string str;
  IVUtils::Format( width, precision, value, str );
  SetTableEntry( row, col, str, table );
}


} // namespace MantidQt 
} // namespace ImageView
