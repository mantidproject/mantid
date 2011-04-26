#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Quat.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace DataObjects
{
  /// Register the workspace as a type
  DECLARE_WORKSPACE(PeaksWorkspace );

//  Kernel::Logger& PeaksWorkspace::g_log = Kernel::Logger::get("PeaksWorkspace");


  //---------------------------------------------------------------------------------------------
  /** Constructor. Create a table with all the required columns.
   *
   * @return PeaksWorkspace object
   */
  PeaksWorkspace::PeaksWorkspace():ITableWorkspace( )
  {
    // Note: These column names must match what PeakColumn expects!
    addColumn( "int", "RunNumber");
    addColumn( "int", "DetID");
    addColumn( "double", "h");
    addColumn( "double", "k");
    addColumn( "double", "l");
    addColumn( "double", "Wavelength");
    addColumn( "double", "Energy");
    addColumn( "double", "TOF");
    addColumn( "double", "DSpacing");
    addColumn( "double", "Intens");
    addColumn( "double", "SigInt");
    addColumn( "double", "BinCount");
    addColumn( "double", "BankName");
    addColumn( "double", "Row");
    addColumn( "double", "Col");
    addColumn( "V3D", "QLab");
    addColumn( "V3D", "QSample");
  }



  //---------------------------------------------------------------------------------------------
  /** Destructor */
  PeaksWorkspace::~PeaksWorkspace()
  {
//    ClearDeleteCalibrationData();
  }

  //---------------------------------------------------------------------------------------------

  //---------------------------------------------------------------------------------------------
  /** Add a column (used by constructor). */
  bool PeaksWorkspace::addColumn(const std::string& type, const std::string& name)
  {
    // Create the PeakColumn.
    columns.push_back( boost::shared_ptr<Mantid::DataObjects::PeakColumn>(new Mantid::DataObjects::PeakColumn( this->peaks, name) ) );
    columnNames.push_back(name);
    columnTypes.push_back(type);

    return true;
  }

//
//  //---------------------------------------------------------------------------------------------
//  /** Rebuild all the columns in this PeaksWorkspace. */
//  void PeaksWorkspace::buildColumns()
//  {
//    columns.clear();
//    for (size_t index=0; index< static_cast<int>(numColumns); index++)
//    {
//      std::string name = columnNames[index];
//      std::string type = columnTypes[index];
//
//      boost::shared_ptr<Mantid::API::Column> out;
//
//      if (type == "V3D")
//      {
//        boost::shared_ptr<TableColumn<V3D> > temp(new TableColumn<V3D>());
//        out = boost::dynamic_pointer_cast<Column>(temp);
//        for (size_t i=0; i<peaks.size(); i++)
//        {
//          switch (index)
//          {
//          case IhklCol: /** Column number where V3D hkl value is stored*/
//            temp->data().push_back( peaks[i].getHKL() );
//            break;
//          case IpositionCol: /** Column number where V3D xyz position is stored*/
//            temp->data().push_back( V3D() );
//            break;
//          case IsamplePositionCol: /**Column where V3d sample orientation is stored*/
//            temp->data().push_back( V3D() );
//            break;
//          }
//        }
//      }
//
//
//      else if (type == "int")
//      {
//        boost::shared_ptr<TableColumn<int> > temp(new TableColumn<int>());
//        out = boost::dynamic_pointer_cast<Column>(temp);
//        for (size_t i=0; i<peaks.size(); i++)
//        {
//          switch (index)
//          {
//          case IreflagCol:  /** Column where the reflag is stored */
//            temp->data().push_back( 0 );
//            break;
//          case IDetBankCol:  /** Column where the Bank number is store */
//            temp->data().push_back( 0 );
//            break;
//          case IrunNumCol:  /** Column where the run number is stored */
//            temp->data().push_back( peaks[i].getRunNumber() );
//            break;
//          }
//        }
//      }
//
//      else if (type == "double")
//      {
//        boost::shared_ptr<TableColumn<double> > temp(new TableColumn<double>());
//        out = boost::dynamic_pointer_cast<Column>(temp);
//        for (size_t i=0; i<peaks.size(); i++)
//        {
//          switch (index)
//          {
//          case IPeakIntensityCol:  /** Column where intensity of middle cell is stored */
//            temp->data().push_back( 0 );
//            break;
//          case IPeakIntegrateCol: /** Column where intensity-backgound of all cells in peak is stored */
//            temp->data().push_back( peaks[i].getIntensity() );
//            break;
//          case IPeakIntegrateErrorCol:  /** Column where the error in the integrated intensity is stored */
//            temp->data().push_back( peaks[i].getSigmaIntensity() );
//            break;
//          case IMonitorCountCol:     /** Column where the monitor count is stored */
//            temp->data().push_back( 0);
//            break;
//          case IPeakRowCol:       /** Column where the row of the peak is stored */
//            temp->data().push_back( peaks[i].getRow() );
//            break;
//          case IPeakColCol:      /** Column where the column of the peak is stored */
//            temp->data().push_back( peaks[i].getCol() );
//            break;
//          case IPeakChanCol:       /** Column where the time channel of the peak is stored */
//            temp->data().push_back( 0 );
//            break;
//          case IL1Col:               /** Column where the initial path of the peak is stored */
//            temp->data().push_back( 0 );
//            break;
//          case ItimeCol:             /** Column where the time of the peak is stored */
//            temp->data().push_back( peaks[i].getTOF() );
//            break;
//          case ItimeOffsetChanCol:   /** Column where the time offset of the peak is stored */
//            temp->data().push_back( 0 );
//            break;
//          }
//        }
//      }
//
//      else
//      {
//        throw std::invalid_argument("Unexpected column type");
//      }
//      out->setName(name);
//      columns.push_back(out);
//    }
//  }

  //---------------------------------------------------------------------------------------------
  /// @return the index of the column with the given name.
  int PeaksWorkspace::getColumnIndex(const std::string& name)
  {
    for (int i=0; i < int(columnNames.size()); i++)
      if (columnNames[i] == name)
        return i;
    throw std::invalid_argument("Column named " + name + " was not found in the PeaksWorkspace.");
  }

  //---------------------------------------------------------------------------------------------
  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<Mantid::API::Column> PeaksWorkspace::getColumn(int index)
  {
    if (index < 0 || index >= numColumns) throw std::invalid_argument("PeaksWorkspace::getColumn() called with invalid index.");
    return columns[index];
  }

//
//
//
//
//  /** Get a word from a line and strips spaces */
//  std::string getWord( std::ifstream &in ,  bool consumeEOL )
//  {
//    std::string s;
//    char c = 0;
//    if( in.good() )
//      for(  c = in.get() ; c == ' ' && in.good() ; c = in.get() )
//      {}
//    else
//      return std::string();
//
//    if( c == '\n' )
//    {
//      if( !consumeEOL )
//        in.putback( c );
//
//      return std::string();
//    }
//
//    s.push_back( c );
//
//    if( in.good() )
//      for(  c = in.get() ; in.good()&&c != ' ' && c != '\n' && c != '\r' ; c = in.get() )
//        s.push_back( c );
//
//    if( ((c == '\n') || (c == '\r')) && !consumeEOL )
//      in.putback( c );
//
//    return s;
//  }
//
//  /** Read up to the eol */
//  void readToEndOfLine( std::ifstream& in ,  bool ConsumeEOL )
//  {
//    while( in.good() && getWord( in ,  false ).length() > 0  )
//      getWord( in ,  false );
//    if( !ConsumeEOL )
//      return ;
//    getWord( in ,  true );
//  }
//
//
//  /** Reads the header of a .peaks file
//   *
//   * @param in :: stream of the input file
//   * @return TODO: I don't know what here
//   */
//  std::string PeaksWorkspace::readHeader( std::ifstream& in )
//  {
//
//    std::string r = getWord( in ,  false );
//
//    if( r.length() < 1 )
//      throw std::logic_error( std::string( "No first line of Peaks file" ) );
//
//    if( r.compare( std::string( "Version:" ) ) != 0 )
//      throw std::logic_error(
//          std::string( "No Version: on first line of Peaks file" ) );
//
//    C_version = getWord( in ,  false );
//
//    if( C_version.length() < 1 )
//      throw  std::logic_error( std::string( "No Version for Peaks file" ) );
//
//    C_Facility = getWord( in ,  false );
//
//    if( C_Facility.length() < 1  )
//      throw  std::logic_error(
//          std::string( "No Facility tag for Peaks file" ) );
//
//    if( C_Facility.compare( std::string( "Facility:" ) ) != 0 )
//      throw  std::logic_error(
//          std::string( "No Facility tag for Peaks file" ) );
//
//    C_Facility = getWord( in ,  false );
//
//    if( C_Facility.length() < 1 )
//      throw  std::logic_error( std::string( "No Facility in Peaks file" ) );
//
//    C_Instrument = getWord( in ,  false );
//
//    if( C_Instrument.length() < 1 )
//      throw  std::logic_error(
//          std::string( "No Instrument tag for Peaks file" ) );
//
//    if( C_Instrument.compare( std::string( "Instrument:" ) ) != 0 )
//      throw  std::logic_error(
//          std::string( "No Instrument tag for Peaks file" ) );
//
//    C_Instrument = getWord( in ,  false );
//
//    if( C_Instrument.length() < 1 )
//      throw std::logic_error(
//          std::string( "No Instrument for Peaks file" ) );
//
//    std::string date = getWord( in ,  false );
//
//    if(date .length()< 1)
//    {
//      Kernel::DateAndTime x = Kernel::DateAndTime::get_current_time();
//      C_experimentDate.set_from_time_t(x.to_time_t());
//    }
//
//    else
//      if( date.compare( std::string( "Date:" ) ) == 0 )
//      {
//        date = getWord( in ,  false );
//        C_experimentDate = Kernel::DateAndTime( date );
//      }
//
//
//    while( getWord( in ,  false ).length() > 0 )
//      getWord( in ,  false );
//
//    // Now get info on detector banks
//
//    getWord( in ,  true );
//    std::string s = getWord( in ,  false );
//    if( s.compare( "6" ) != 0 )
//      throw std::logic_error(
//          std::string( "Peaks File has no L0 and T0 header info" ) );
//
//    readToEndOfLine( in ,  true );
//    s = getWord( in ,  false );
//
//    if( s.compare( "7" ) != 0 )
//      throw std::logic_error(
//          std::string( "Peaks File has no L0 and T0 info" ) );
//
//    C_L1 = strtod( getWord( in ,  false ).c_str() ,  0 )/100;
//    C_time_offset = strtod( getWord( in ,  false ).c_str() ,  0 );
//
//
//    readToEndOfLine( in ,  true );
//    for( s = getWord( in ,  false ) ; s.length() < 1 ; getWord( in ,  true ) )
//    {
//      s = getWord( in ,  false );
//    }
//
//    if( s.compare( "4" ) != 0 )
//      throw std::logic_error( std::string( "Peaks File has no bank descriptor line" ) );
//
//    readToEndOfLine( in ,  true );
//    s = getWord( in ,  false );
//    while( s.length() < 1 && in.good() )
//    {
//      s = getWord( in ,  true );
//      s = getWord( in ,  false );
//    }
//
//    if(  !in.good() )
//      throw std::runtime_error( std::string( "Peaks File Quit too fast" ) );
//
//    if( s.compare( std::string( "5" ) ) != 0 )
//      throw std::logic_error( "No information on banks given" );//log this only
//
//
//    bool err = false;
//    while( s.compare( std::string( "5" ) ) == 0  )
//    {
//      double* data = new double[ 15 ];//Use boost pointers here??? does gc
//      s = getWord( in ,  false );
//
//      if( s.length() < 1 )
//        err = true;
//      else
//        DetNames.push_back( s );
//
//      for( int i = 0 ; i < 15 && !err ; i++ )
//      {
//        s = getWord( in ,  false );
//        if( s.length() < 1 )
//          err = true;
//        else
//          data[ i ] = strtod( s.c_str() ,  0 );
//
//      }
//
//      if( err )
//        s = std::string( "2" );
//      else
//      {
//        DetInfo.push_back( data );
//
//        for( s = getWord( in ,  false ) ; s.length() > 0 ; s = getWord( in ,  false ) )
//        {}
//
//        s = getWord( in ,  true );
//        s = getWord( in ,  false );//no blank lines will be allowed so far
//      }
//    }
//
//
//    if( err )
//      return std::string();
//
//    return s;
//  }
//
//
//
//  std::string readPeak( std::string lastStr ,  std::ifstream& in ,
//      double& h , double& k ,  double& l ,  double& col ,
//      double& row , double& chan ,  double& L2 ,
//      double&  ScatAng , double& Az ,  double& wl ,
//      double& D ,  double& IPK , double& Inti ,
//      double& SigI ,  int &iReflag )
//  {
//
//    std::string s = lastStr;
//
//    if( s.length() < 1 && in.good() )//blank line
//    {
//      readToEndOfLine( in ,  true );
//
//      s = getWord( in ,  false );;
//    }
//
//    if( s.length() < 1 )
//      return 0;
//
//
//    if( s.compare( "2" ) == 0 )
//    {
//      readToEndOfLine( in ,  true );
//
//      for( s = getWord( in ,  false ) ; s.length() < 1 && in.good() ;
//          s = getWord( in ,  true ) )
//      {
//        s = getWord( in ,  false );
//      }
//    }
//
//    if( s.length() < 1 )
//      return 0;
//
//    if( s.compare( "3" ) != 0 )
//      return s;
//
//    int seqNum = atoi( getWord( in ,  false ).c_str() );
//
//    h = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//    k = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//    l = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//
//    col = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//    row = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//    chan = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//    L2 = strtod( getWord( in ,  false ).c_str() ,  0 )/100 ;
//    ScatAng = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
//
//    Az = strtod( getWord( in , false ).c_str() , 0 ) ;
//    wl = strtod( getWord( in , false ).c_str() , 0 ) ;
//    D = strtod( getWord( in , false ).c_str() , 0 ) ;
//    IPK = strtod( getWord( in , false ).c_str() , 0 ) ;
//
//    Inti = strtod( getWord( in , false ).c_str() , 0 ) ;
//    SigI = strtod( getWord( in , false ).c_str() , 0 ) ;
//    iReflag = atoi( getWord( in , false ).c_str() ) ;
//
//
//    readToEndOfLine( in ,  true );
//    return getWord( in , false );
//  }
//
//
//  std::string readPeakBlockHeader( std::string lastStr ,  std::ifstream &in ,
//      int&run ,int& detName ,
//      double&chi , double&phi ,
//      double&omega , double&monCount )
//  {
//    std::string s = lastStr;
//
//    if( s.length() < 1 && in.good() )//blank line
//    {
//      readToEndOfLine( in ,  true );
//      s = getWord( in , false );
//    }
//
//    if( s.length() < 1 )
//      return std::string();
//
//    if( s.compare( "0" ) == 0 )
//    {
//      readToEndOfLine( in ,  true );
//      s = getWord( in , false );
//      while( s.length() < 1 )
//      {
//        readToEndOfLine( in ,  true );
//        s = getWord( in , false );
//      }
//    }
//
//    if( s.compare( std::string( "1" ) ) != 0 )
//      return s;
//
//    run = atoi( getWord( in , false ).c_str() );
//    detName =atoi( getWord( in , false ).c_str());
//    chi = strtod( getWord( in , false ).c_str() , 0 );
//    phi = strtod( getWord( in , false ).c_str() , 0 );
//
//    omega = strtod( getWord( in , false ).c_str() , 0 );
//    monCount = strtod( getWord( in , false ).c_str() , 0 );
//    readToEndOfLine( in ,  true );
//
//    return getWord( in , false );
//
//  }
//  //special formatted file Use clear peaks to no append
//
////
//  /** Append the peaks from a .peaks file into the workspace
//   *
//   * @param filename :: path to the .peaks file
//   */
//  void PeaksWorkspace::appendFile( std::string filename, IInstrument_sptr inst )
//  {
//    try
//    {
//      std::ifstream in( filename.c_str() );
//
//      std::string s = readHeader( in );
//
//      if( !in.good() || s.length() < 1 )
//        throw std::runtime_error( "End of Peaks file before peaks" );
//
//      if( s.compare( std::string( "0" ) ) != 0 )
//        throw std::logic_error( "No header for Peak segments"  );
//
//      readToEndOfLine( in ,  true );
//      s = getWord( in , false );
//
//      int run , Reflag, detName;
//      double chi , phi , omega , monCount;
//
//      double h , k , l , col , row , chan , L2 , ScatAng , Az , wl , D ,
//      IPK , Inti , SigI;
//      double x , y , z , r , time;
//
//      while( in.good() )
//      {
//        s = readPeakBlockHeader( s ,  in  , run , detName , chi , phi ,
//            omega , monCount );
//
//        s = readPeak( s , in , h , k , l , col , row , chan , L2 ,
//            ScatAng , Az , wl , D , IPK , Inti , SigI , Reflag );
//
//        chi *= M_PI/180;
//        phi *= M_PI/180;
//        omega *= M_PI/180;
//
//        z = L2*cos( ScatAng );
//        r = sqrt( L2*L2 - z*z );
//        x = r*cos( Az );
//        y = r*sin( Az );
//        //Would use the V3D spherical stuff, but it seemed weird
//        std::vector< double >xx , yy;
//        xx.push_back( wl );
//
//        Kernel::Units::Wavelength WL;
//        WL.toTOF( xx , yy , C_L1 , L2 , ScatAng , 12 , 12.1 , 12.1 );
//        time = xx[ 0 ];
//
//        // Build the peak object
//        IDetector_sptr det;
//
//        Peak peak(inst, det->getID(), 1.0);
//
//        // Add it to workspace
//        this->addPeak(peak);
////
////        this->addPeak( Geometry::V3D( x , y , z ) , time ,
////            Geometry::V3D( h , k , l ) ,
////            Geometry::V3D( phi , chi , omega ) ,
////            Reflag , run , monCount , detName , IPK ,
////            row , col , chan ,  Inti , SigI );
//      }
//    }catch( std::exception & xe)
//    {
//      g_log.error( xe.what());
//      throw (std::logic_error(xe.what()));
//    }catch( char* str)
//    {
//      g_log.error( std::string(str));
//      throw (std::logic_error(std::string(str)));
//    }catch(...)
//    {
//      g_log.error( "Unknown Error");
//      throw ("Unknown Error");
//    }
//  }
//
//

//
//  void PeaksWorkspace:: ClearDeleteCalibrationData()
//  {
//    DetNames.clear();// elements std::string so will delete self??
//
//    for( size_t n=DetInfo.size(); n >0;)
//    {
//      double* val = DetInfo[n-1];
//      DetInfo.pop_back();
//      delete val;
//
//      n=DetInfo.size();
//    }
//  }
//
//
//  void PeaksWorkspace::initialize(    const double      L1 ,               //m
//                                      const double      time_offset ,      //microseconds
//                                      const std::string Facility ,
//                                      const std::string Instrument ,
//                                      const Kernel::DateAndTime  experimentDate ,
//                                      const std::string version ,
//
//                                      const std::vector<std::string> &PanelNames,
//                                      const std::vector< double*> &PanelInfo
//                                      )
//     {
//       C_L1 = L1;
//       C_time_offset = time_offset;
//       C_Facility = std::string( Facility);
//       C_Instrument = std::string( Instrument);
//       C_version = std::string( version );
//       C_experimentDate = Kernel::DateAndTime( experimentDate);
//       ClearDeleteCalibrationData();
//       for( size_t i=0; i<PanelNames.size() ; i++)
//         DetNames.push_back( std::string(PanelNames[i]));
//
//       for( size_t i=0; i<PanelInfo.size(); i++)
//       {
//         double* data = PanelInfo[i];
//         double* data1 = new double[15];
//         for( int j=0; j<15;j++)
//         {
//           data1[j] = data[j];

  //           if( (j>=2) &&( j <=8))
//             data1[j] *=100;
//         }
//         DetInfo.push_back(data1);
//       }
//     }
//
//     void PeaksWorkspace::initialize(  const std::string DetCalFileName)
//     {
//       ClearDeleteCalibrationData();
//
//       try
//       {
//         std::ifstream in( DetCalFileName.c_str() );
//
//         std::string s = readHeader( in );
//         in.close();
//       }catch( char * str)
//       {
//         std::cout <<"Exception reading detector info "<<str <<std::endl;
//       }
//
//     }
//
//  void PeaksWorkspace::addPeak( const Geometry::V3D position ,
//                                const double time ,
//                                const Geometry::V3D hkl ,
//                                const Geometry::V3D sample_orientation ,  //radians ,  phi,chi,omega
//                                const int  reflag ,
//                                const int  runNum ,
//                                const double monCount ,
//                                const int bankName ,
//                                const double PeakCellCount ,
//                                const double row ,
//                                const double col ,
//                                const double chan ,
//                                const double PeakIntegrateCount ,
//                                const double PeakIntegrateError
//         )
//  {
//    int i = rowCount();
//    insertRow( i );
//    //double T = time;
//
//    try
//    {
//
//      getRef< double >( std::string( "ipeak" ) ,  i ) = PeakCellCount;
//      getRef< double >( std::string( "inti" ) ,  i ) = PeakIntegrateCount;
//      getRef< double >( std::string( "sigi" ) ,  i ) = PeakIntegrateError;
//      getRef< double >( std::string( "moncount" ) ,  i ) = monCount;
//
//      getRef< Geometry::V3D >( std::string( "position" ) ,  i ) =
//          Geometry::V3D( position );
//
//      getRef< Geometry::V3D >( std::string( "sample_orientation" ) ,  i ) =
//          Geometry::V3D( sample_orientation );
//
//      getRef< Geometry::V3D >( std::string( "hkl" ) ,  i ) =
//          Geometry::V3D( hkl );
//
//      getRef< double >( std::string( "L1" ) ,  i ) = C_L1;
//      getRef< double >( std::string( "time" ) ,  i ) = time;
//      getRef< double >( std::string( "t_offset" ) ,  i ) = C_time_offset;
//      getRef< int >( std::string( "run" ) ,  i ) = runNum;
//
//      getRef< double >( std::string( "chan" ) ,  i ) = chan;
//      //getRef< double >( std::string( "L2" ) ,  i ) = L2;
//      getRef< int >( std::string( "bank" ) ,  i ) =
//          ( bankName );
//
//      getRef< double >( std::string( "row" ) ,  i ) = row;
//      getRef< double >( std::string( "col" ) ,  i ) = col;
//      getRef< int >( std::string( "reflag" ) ,  i ) = reflag;
//
//    }catch( char * str )
//    {
//      std::cout <<  "exception occurred " <<  str <<  "\n";
//    }
//
//  }
//
//
//  /**
//   * @return the number of peaks
//   */
//  int PeaksWorkspace::getNumberPeaks() const
//  {
//    return rowCount();
//  }
//
//  /** Remove a peak
//   *
//   * @param peakNum :: peak index to remove
//   */
//  void PeaksWorkspace::removePeak( const int peakNum )
//  {
//    removeRow( peakNum );
//  }
//
//
//  //returns true if strictly less than
//  bool compareAsVecs( std::vector< int > arg1, std::vector< int > arg2 )
//  {
//    if( arg1.size() < 3 )
//    {
//      if( arg2.size() < 3 )
//        return true;
//      else
//        return false;
//    }
//
//    if( arg2.size() < 3 )
//      return false;   //hopefully bad values go to back
//
//    int r1 = arg1[0];
//    int r2 = arg2[0];
//
//    if( r1 < r2 )
//      return true;
//
//    if( r1 > r2 )
//      return false;
//
//    r1 = arg1[1];
//    r2 = arg2[1];
//
//    if( r1 < r2 )
//      return true;
//
//    return false;
//  }
//
//
//
//  /** Write an ISAW-style .peaks file
//   *
//   * @param filename :: filename to write to
//   */
//  void PeaksWorkspace::write( std::string filename )
//  {
//    try
//    {
//      std::ofstream out( filename.c_str() );
//      std::string date =C_experimentDate.to_ISO8601_string();
//
//      out <<  "Version: " <<  C_version <<  " Facility: " <<  C_Facility  ;
//      out <<  " Instrument: " <<  C_Instrument <<  " Date: " ;
//      out<< std::setw(date.length())<<date<< std::endl;
//
//      out <<  "6        L1    T0_SHIFT" <<  std::endl;
//      out << "7 "<< std::setw( 11 )  ;
//      out <<   std::setprecision( 4 ) <<  std::fixed <<  ( C_L1*100 ) ;
//      out << std::setw( 12 ) <<  std::setprecision( 3 ) <<  std::fixed  ;
//      out << ( C_time_offset ) <<  std::endl;
//
//      out <<  "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT     DEPTH   DETD  "
//          <<  " CenterX   CenterY   CenterZ   BaseX    BaseY    BaseZ    "
//          <<  "  UpX      UpY      UpZ" <<  std::endl;
//
//      for(size_t i = 0 ; i < DetNames.size() ; i++ )
//      {
//        double* info = DetInfo[ i ];
//        out <<  "5" <<  std::setw( 7 ) <<  DetNames[ i ];
//
//        out <<  std::setw( 7 ) <<  (int)info[ 0 ] <<  std::setw( 7 );
//        out  <<  (int)info[ 1 ]<<  std::setw( 9 ) <<  std::setprecision( 4 );
//        out  <<  std::fixed <<  info[ 2 ];
//
//        for( int j = 3 ; j < 15 ; j++ )
//        {
//          out <<  std::setw( 9 ) <<  std::setprecision( 4 ) <<  std::fixed;
//          out <<  info[ j ];
//
//        }
//        out  <<  std::endl;
//
//      }
//
//
//      std::vector< std::vector< int > > SortKeys;
//
//      for( int i = 0 ; i < rowCount() ; i++ )
//      {
//        std::vector<int > dat ;
//
//        dat.push_back(getRef< int >( "run" ,  i ) );
//        dat.push_back( getRef< int >( "bank" ,  i ) );
//
//        dat.push_back( i );
//        SortKeys.push_back( dat );
//      }
//
//      stable_sort( SortKeys.begin() ,  SortKeys.end() ,  compareAsVecs );
//
//      int LastRun = -1;;
//      int LastDet;
//
//      int thisRun ,  thisReflag;
//      double this_h ,  this_k ,  this_l ,   this_row ,  this_col ,
//      this_chan ,  this_L2 ,  this_az ,  this_polar ,  this_time ,
//      this_inti ,  this_ipk ,   this_sigi ,  this_chi ,  this_phi ,
//      this_omega ,   this_monCt ,  this_tOffset ,  this_L1;
//
//      int thisDet;
//      Geometry::V3D thisSampleOrientation;
//      int seqNum = 0;
//      std::vector< std::vector< int > >::iterator it;
//
//      for ( it = SortKeys.begin(); it != SortKeys.end(); ++it )
//      {
//        seqNum++;
//        std::vector<int > elt = *it;
//        int peakNum =elt[2];
//
//        this_row =
//            getRef< double  >( std::string( "row" ) ,  peakNum );
//        Geometry::V3D hkl = getRef< Geometry::V3D  > ( std::string( "hkl" ) ,
//            peakNum );
//        this_h = hkl.X();
//        this_k = hkl.Y();
//        this_l = hkl.Z();
//        this_col = getRef < double >( std::string( "col" ) ,  peakNum );
//        this_chan = getRef< double >( std::string( "chan" ) ,  peakNum );
//        this_ipk = getRef< double >( "ipeak" ,  peakNum );
//        this_inti = getRef< double >( "inti" ,  peakNum );
//        this_sigi = getRef< double >( "sigi" ,  peakNum );
//
//        this_monCt = getRef< double >( std::string( "moncount" ) ,  peakNum );
//        this_L1 = getRef< double >( "L1" ,  peakNum );
//        this_tOffset = getRef< double >( "t_offset" ,  peakNum );
//        this_time = getRef< double >( "time" ,  peakNum );
//        thisRun = getRef< int >( "run" ,  peakNum );
//        thisReflag = Int( peakNum ,  3 );
//        thisDet = getRef< int >( "bank" ,  peakNum );
//
//        Geometry::V3D position = getRef< Geometry::V3D >( "position" ,
//            peakNum );
//
//        double x = position.X();
//        double y = position.Y();
//        double z = position.Z();
//        this_L2 = sqrt( x*x + y*y + z*z );
//        this_polar = acos(  z/this_L2 );
//        this_az = atan2( y ,  x );
//
//
//        thisSampleOrientation = getRef<Geometry::V3D >( "sample_orientation" ,
//            peakNum );
//        double mult = 180/3.1415926535897932384626433832795;
//        this_chi = thisSampleOrientation.Y()*mult;
//        this_phi = thisSampleOrientation.X()*mult;
//        this_omega = thisSampleOrientation.Z()*mult;
//
//        if( thisRun != LastRun  || LastDet != thisDet )
//        {
//          out <<  "0 NRUN DETNUM    CHI    PHI  OMEGA MONCNT" <<  std::endl;
//          out <<  "1" <<  std::setw( 5 ) <<  thisRun <<  std::setw( 7 ) <<
//              std::right <<  thisDet;
//
//          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
//          <<  this_chi;
//
//          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
//          <<  this_phi;
//
//          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
//          <<  this_omega;
//          out  <<  std::setw( 7 ) <<  (int)( .5 + this_monCt ) <<  std::endl;
//          LastRun = thisRun;
//          LastDet = thisDet;
//          out <<  "2   SEQN    H    K    L     COL     ROW    CHAN       L2  "
//              <<  "2_THETA       AZ        WL        D   IPK      INTI   "
//              << "SIGI RFLG" <<  std::endl;
//        }
//
//        int h = (int)floor( this_h+.5) , //assume when set via UB those
//            k = (int)floor( this_k +.5 ) , //not close enuf get 0's
//            l = (int)floor( this_l +.5 );
//
//
//        out <<  "3" <<  std::setw( 7 ) <<  seqNum <<  std::setw( 5 ) <<  h
//            <<  std::setw( 5 ) <<  k <<  std::setw( 5 ) <<  l;
//        out <<  std::setw( 8 ) <<  std::fixed << std::setprecision( 2 )
//        << this_col;
//
//        out << std::setw( 8 ) << std::fixed << std::setprecision( 2 )
//        << this_row;
//
//        out << std::setw( 8 ) << std::fixed << std::setprecision( 2 )
//        << this_chan;
//
//
//        out << std::setw( 9 ) << std::fixed << std::setprecision( 3 )
//        << ( this_L2*100 );
//
//        out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
//        << this_polar;
//
//        out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
//        << this_az;
//
//
//        std::vector< double >xx ,  yy;
//        xx.push_back( this_time );
//        Kernel::Units::Wavelength wl;
//        wl.fromTOF( xx ,  yy ,  this_L1 ,  this_L2 ,  this_polar ,
//            0 ,  1.2 ,  1.2 );
//
//        out << std::setw( 10 ) << std::fixed << std::setprecision( 6 )
//        << xx[ 0 ];
//
//        xx[ 0 ] = this_time;
//        Kernel::Units::dSpacing dsp;
//
//        dsp.fromTOF( xx ,  yy ,  this_L1 ,  this_L2 ,  this_polar ,
//            0 ,  1.2 ,  1.2 );
//
//        out << std::setw( 9 ) << std::fixed << std::setprecision( 4 )
//        <<  xx[ 0 ];
//
//        out << std::setw( 6 ) << (int)this_ipk << std::setw( 10 )
//        << std::fixed << std::setprecision( 2 ) << this_inti;
//
//        out << std::setw( 7 ) << std::fixed << std::setprecision( 2 )
//        << this_sigi << std::setw( 5 ) << thisReflag << std::endl;
//
//      }
//
//      out.flush();
//      out.close();
//
//    }catch( char *s )
//    {
//      g_log.error(std::string(s));
//      throw( std::string(s));
//    }
//   catch( std::exception &e)
//   {
//     g_log.error(e.what());
//          throw( e.what());
//   }catch(...)
//   {
//     g_log.error("Unknown Error");
//     throw( "Unknown Error");
//   }
//  }
//
//
//  /** Remove all the peaks from the workspace */
//  void PeaksWorkspace::removeAllPeaks()
//  {
//    for( int i = rowCount() - 1 ; i >= 0 ; i-- )
//      removeRow( i );
//  }
//
//
//
//  /** Append the peaks from a .peaks file into the workspace
//   *
//   * @param filename :: path to the .peaks file
//   */
//  void PeaksWorkspace::append( std::string filename )
//  {
//    try
//    {
//
//      std::ifstream in( filename.c_str() );
//
//      std::string s = readHeader( in );
//
//      if( !in.good() || s.length() < 1 )
//        throw std::runtime_error(
//            std::string( "End of Peaks file before peaks" ) );
//
//      if( s.compare( std::string( "0" ) ) != 0 )
//        throw std::logic_error(
//            std::string( "No header for Peak segments" ) );
//
//      readToEndOfLine( in ,  true );
//      s = getWord( in , false );
//
//      int run , Reflag, detName;
//      double chi , phi , omega , monCount;
//
//      double h , k , l , col , row , chan , L2 , ScatAng , Az , wl , D ,
//      IPK , Inti , SigI;
//      double x , y , z , r , time;
//
//      while( in.good() )
//      {
//        s = readPeakBlockHeader( s ,  in  , run , detName , chi , phi ,
//            omega , monCount );
//
//        s = readPeak( s , in , h , k , l , col , row , chan , L2 ,
//            ScatAng , Az , wl , D , IPK , Inti , SigI , Reflag );
//
//        chi *= M_PI/180;
//        phi *= M_PI/180;
//        omega *= M_PI/180;
//
//        z = L2*cos( ScatAng );
//        r = sqrt( L2*L2 - z*z );
//        x = r*cos( Az );
//        y = r*sin( Az );
//        //Would use the V3D spherical stuff, but it seemed weird
//        std::vector< double >xx , yy;
//        xx.push_back( wl );
//
//        Kernel::Units::Wavelength WL;
//        WL.toTOF( xx , yy , C_L1 , L2 , ScatAng , 12 , 12.1 , 12.1 );
//        time = xx[ 0 ];
//
//
//        this->addPeak( Geometry::V3D( x , y , z ) , time ,
//            Geometry::V3D( h , k , l ) ,
//            Geometry::V3D( phi , chi , omega ) ,
//            Reflag , run , monCount , detName , IPK ,
//            row , col , chan ,  Inti , SigI );
//      }
//  }catch( std::exception & xe)
//  {
//    g_log.error( xe.what());
//    throw (std::logic_error(xe.what()));
//  }catch( char* str)
//  {
//
//
//    g_log.error( std::string(str));
//    throw (std::logic_error(std::string(str)));
//  }catch(...)
//  {
//
//    g_log.error( "Unknown Error");
//        throw ("Unknown Error");
//  }
//
//  }
//
//
//  /** Get the d spacing of the given peak
//   *
//   * @param peakNum :: index of the peak
//   * @return double, d_spacing
//   */
//  double  PeaksWorkspace::get_dspacing(const int peakNum )
//  {
//    double time = getTime(peakNum);
//    double L1   =get_L1(peakNum );
//    Geometry::V3D position = getPosition( peakNum);
//
//    double rho, polar,az;
//    position.getSpherical( rho, polar, az);
//    polar *= M_PI/180;
//    double L2=rho;
//    std::vector< double >xx , yy;
//    xx.push_back( time );
//
//    Kernel::Units::dSpacing dsp;
//    dsp.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
//    return xx[ 0 ];
//
//  }
//
//  double  PeaksWorkspace::get_wavelength( const int peakNum )
//  {
//    double time = cell< double >(peakNum, ItimeCol);
//    double L1   =cell< double >(peakNum, IL1Col );
//    Geometry::V3D position = cell< Geometry::V3D >( peakNum , IpositionCol);
//    double rho, polar,az;
//    position.getSpherical( rho, polar, az);
//    polar *= M_PI/180;
//    double L2=rho;
//    std::vector< double >xx , yy;
//    xx.push_back( time );
//
//    Kernel::Units::Wavelength wl;
//    wl.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
//    return xx[ 0 ];
//  }
//
//
//  //I believe their |Q| = 2pi/d.  This is what is returned.
//  double  PeaksWorkspace::get_Qmagnitude( const int peakNum )
//  {
//
//
//    double time =getTime(peakNum);
//    double L1   = get_L1(peakNum );
//    Geometry::V3D position = getPosition( peakNum );
//    double rho, polar,az;
//    position.getSpherical( rho, polar, az);
//    polar *= M_PI/180;
//    double L2=rho;
//    std::vector< double >xx , yy;
//    xx.push_back( time );
//
//    Kernel::Units::MomentumTransfer Q;
//    Q.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
//    return xx[ 0 ]/2/M_PI;
//  }
//
//  Geometry::V3D    PeaksWorkspace::get_Qlab( const int peakNum )
//  {
//    double MagQ = get_Qmagnitude( peakNum);
//
//
//    Geometry::V3D position =
//        getPosition( peakNum );
//    position =Geometry::V3D(position);
//    position /=position.norm();
//    position.setZ( position.Z()-1);
//    double nrm= position.norm();
//    position *=MagQ/nrm;
//    return position;
//
//
//  }
//
//  Geometry::V3D     PeaksWorkspace::get_QXtal(const int peakNum )
//  {
//
//    Geometry::V3D Qvec= Geometry::V3D( get_Qlab( peakNum) );
//
//    Geometry::V3D sampOrient = getSampleOrientation( peakNum );
//
//    //phi, chi, omega
//    Geometry::Quat rot;
//    sampOrient *=180/M_PI;
//    rot.setAngleAxis( -sampOrient[2],Geometry::V3D( 0,1,0));
//
//    rot.rotate( Qvec );
//
//    rot.setAngleAxis( -sampOrient[1] ,Geometry::V3D( 0,0,1));
//    rot.rotate( Qvec );
//
//    rot.setAngleAxis( -sampOrient[0],Geometry::V3D( 0,1,0));
//
//    rot.rotate( Qvec );
//
//
//    return Qvec;
//  }
//
//  Geometry::V3D   PeaksWorkspace::get_hkl( const int peakNum )
//  {
//
//    return Geometry::V3D( cell< Geometry::V3D >(peakNum , IhklCol ));
//  }
//
//  double     PeaksWorkspace::get_row(const int peakNum )
//  {
//    return cell< double >( peakNum , IPeakRowCol );
//  }
//
//  double     PeaksWorkspace::get_ipk(const int peakNum )
//  {
//
//    return cell< double >( peakNum , IPeakIntensityCol );
//  }
//
//  double     PeaksWorkspace::get_column( const int peakNum )
//  {
//
//    return cell< double >( peakNum , IPeakColCol );
//  }
//
//  double     PeaksWorkspace::get_time_channel( const int peakNum )
//  {
//
//    return cell< double >( peakNum , IPeakChanCol );
//  }
//
//  double  PeaksWorkspace::get_time_offset( const int peakNum )
//  {
//
//    return cell< double >( peakNum ,ItimeOffsetChanCol) ;
//  }
//
//  double  PeaksWorkspace::get_L1(const int peakNum )
//  {
//
//    return cell< double >( peakNum , IL1Col );
//  }
//
//  double  PeaksWorkspace::get_L2(const int peakNum )
//  {
//
//    return cell< Geometry::V3D>( peakNum ,IpositionCol).norm();
//  }
//
//  int    PeaksWorkspace::get_Bank(const int peakNum )
//  {
//
//    return cell< int >( peakNum , IDetBankCol );
//  }
//
//  Geometry::V3D     PeaksWorkspace::getPosition( const int peakNum )
//  {
//    return Geometry::V3D(cell< Geometry::V3D >( peakNum , IpositionCol ));
//  }
//
//  Geometry::V3D   PeaksWorkspace::getSampleOrientation(const int peakNum)
//  {
//    return Geometry::V3D( cell<Geometry::V3D>( peakNum , IsamplePositionCol));
//  }
//
//  int   PeaksWorkspace::getRunNumber( const int peakNum)
//  {
//    return cell<int>( peakNum , IrunNumCol);
//  }
//
//  int  PeaksWorkspace::getReflag( const int peakNum )
//  {
//      return cell<int>( peakNum , IreflagCol);
//  }
//
//  double  PeaksWorkspace::getMonitorCount( const int peakNum)
//  {
//     return cell<double>( peakNum , IMonitorCountCol );
//  }
//  double  PeaksWorkspace::getPeakCellCount( const int peakNum )
//  {
//    return cell< double >( peakNum , IPeakIntensityCol );
//  }
//
//  double  PeaksWorkspace::getPeakIntegrationCount( const int peakNum )
//  {
//    return cell< double >( peakNum , IPeakIntegrateCol );
//  }
//
//  double  PeaksWorkspace::getPeakIntegrationError( const int peakNum )
//  {
//    return cell< double >( peakNum , IPeakIntegrateErrorCol );
//  }
//
//  void  PeaksWorkspace::sethkls( const Geometry::Matrix<double>UB, const double tolerance,
//                 const bool SetOnlyUnset, const int reflag)
//  {
//     std::pair<int,int> size = UB.size();
//
//     if( size.first !=3 || size.second != 3)
//     {
//       g_log.error( ("orientation matrix is the wrong size"));
//       throw std::runtime_error(" Orientation matrix is the wrong size");
//
//     }
//
//     Geometry::Matrix<double> UBI= Geometry::Matrix<double>(UB);
//     if( UBI.Invert() ==0 )
//     {
//       g_log.error(" Orientation matrix is not invertible");
//       throw std::runtime_error(" Orientation matrix is not invertible");
//     }
//
//     for( int i=0; i< getNumberPeaks(); i++)
//       if( !SetOnlyUnset || get_hkl(i)==Geometry::V3D(0,0,0))
//       {
//         int Reflag = getReflag( i );
//         int unitsDig =  Reflag %10;
//         Geometry::V3D hkl= UBI*get_QXtal(i);
//
//         double h_low = hkl.X()-floor(hkl.X());
//         double k_low = hkl.Y()-floor( hkl.Y());
//         double l_low = hkl.Z()-floor( hkl.Z());
//         if( (h_low <tolerance || (1-h_low)<tolerance ) &&
//             (l_low <tolerance || (1-k_low)<tolerance ) &&
//             (l_low <tolerance || (1-l_low)<tolerance ))
//           {
//             sethkl( hkl, i);
//
//             Reflag = (Reflag /100)+10*reflag + unitsDig;
//           }
//         else
//           {
//             sethkl( Geometry::V3D(0,0,0), i);
//             Reflag = (Reflag /100)+10*0 + unitsDig;
//           }
//         setReflag( Reflag, i);
//
//       }
//  }
//
//  void    PeaksWorkspace::clearhkls( )
//  {
//    for( int i=0; i< rowCount(); i++)
//      cell< Geometry::V3D >( i , IhklCol ) = Geometry::V3D(0,0,0);
//  }
//
//  void    PeaksWorkspace::sethkl( const Geometry::V3D hkl , const int peakNum )
//  {
//    cell< Geometry::V3D >( peakNum , IhklCol ) = Geometry::V3D( hkl );
//  }
//
//  void    PeaksWorkspace::setPeakCount( const double count , const int peakNum )
//  {
//    cell< double >( peakNum , IPeakIntensityCol ) = count;
//  }
//
//  void    PeaksWorkspace::setPeakIntegrateCount( const double count , const int peakNum )
//  {
//    cell< double >( peakNum ,IPeakIntegrateCol) = count;
//  }
//
//  void    PeaksWorkspace::setPeakIntegrateError( const double count , const int peakNum )
//  {
//    cell< double >( peakNum ,IPeakIntegrateErrorCol) = count;
//  }
//
//  void    PeaksWorkspace::setPeakPos( const Geometry::V3D position, const int peakNum )
//  {
//    cell< Geometry::V3D >( peakNum , IpositionCol ) = Geometry::V3D( position );
//  }
//
//  void    PeaksWorkspace::setReflag( const int newValue , const int peakNum )
//  {
//    cell< int >( peakNum ,IreflagCol ) = newValue;
//  }

}
}
