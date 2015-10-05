#include "MantidMatrixModel.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/ReadLock.h"

#include "MantidGeometry/IDetector.h"

#include <QObject>
#include <QVariant>
// ----------   MantidMatrixModel   ------------------ //

/**   MantidMatrixModel constructor.
@param parent :: Pointer to the parent MantidMatrix
@param ws :: Underlying workspace
@param rows :: Number of rows in the workspace to be visible via MantidMatrixModel
@param cols :: Number of columns (time bins)
@param start :: Starting index
@param type :: Type of the data to display: Y, X, or E
*/
MantidMatrixModel::MantidMatrixModel(QObject *parent,
                                     const Mantid::API::MatrixWorkspace* ws,
                                     int rows,
                                     int cols,
                                     int start,
                                     Type type):
QAbstractTableModel(parent),m_type(type),
  m_format('e'),m_prec(6)
{
  setup(ws,rows,cols,start);
}

/// Call this function if the workspace has changed
void MantidMatrixModel::setup(const Mantid::API::MatrixWorkspace* ws,
                              int rows,
                              int cols,
                              int start)
{
  m_workspace = ws;
  m_rows = rows;
  m_cols = cols;
  m_colNumCorr = 1;
  m_endRow = m_rows - 1;
  m_startRow = start >= 0? start : 0;
  m_mon_color = QColor(255,255,204);
  if (ws->blocksize() != 0)
    m_colNumCorr = ws->isHistogramData() ? 1 : 0;
  else
    m_colNumCorr = 0;
}


double MantidMatrixModel::data(int row, int col) const
{
  Mantid::Kernel::ReadLock _lock(*m_workspace);

  double val;
  if (m_type == X)
  {
    val = m_workspace->readX(row + m_startRow)[col];
  }
  else if (m_type == Y)
  {
    val = m_workspace->readY(row + m_startRow)[col];
  }
  else if (m_type == E)
  {
    val = m_workspace->readE(row + m_startRow)[col];
  } else
  {
    val = m_workspace->readDx(row + m_startRow)[col];
  }
  return val;
}

QVariant MantidMatrixModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
  if (!(role == Qt::DisplayRole || role == Qt::ToolTipRole)) return QVariant();
  if (orientation == Qt::Vertical && m_workspace->axes() > 1)
  {
    Mantid::API::TextAxis* xAxis = dynamic_cast<Mantid::API::TextAxis*>(m_workspace->getAxis(1));
    if (xAxis)
    {
      return QString::fromStdString(xAxis->label(section));
    }
  }

  //initialise with horizontal values
  int axisIndex = 0;
  QString toolTipSeperator = "\n";
  QString headerSeperator = "\n";
  if (orientation == Qt::Vertical) 
  {
    axisIndex = 1;
    toolTipSeperator = "\n";
    headerSeperator = " ";
  }

  if (m_workspace->axes() > axisIndex) //if the axis exists
  {
    Mantid::API::Axis* axis = m_workspace->getAxis(axisIndex);
    Mantid::API::TextAxis* textAxis = dynamic_cast<Mantid::API::TextAxis*>(axis);
    if (textAxis) //just return the text label
    {
      return QString::fromStdString(textAxis->label(section));
    }

    Mantid::API::SpectraAxis* spAxis = dynamic_cast<Mantid::API::SpectraAxis*>(axis);
    if (spAxis)
    {
      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2spectra no %3").arg(QString::number(section), toolTipSeperator, 
          QString::number(m_workspace->getSpectrum(section)->getSpectrumNo()));          
      }
      else
      {
        return QString("%1%2sp-%3").arg(QString::number(section), headerSeperator,
          QString::number(m_workspace->getSpectrum(section)->getSpectrumNo()));
      }
    }

    
    QString unit = QString::fromStdWString( axis->unit()->label().utf8());

    // Handle RefAxis for X axis
    Mantid::API::RefAxis* refAxis = dynamic_cast<Mantid::API::RefAxis*>(axis);
    if (refAxis && axisIndex == 0)
    {
      //still need to protect from ragged workspaces
      if (m_type==X || m_type==DX)
      {
        if (role == Qt::ToolTipRole) 
        {
          return QString("index %1").arg(QString::number(section));
        }
        else
        {
          return section;
        }
      }

      if (!m_workspace->isCommonBins())
      {
        if (role == Qt::ToolTipRole) 
        {
          return QString("index %1%2bin centre value varies%3Rebin to set common bins").arg(QString::number(section),toolTipSeperator,toolTipSeperator);
        }
        else
        {
          return QString("%1%2bins vary").arg(QString::number(section),headerSeperator);
        }
      }

      //get bin centre value
      double binCentreValue;
      const Mantid::MantidVec xVec = m_workspace->readX(0);
      if (m_workspace->isHistogramData())
      {
        if ((section+1) >= static_cast<int>(xVec.size())) return section;
        binCentreValue= (xVec[section] + xVec[section+1])/2.0;
      }
      else
      {
        if (section >= static_cast<int>(xVec.size())) return section;
        binCentreValue = xVec[section];
      }

      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2%3 %4%5 (bin centre)").arg(QString::number(section), toolTipSeperator,
          QString::fromStdString(axis->unit()->caption()),
          QString::number(binCentreValue), unit);
      }
      else
      {
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator,
          QString::number(binCentreValue), unit);
      }
    }

    // Handle BinEdgeAxis for vertical axis
    Mantid::API::BinEdgeAxis* binEdgeAxis = dynamic_cast<Mantid::API::BinEdgeAxis*>(axis);
    if (binEdgeAxis && axisIndex == 1)
    {
      const Mantid::MantidVec axisBinEdges = binEdgeAxis->getValues();
      double binCentreValue = (axisBinEdges[section] + axisBinEdges[section + 1]) / 2.0;

      if (role == Qt::ToolTipRole)
      {
        return QString("index %1%2%3 %4%5 (bin centre)").arg(QString::number(section), toolTipSeperator,
          QString::fromStdString(axis->unit()->caption()),
          QString::number(binCentreValue), unit);
      }
      else
      {
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator,
          QString::number(binCentreValue), unit);
      }
    }

    Mantid::API::NumericAxis* numAxis = dynamic_cast<Mantid::API::NumericAxis*>(axis);
    if (numAxis)
    {
      QString valueString;
      try
      {
        valueString = QString::number(numAxis->getValue(section));          
      }
      catch (Mantid::Kernel::Exception::IndexError&)
      {
        valueString="";
      }

      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2%3 %4%5").arg(QString::number(section), toolTipSeperator, 
            QString::fromStdString(axis->unit()->caption()),
            valueString, unit);          
      }
      else
      {
        if (headerSeperator == " ") headerSeperator = "   ";
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator, 
          valueString, unit);
      }
    }

  }
  // fall through value, just return the section value
  return section;


}

Qt::ItemFlags MantidMatrixModel::flags(const QModelIndex & index ) const
{
  // MG: For item selection to work correclty in later Qt versions it must be marked as enabled
  if (index.isValid())
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  else
    return Qt::ItemIsEnabled;
}

/**
@param f :: Number format:  'f' - fixed, 'e' - scientific, 'g' - shorter of the earlier two.
@param prec :: New precision (number of digits after the decimal point) with which the data will
be shown in MantidMatrix.
*/
void MantidMatrixModel::setFormat(const QChar& f,int prec)
{
  QString formats = " efg";
  if ( formats.indexOf(f) > 0 )
  {
    m_format = f.toAscii();
    m_prec = prec;
  }
}

QVariant MantidMatrixModel::data(const QModelIndex &index, int role) const
{  
  switch (role)
  {
  case Qt::DisplayRole:
    {
      double val = data(index.row(),index.column());
      return QVariant(m_locale.toString(val,m_format,m_prec));
    }
  case Qt::BackgroundRole:
    {
      if (checkMonitorCache(index.row()))
      {
        return m_mon_color;
      }
      else
      {
        return QVariant();
      }
    }
  default:
    return QVariant();
  }
}

/**   Checks the row cache to see if the detector flag is stored, then returns it, otherwise it looks it up and adds it to the cache for quick lookup
@param row :: current row in the table that maps to a detector.
@return bool :: the value of if the detector is a monitor or not.
*/
bool MantidMatrixModel::checkMonitorCache(int row) const
{
  row += m_startRow; //correctly offset the row
  if (m_workspace->axes() > 1 && m_workspace->getAxis(1)->isSpectra())
  {
    bool isMon = false;
    if (m_monCache.contains(row))
    {
      isMon = m_monCache.value(row);
    }
    else
    {
      try
      {
        size_t wsIndex = static_cast<size_t>(row);
        auto det = m_workspace->getDetector(wsIndex);
        if (det->isMonitor())
        {
          isMon = true;
        }
        else
        {
          isMon = false;
        }
        m_monCache.insert(row, isMon);
      }
      catch (std::exception&)
      {
        m_monCache.insert(row,false);
        isMon = false;
      }
    }
    return isMon;
  }
  else
  {
    return false;
  }
}

