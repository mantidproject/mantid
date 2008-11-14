#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TableRow.h"

namespace Mantid
{
namespace DataObjects
{

TableRow::TableRow(const TableRowHelper& trh):m_columns(trh.m_workspace->m_columns),m_row(trh.m_row),m_col(0),m_sep(",")
{
    if (m_columns.size()) m_nrows = int(m_columns[0]->size());
    else
        m_nrows = 0;
}
void TableRow::row(int i)
{
    if (i >= 0 && i < m_nrows)
    {
        m_row = i;
        m_col = 0;
    }
}
bool TableRow::next()
{
    if (m_row < m_nrows - 1)
    {
        ++m_row;
        m_col = 0;
        return true;
    }
    return false;
}
bool TableRow::prev()
{
    if (m_row > 0)
    {
        --m_row;
        m_col = 0;
        return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream& s,const TableRow& row)
{
    if (row.m_columns.size() == 0) return s;
    if (row.m_columns.size() == 1)
    {
        row.m_columns[0]->print(s,row.row());
        return s;
    }
    std::vector< boost::shared_ptr<Column> >::const_iterator ci = row.m_columns.begin();
    for(;ci!=row.m_columns.end()-1;ci++)
    {
        (*ci)->print(s,row.row());
        s << row.m_sep;
    }
    (*ci)->print(s,row.row());
    return s;
}

} // namespace DataObjects
} // namespace Mantid

