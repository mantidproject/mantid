/**
 *  File: MatrixWSDataSource.cpp
 */

#include <iostream>
#include <sstream>
#include <math.h>

#include <QThread>

#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"
#include "MantidQtSpectrumViewer/IVUtils.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/Run.h"
#include "MantidQtSpectrumViewer/ErrorHandler.h"

using namespace Mantid;
using namespace Kernel;
using namespace API;
using namespace Geometry;

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct a DataSource object around the specifed MatrixWorkspace.
 *
 * @param mat_ws  Shared pointer to the matrix workspace being "wrapped"
 */
MatrixWSDataSource::MatrixWSDataSource( MatrixWorkspace_const_sptr mat_ws )
                 :ImageDataSource( 0.0, 1.0, 0.0, 1.0, 0, 0 )  // some defaults
{
  this->mat_ws = mat_ws;

  total_xmin = mat_ws->getXMin(); 
  total_xmax = mat_ws->getXMax(); 

  total_ymin = 0;                 // y direction is spectrum index
  total_ymax = (double)mat_ws->getNumberHistograms();

  total_rows = mat_ws->getNumberHistograms();

  total_cols = 1000000;              // Default data resolution

  saved_emode_handler = 0;
}


MatrixWSDataSource::~MatrixWSDataSource()
{
}


/**
 * Get the smallest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetXMin()
{
  total_xmin = mat_ws->getXMin(); 
  return total_xmin;
}


/**
 * Get the largest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetXMax()
{
  total_xmax = mat_ws->getXMax(); 
  return total_xmax;
}


/**
 * Get the largest 'y' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetYMax()
{
  total_ymax = (double)mat_ws->getNumberHistograms();
  return total_ymax;
}


/**
 * Get the total number of rows the data is divided into.  Must override base
 * class method, since the DataSource can be changed!
 */
size_t MatrixWSDataSource::GetNRows()
{
  total_ymax = (double)mat_ws->getNumberHistograms();
  return total_rows;
}


/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.  NOTE: The calling code is responsible for deleting the 
 * DataArray that is constructed in and returned by this method.
 *
 * @param xmin      Left edge of region to be covered.
 * @param xmax      Right edge of region to be covered.
 * @param ymin      Bottom edge of region to be covered.
 * @param ymax      Top edge of region to be covered.
 * @param n_rows    Number of rows to return. If the number of rows is less
 *                  than the actual number of data rows in [ymin,ymax], the 
 *                  data will be subsampled, and only the specified number 
 *                  of rows will be returned.
 * @param n_cols    The specrum data will be rebinned using the specified
 *                  number of colums.
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically. 
 */
DataArray* MatrixWSDataSource::GetDataArray( double xmin,   double  xmax,
                                             double ymin,   double  ymax,
                                             size_t n_rows, size_t  n_cols,
                                             bool   is_log_x )
{
/*
  std::cout << "Start MatrixWSDataSource::GetDataArray " << std::endl;
  std::cout << "  xmin   = " << xmin 
            << "  xmax   = " << xmax 
            << "  ymin   = " << ymin 
            << "  ymax   = " << ymax 
            << "  n_rows = " << n_rows
            << "  n_cols = " << n_cols << std::endl;
*/
                                                  // since we're rebinning, the
                                                  // columns can be arbitrary
                                                  // but rows must be aligned 
                                                  // to get whole spectra
  size_t first_row;
  IVUtils::CalculateInterval( total_ymin, total_ymax, total_rows,
                              first_row, ymin, ymax, n_rows );

  float* new_data = new float[n_rows * n_cols];   // this array is deleted in
                                                  // the DataArrray destructor
  MantidVec x_scale;
  x_scale.resize(n_cols+1);
  if ( is_log_x )
  {
    for ( size_t i = 0; i < n_cols+1; i++ )
    {
      x_scale[i] = xmin * exp ( (double)i / (double)n_cols * log(xmax/xmin) );  
    }
  }
  else
  {
    double dx = (xmax - xmin)/((double)n_cols + 1.0);
    for ( size_t i = 0; i < n_cols+1; i++ )
    {
      x_scale[i] = xmin + (double)i * dx;
    }
  }                                                // choose spectra from  
                                                   // required range of 
                                                   // spectrum indexes 
  double y_step = (ymax - ymin) / (double)n_rows;
  double mid_y;
  double d_y_index;
  size_t source_row;

  MantidVec y_vals;
  MantidVec err;
  y_vals.resize(n_cols);
  err.resize(n_cols);
  size_t index = 0;
  for ( size_t i = 0; i < n_rows; i++ )
  {
    mid_y = ymin + ((double)i + 0.5) * y_step;
    IVUtils::Interpolate( total_ymin, total_ymax, mid_y,
                                 0.0, (double)total_rows, d_y_index );
    source_row = (size_t)d_y_index;
    y_vals.clear();
    err.clear();
    y_vals.resize(n_cols,0);
    err.resize(n_cols,0);

    mat_ws->generateHistogram( source_row, x_scale, y_vals, err, true );
    for ( size_t col = 0; col < n_cols; col++ )
    {
      new_data[index] = (float)y_vals[col];
      index++;
    }
  }
                                // The calling code is responsible for deleting 
                                // the DataArray when it is done with it      
  DataArray* new_data_array = new DataArray( xmin, xmax, ymin, ymax,
                                             is_log_x, 
                                             n_rows, n_cols, new_data);
  return new_data_array;
}


/**
 * Get a data array covering the full range of data.
 *
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.
 */
DataArray * MatrixWSDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}


/**
 * Set the class that gets the emode & efixed info from the user.
 *
 * @param emode_handler  Pointer to the user interface handler that
 *                       can provide user values for emode and efixed.
 */
void MatrixWSDataSource::SetEModeHandler( EModeHandler* emode_handler )
{
  saved_emode_handler = emode_handler;
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
void MatrixWSDataSource::GetInfoList( double x, 
                                      double y,
                                      std::vector<std::string> &list )
{
  list.clear();
                                        // first get the info that is always
                                        // available for any matrix workspace
  int row = (int)y;
  RestrictRow( row );

  const ISpectrum* spec = mat_ws->getSpectrum( row );

  double spec_num = spec->getSpectrumNo();
  IVUtils::PushNameValue( "Spec Num", 8, 0, spec_num, list );

  std::string x_label = "";
  Unit_sptr& old_unit = mat_ws->getAxis(0)->unit();
  if ( old_unit != 0 )
  {
    x_label = old_unit->caption();
    IVUtils::PushNameValue( x_label, 8, 3, x, list );
  }

  double d_id = 0;
  std::set<detid_t> ids = spec->getDetectorIDs();
  if ( !ids.empty() )
  {
    std::set<detid_t>::iterator it = ids.begin();
    d_id = (double)*it;
    IVUtils::PushNameValue( "Det ID", 8, 0, d_id, list );
  }

  IDetector_const_sptr det;          // now try to do various unit conversions
  try                                // to get equivalent info
  {                                  // first make sure we can get the needed
                                     // information
    if ( old_unit == 0 )
    {
      ErrorHandler::Error("No UNITS on MatrixWorkspace X-axis");
      return;
    }

    Instrument_const_sptr instrument = mat_ws->getInstrument();
    if ( instrument == 0 )
    {
      ErrorHandler::Error("No INSTRUMENT on MatrixWorkspace");
      return;
    }

    IObjComponent_const_sptr source = instrument->getSource();
    if ( source == 0 )
    {
      ErrorHandler::Error("No SOURCE on instrument in MatrixWorkspace");
      return;
    }

    IObjComponent_const_sptr sample = instrument->getSample();
    if ( sample == 0 )
    {
      ErrorHandler::Error("No SAMPLE on instrument in MatrixWorkspace");
      return;
    }

    det = mat_ws->getDetector( row );
    if ( det == 0 )
    {
      std::ostringstream message;
      message << "No DETECTOR for row " << row << " in MatrixWorkspace";
      ErrorHandler::Error( message.str() );
      return;
    }

    double l1        = source->getDistance(*sample);
    double l2        = 0;
    double two_theta = 0;
    if ( det->isMonitor() )
    {
      l2 = det->getDistance(*source);
      l2 = l2-l1;
    }
    else
    {
      l2 = det->getDistance(*sample);
      two_theta = mat_ws->detectorTwoTheta(det);
    }
                        // For now, only support diffractometers and monitors.
                        // We need a portable way to determine emode and
                        // and efixed that will work for any matrix workspace!
    int    emode  = 0;
    double efixed = 0;
    double delta  = 0;

//  std::cout << "Start of checks for emode" << std::endl;

                        // First try to get emode & efixed from the user
    if ( saved_emode_handler != 0 )
    {
      efixed = saved_emode_handler->GetEFixed();
      if ( efixed != 0 )
      {
        emode = saved_emode_handler->GetEMode();
        if ( emode == 0 )
        {
          ErrorHandler::Error("EMode invalid, spectrometer needed if emode != 0");
          ErrorHandler::Error("Assuming Direct Geometry Spectrometer....");
          emode = 1;
        }
      }
    }

//  std::cout << "Done with calls to GetEFixed and GetEMode" << std::endl;
//  std::cout << "EMode  = " << emode  << std::endl;
//  std::cout << "EFixed = " << efixed << std::endl;

    if ( efixed == 0 )    // Did NOT get emode & efixed from user, try getting 
    {                     // direct geometry information from the run object
      const API::Run & run = mat_ws->run(); 
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

//  std::cout << "Done with getting info from run" << std::endl;
//  std::cout << "EMode  = " << emode  << std::endl;
//  std::cout << "EFixed = " << efixed << std::endl;

    if ( efixed == 0 )    // finally, try getting indirect geometry information 
    {                     // from the detector object
      if ( !(det->isMonitor() && det->hasParameter("Efixed")))
      {
        try
        {
          const ParameterMap& pmap = mat_ws->constInstrumentParameters();
          Parameter_sptr par = pmap.getRecursive(det.get(),"Efixed");
          if (par)
          {
            efixed = par->value<double>();
            emode = 2;
          }
        }
        catch ( std::runtime_error& )
        {
          std::ostringstream message;
          message << "Failed to get Efixed from detector ID: " 
                  << det->getID() << " in MatrixWSDataSource";
          ErrorHandler::Error( message.str() );
          efixed = 0;
        }
      } 
    }

//  std::cout << "Done with getting info from detector" << std::endl;
//  std::cout << "EMode  = " << emode  << std::endl;
//  std::cout << "EFixed = " << efixed << std::endl;
 
    if ( efixed == 0 )
    {
      emode = 0;
    }
    
    if ( saved_emode_handler != 0 )
    {
      saved_emode_handler -> SetEFixed( efixed );  
      saved_emode_handler -> SetEMode ( emode );  
    }

    double tof = old_unit->convertSingleToTOF( x, l1, l2, two_theta, 
                                               emode, efixed, delta );
    if ( ! (x_label == "Time-of-flight") )
    {
      IVUtils::PushNameValue( "Time-of-flight", 8, 1, tof, list );
    }

    if ( ! (x_label == "Wavelength") )
    {
      const Unit_sptr& wl_unit = UnitFactory::Instance().create("Wavelength");
      double wavelength = wl_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                         emode, efixed, delta );
      IVUtils::PushNameValue( "Wavelength", 8, 4, wavelength, list );
    }

    if ( ! (x_label == "Energy") )
    {
      const Unit_sptr& e_unit = UnitFactory::Instance().create("Energy");
      double energy = e_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                    emode, efixed, delta );
      IVUtils::PushNameValue( "Energy", 8, 4, energy, list );
    }

    if ( (! (x_label == "d-Spacing")) && (two_theta != 0.0) && ( emode == 0 ) )
    {
      const Unit_sptr& d_unit = UnitFactory::Instance().create("dSpacing");
      double d_spacing = d_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                       emode, efixed, delta );
      IVUtils::PushNameValue( "d-Spacing", 8, 4, d_spacing, list );
    }

    if ( (! (x_label == "q")) && (two_theta != 0.0) )
    {
      const Unit_sptr& q_unit=UnitFactory::Instance().create("MomentumTransfer");
      double mag_q = q_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                   emode, efixed, delta );
      IVUtils::PushNameValue( "|Q|", 8, 4, mag_q, list );
    }

    if ( (! (x_label == "DeltaE")) && (two_theta != 0.0) && ( emode != 0 ) )
    {
      const Unit_sptr& deltaE_unit=UnitFactory::Instance().create("DeltaE");
      double delta_E = deltaE_unit->convertSingleFromTOF( tof, l1, l2, two_theta,
                                                          emode, efixed, delta );
      IVUtils::PushNameValue( "DeltaE", 8, 4, delta_E, list );
    }
  }
  catch (std::exception & e)
  {
    ErrorHandler::Notice("Failed to get information from Workspace:");
    ErrorHandler::Notice( e.what() );
  }
}


} // namespace SpectrumView
} // namespace MantidQt 
