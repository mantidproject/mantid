/**
 *  File: MatrixWSDataSource.cpp
 */

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sstream>
#include <math.h>

#include <QThread>

#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"
#include "MantidQtSpectrumViewer/EModeHandler.h"
#include "MantidQtSpectrumViewer/SVUtils.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/Run.h"


using namespace Mantid;
using namespace Kernel;
using namespace API;
using namespace Geometry;


namespace
{
  Kernel::Logger g_log("SpectrumView");
}


namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct a DataSource object around the specifed MatrixWorkspace.
 *
 * @param matWs  Shared pointer to the matrix workspace being "wrapped"
 */
MatrixWSDataSource::MatrixWSDataSource( MatrixWorkspace_const_sptr matWs ) :
  SpectrumDataSource( 0.0, 1.0, 0.0, 1.0, 0, 0 ),
  m_matWs(matWs),
  m_emodeHandler(NULL)
{
  m_totalXMin = matWs->getXMin();
  m_totalXMax = matWs->getXMax();

  m_totalYMin = 0;  // Y direction is spectrum index
  m_totalYMax = (double)matWs->getNumberHistograms();

  m_totalRows = matWs->getNumberHistograms();
  m_totalCols = 1000000;  // Default data resolution
}


MatrixWSDataSource::~MatrixWSDataSource()
{
}


bool MatrixWSDataSource::hasData(const std::string& wsName,
                                 const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (m_matWs->getName() == wsName)
    return true;

  Mantid::API::MatrixWorkspace_const_sptr other = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  if (!other)
    return false;

  return (m_matWs == other);
}


/**
 * Get the smallest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::getXMin()
{
  m_totalXMin = m_matWs->getXMin();
  return m_totalXMin;
}


/**
 * Get the largest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::getXMax()
{
  m_totalXMax = m_matWs->getXMax();
  return m_totalXMax;
}


/**
 * Get the largest 'y' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::getYMax()
{
  m_totalYMax = (double)m_matWs->getNumberHistograms();
  return m_totalYMax;
}


/**
 * Get the total number of rows the data is divided into.  Must override base
 * class method, since the DataSource can be changed!
 */
size_t MatrixWSDataSource::getNRows()
{
  m_totalYMax = (double)m_matWs->getNumberHistograms();
  return m_totalRows;
}


/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.  NOTE: The calling code is responsible for deleting the
 * DataArray that is constructed in and returned by this method.
 *
 * @param xMin    Left edge of region to be covered.
 * @param xMax    Right edge of region to be covered.
 * @param yMin    Bottom edge of region to be covered.
 * @param yMax    Top edge of region to be covered.
 * @param numRows Number of rows to return. If the number of rows is less
 *                than the actual number of data rows in [yMin,yMax], the
 *                data will be subsampled, and only the specified number
 *                of rows will be returned.
 * @param numCols The specrum data will be rebinned using the specified
 *                number of colums.
 * @param isLogX  Flag indicating whether or not the data should be
 *                binned logarithmically.
 */
DataArray_const_sptr MatrixWSDataSource::getDataArray( double xMin,    double  xMax,
                                                       double yMin,    double  yMax,
                                                       size_t numRows, size_t  numCols,
                                                       bool   isLogX )
{
  /* Since we're rebinning, the columns can be arbitrary */
  /* but rows must be aligned to get whole spectra */
  size_t first_row;
  SVUtils::CalculateInterval( m_totalYMin, m_totalYMax, m_totalRows,
                              first_row, yMin, yMax, numRows );

  std::vector<float> newData(numRows * numCols);

  MantidVec xScale;
  xScale.resize(numCols + 1);
  if ( isLogX )
  {
    for ( size_t i = 0; i < numCols+1; i++ )
    {
      xScale[i] = xMin * exp ( (double)i / (double)numCols * log(xMax/xMin) );
    }
  }
  else
  {
    double dx = (xMax - xMin) / ((double)numCols + 1.0);
    for ( size_t i = 0; i < numCols+1; i++ )
    {
      xScale[i] = xMin + (double)i * dx;
    }
  }

  // Choose spectra from required range of spectrum indexes
  double yStep = (yMax - yMin) / (double)numRows;
  double dYIndex;

  MantidVec yVals;
  MantidVec err;
  yVals.resize(numCols);
  err.resize(numCols);
  size_t index = 0;
  for ( size_t i = 0; i < numRows; i++ )
  {
    double midY = yMin + ((double)i + 0.5) * yStep;
    SVUtils::Interpolate( m_totalYMin, m_totalYMax,         midY,
                          0.0,         (double)m_totalRows, dYIndex );
    size_t sourceRow = (size_t)dYIndex;
    yVals.clear();
    err.clear();
    yVals.resize(numCols, 0);
    err.resize(numCols, 0);

    m_matWs->generateHistogram( sourceRow, xScale, yVals, err, true );
    for ( size_t col = 0; col < numCols; col++ )
    {
      newData[index] = (float)yVals[col];
      index++;
    }
  }

  // The calling code is responsible for deleting the DataArray when it is done with it
  DataArray_const_sptr newDataArray( new DataArray( xMin, xMax, yMin, yMax,
                                                    isLogX,
                                                    numRows, numCols,
                                                    newData) );

  return newDataArray;
}


/**
 * Get a data array covering the full range of data.
 *
 * @param isLogX  Flag indicating whether or not the data should be
 *                binned logarithmically.
 */
DataArray_const_sptr MatrixWSDataSource::getDataArray( bool isLogX )
{
  return getDataArray( m_totalXMin, m_totalXMax, m_totalYMin, m_totalYMax,
                       m_totalRows, m_totalCols, isLogX );
}


/**
 * Set the class that gets the emode & efixed info from the user.
 *
 * @param emodeHandler  Pointer to the user interface handler that
 *                      can provide user values for emode and efixed.
 */
void MatrixWSDataSource::setEModeHandler( EModeHandler* emodeHandler )
{
  m_emodeHandler = emodeHandler;
}


/**
 * Clear the vector of strings and then add pairs of strings giving information
 * about the specified point, x, y.  The first string in a pair should
 * generally be a string describing the value being presented and the second
 * string should contain the value.
 *
 * @param x    The x-coordinate of the point of interest in the data.
 * @param y    The y-coordinate of the point of interest in the data.
 * @param list Vector that will be filled out with the information strings.
 */
void MatrixWSDataSource::getInfoList( double x,
                                      double y,
                                      std::vector<std::string> &list )
{
  // First get the info that is always available for any matrix workspace
  list.clear();

  int row = (int)y;
  restrictRow( row );

  const ISpectrum* spec = m_matWs->getSpectrum( row );

  double spec_num = spec->getSpectrumNo();
  SVUtils::PushNameValue( "Spec Num", 8, 0, spec_num, list );

  std::string x_label = "";
  Unit_sptr& old_unit = m_matWs->getAxis(0)->unit();
  if ( old_unit != 0 )
  {
    x_label = old_unit->caption();
    SVUtils::PushNameValue( x_label, 8, 3, x, list );
  }

  std::set<detid_t> ids = spec->getDetectorIDs();
  if ( !ids.empty() )
  {
    list.push_back("Det ID");
    const int64_t id = static_cast<int64_t>(*(ids.begin()));
    list.push_back(boost::lexical_cast<std::string>(id));
  }

  /* Now try to do various unit conversions to get equivalent info */
  /* first make sure we can get the needed information */
  IDetector_const_sptr det;
  try
  {

    if ( old_unit == 0 )
    {
      g_log.debug("No UNITS on MatrixWorkspace X-axis");
      return;
    }

    Instrument_const_sptr instrument = m_matWs->getInstrument();
    if ( instrument == 0 )
    {
      g_log.debug("No INSTRUMENT on MatrixWorkspace");
      return;
    }

    IComponent_const_sptr source = instrument->getSource();
    if ( source == 0 )
    {
      g_log.debug("No SOURCE on instrument in MatrixWorkspace");
      return;
    }

    IComponent_const_sptr sample = instrument->getSample();
    if ( sample == 0 )
    {
      g_log.debug("No SAMPLE on instrument in MatrixWorkspace");
      return;
    }

    det = m_matWs->getDetector( row );
    if ( det == 0 )
    {
      g_log.debug() << "No DETECTOR for row " << row << " in MatrixWorkspace" << std::endl;
      return;
    }

    double l1        = source->getDistance(*sample);
    double l2        = 0.0;
    double two_theta = 0.0;
    double azi       = 0.0;
    if ( det->isMonitor() )
    {
      l2 = det->getDistance(*source);
      l2 = l2-l1;
    }
    else
    {
      l2 = det->getDistance(*sample);
      two_theta = m_matWs->detectorTwoTheta(det);
      azi = det->getPhi();
    }
    SVUtils::PushNameValue( "L2", 8, 4, l2, list );
    SVUtils::PushNameValue( "TwoTheta", 8, 2, two_theta*180./M_PI, list );
    SVUtils::PushNameValue( "Azimuthal", 8, 2, azi*180./M_PI, list );

    /* For now, only support diffractometers and monitors. */
    /* We need a portable way to determine emode and */
    /* and efixed that will work for any matrix workspace! */
    int    emode  = 0;
    double efixed = 0.0;
    double delta  = 0.0;

    // First try to get emode & efixed from the user
    if ( m_emodeHandler != NULL )
    {
      efixed = m_emodeHandler->getEFixed();
      if ( efixed != 0 )
      {
        emode = m_emodeHandler->getEMode();
        if ( emode == 0 )
        {
          g_log.information("EMode invalid, spectrometer needed if emode != 0");
          g_log.information("Assuming Direct Geometry Spectrometer....");
          emode = 1;
        }
      }
    }

    // Did NOT get emode & efixed from user, try getting direct geometry information from the run object
    if ( efixed == 0 )
    {
      const API::Run & run = m_matWs->run();
      if ( run.hasProperty("Ei") )
      {
        Kernel::Property* prop = run.getProperty("Ei");
        efixed = boost::lexical_cast<double,std::string>(prop->value());
        emode  = 1;                         // only correct if direct geometry
      }
      else if ( run.hasProperty("EnergyRequested") )
      {
        Kernel::Property* prop = run.getProperty("EnergyRequested");
        efixed = boost::lexical_cast<double,std::string>(prop->value());
        emode  = 1;
      }
      else if ( run.hasProperty("EnergyEstimate") )
      {
        Kernel::Property* prop = run.getProperty("EnergyEstimate");
        efixed = boost::lexical_cast<double,std::string>(prop->value());
        emode  = 1;
      }
    }

    // Finally, try getting indirect geometry information from the detector object
    if ( efixed == 0 )
    {
      if ( !(det->isMonitor() && det->hasParameter("Efixed")))
      {
        try
        {
          const ParameterMap& pmap = m_matWs->constInstrumentParameters();
          Parameter_sptr par = pmap.getRecursive(det.get(),"Efixed");
          if (par)
          {
            efixed = par->value<double>();
            emode = 2;
          }
        }
        catch ( std::runtime_error& )
        {
          g_log.debug() << "Failed to get Efixed from detector ID: "
                        << det->getID() << " in MatrixWSDataSource" << std::endl;
          efixed = 0;
        }
      }
    }

    if ( efixed == 0 )
      emode = 0;

    if ( m_emodeHandler != NULL )
    {
      m_emodeHandler -> setEFixed( efixed );
      m_emodeHandler -> setEMode ( emode );
    }

    double tof = old_unit->convertSingleToTOF( x, l1, l2, two_theta,
                                               emode, efixed, delta );
    if ( ! (x_label == "Time-of-flight") )
      SVUtils::PushNameValue( "Time-of-flight", 8, 1, tof, list );

    if ( ! (x_label == "Wavelength") )
    {
      const Unit_sptr& wl_unit = UnitFactory::Instance().create("Wavelength");
      double wavelength = wl_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                         emode, efixed, delta );
      SVUtils::PushNameValue( "Wavelength", 8, 4, wavelength, list );
    }

    if ( ! (x_label == "Energy") )
    {
      const Unit_sptr& e_unit = UnitFactory::Instance().create("Energy");
      double energy = e_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                    emode, efixed, delta );
      SVUtils::PushNameValue( "Energy", 8, 4, energy, list );
    }

    if ( (! (x_label == "d-Spacing")) && (two_theta != 0.0) && ( emode == 0 ) )
    {
      const Unit_sptr& d_unit = UnitFactory::Instance().create("dSpacing");
      double d_spacing = d_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                       emode, efixed, delta );
      SVUtils::PushNameValue( "d-Spacing", 8, 4, d_spacing, list );
    }

    if ( (! (x_label == "q")) && (two_theta != 0.0) )
    {
      const Unit_sptr& q_unit=UnitFactory::Instance().create("MomentumTransfer");
      double mag_q = q_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                   emode, efixed, delta );
      SVUtils::PushNameValue( "|Q|", 8, 4, mag_q, list );
    }

    if ( (! (x_label == "DeltaE")) && (two_theta != 0.0) && ( emode != 0 ) )
    {
      const Unit_sptr& deltaE_unit=UnitFactory::Instance().create("DeltaE");
      double delta_E = deltaE_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                          emode, efixed, delta );
      SVUtils::PushNameValue( "DeltaE", 8, 4, delta_E, list );
    }
  }
  catch (std::exception & e)
  {
    g_log.debug() << "Failed to get information from Workspace:" << e.what() << std::endl;
  }
}


} // namespace SpectrumView
} // namespace MantidQt
