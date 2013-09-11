
#include "MantidQtSpectrumViewer/QtUtils.h" 
#include "MantidQtSpectrumViewer/SVUtils.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Set the specified string as the entry at the specified row and col of
 * the specified table.
 *
 * @param row     The row number where the string should appear.
 * @param col     The column number where the string should appear.
 * @param string  The string that will be placed in the table.
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
 * @param precision  The number of significant figures to use when formatting
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
  SVUtils::Format( width, precision, value, str );
  SetTableEntry( row, col, str, table );
}


/**
 * Set the specified string into the specified QLineEdit widget. 
 *
 * @param string       The string that will be placed in the widget.
 * @param q_line_edit  Pointer to the QLineEdit widget.
 */
void  QtUtils::SetText( const std::string & string,
                              QLineEdit*    q_line_edit )
{
  QString q_string = QString::fromStdString( string );
  q_line_edit->setText( q_string );
}


/**
 * Format and set the specified double as the text in the specified  
 * QLineEdit widget. 
 *
 * @param width       The number of spaces to use when formatting the value.
 * @param precision    The number of significant figures to use when formatting
 *                    the value.
 * @param value       The number to be formatted and placed in the table.
 * @param q_line_edit Pointer to the QLineEdit widget.
 */
void  QtUtils::SetText( int        width,
                        int        precision,
                        double     value,
                        QLineEdit* q_line_edit )
{
  std::string str;
  SVUtils::Format( width, precision, value, str );
  SetText( str, q_line_edit );
}


} // namespace SpectrumView
} // namespace MantidQt 
