
#include "MantidQtImageViewer/QtUtils.h" 
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

/**
 * Set the specified string as the entry at the specified row and col of
 * the specified table.
 *
 * @param row     The row number where the string should appear.
 * @param col     The column number where the string should appear.
 * @param string  The string that is place in the table.
 * @param table   Pointer to the table
 */
void QtUtils::SetTableEntry(       int           row, 
                                   int           col, 
                             const std::string & string,
                                   QTableWidget* table )
{
  QString q_string = QString::fromStdString( string );
  QTableWidgetItem *item = new QTableWidgetItem( q_string );
  table->setItem( row, col, item );
}


/**
 * Format and set the specified double as the entry at the specified row 
 * and col of the specified table.
 *
 * @param row       The row number where the string should appear.
 * @param col       The column number where the string should appear.
 * @param width     The number of spaces to use when formatting the value.
 * @param precison  The number of significant figures to use when formatting
 *                  the value.
 * @param value     The number to be formatted and placed in the table.
 * @param table     Pointer to the table
 */
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
