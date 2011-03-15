#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidGeometry/Quat.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "boost/shared_ptr.hpp"
#include <algorithm>
#include <string>
#include <ostream>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

namespace Mantid
{
namespace DataObjects
{
  /// Register the workspace as a type
  DECLARE_WORKSPACE(PeaksWorkspace );

  double pi = 3.1415926535;

  bool  BanksSetUp;
  std::vector< std::string > DetNames;

  std::vector< double* > DetInfo;

  Kernel::DateAndTime  C_experimentDate;


  /** Constructor. Create a table with all the required columns.
   *
   * @return PeaksWorkspace object
   */
  PeaksWorkspace::PeaksWorkspace():TableWorkspace( 0 )
  {

    TableWorkspace::addColumn( std::string( "V3D" )  ,  std::string( "hkl" ) );
    TableWorkspace::addColumn( std::string( "V3D" ) ,  std::string( "position" ) );
    TableWorkspace::addColumn( std::string( "V3D" ) ,  std::string( "sample_orientation" ) );
    TableWorkspace::addColumn( std::string( "int" ) ,  std::string( "reflag" ) );
    TableWorkspace::addColumn( std::string( "int" ) ,  std::string( "bank" ) );
    TableWorkspace::addColumn( std::string( "int" ) ,  std::string( "run" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "ipeak" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "inti" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "sigi" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "moncount" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "row" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "col" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "chan" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "L1" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "L2" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "time" ) );
    TableWorkspace::addColumn( std::string( "double" ) ,  std::string( "t_offset" ) );

    BanksSetUp = false;
  }


  /** Destructor */
  PeaksWorkspace::~PeaksWorkspace()
  {}

  /** Initialize the workspace
   *
   * @param L1
   * @param time_offset
   * @param Facility
   * @param Instrument
   * @param experimentDate
   * @param version
   * @param DetCalFilename
   */
  void PeaksWorkspace::initialize( double      L1 ,               //m
      double      time_offset ,      //microseconds
      std::string Facility ,
      std::string Instrument ,
      Kernel::DateAndTime  experimentDate ,
      std::string version ,
      std::string DetCalFilename
      //Isaw DetCal format for
      //          rectangular detectors
  )
  {
    C_L1 = L1;
    C_time_offset = time_offset;
    C_Facility = Facility;
    C_Instrument = Instrument;
    C_version = version;
    C_experimentDate = experimentDate;
    if( DetCalFilename.length() > 1 )//Set up Detectors
    {
      try
      {
        std::ifstream in( DetCalFilename.c_str() );

        std::string s = readHeader( in );
        in.close();
      }catch( char * str)
      {
        std::cout <<"Exception reading detector info "<<str <<std::endl;
      }

    }
  }

  void PeaksWorkspace::addPeak( const Geometry::V3D position ,   //in m McStas coordinates
      const double time ,    //in seconds
      const Geometry::V3D hkl ,
      const Geometry::V3D sample_orientation ,  //radians ,  phi,chi,omega
      const int  reflag ,
      const int  runNum ,
      const double monCount ,
      const int bankName ,
      const double PeakCellCount ,
      const double row ,
      const double col ,
      const double chan ,
      const double L2 ,
      const double PeakIntegrateCount ,
      const double PeakIntegrateError
  )
  {
    int i = rowCount();
    insertRow( i );
    double T = time;

    try
    {

      getRef< double >( std::string( "ipeak" ) ,  i ) = PeakCellCount;
      getRef< double >( std::string( "inti" ) ,  i ) = PeakIntegrateCount;
      getRef< double >( std::string( "sigi" ) ,  i ) = PeakIntegrateError;
      getRef< double >( std::string( "moncount" ) ,  i ) = monCount;

      getRef< Geometry::V3D >( std::string( "position" ) ,  i ) =
          Geometry::V3D( position );

      getRef< Geometry::V3D >( std::string( "sample_orientation" ) ,  i ) =
          Geometry::V3D( sample_orientation );

      getRef< Geometry::V3D >( std::string( "hkl" ) ,  i ) =
          Geometry::V3D( hkl );

      getRef< double >( std::string( "L1" ) ,  i ) = C_L1;
      getRef< double >( std::string( "time" ) ,  i ) = time;
      getRef< double >( std::string( "t_offset" ) ,  i ) = C_time_offset;
      getRef< int >( std::string( "run" ) ,  i ) = runNum;

      getRef< double >( std::string( "chan" ) ,  i ) = chan;
      getRef< double >( std::string( "L2" ) ,  i ) = L2;
      getRef< int >( std::string( "bank" ) ,  i ) =
          ( bankName );

      getRef< double >( std::string( "row" ) ,  i ) = row;
      getRef< double >( std::string( "col" ) ,  i ) = col;
      getRef< int >( std::string( "reflag" ) ,  i ) = reflag;

    }catch( char * str )
    {
      std::cout <<  "exception occurred " <<  str <<  "\n";
    }

  }


  /** Return the number of peaks
   * @return the number of peaks
   */
  int PeaksWorkspace::getNumberPeaks() const
  {
    return rowCount();
  }

  /** Remove a peak
   *
   * @param peakNum :: peak index to remove
   */
  void PeaksWorkspace::removePeak( int peakNum )
  {
    removeRow( peakNum );
  }


  //returns true if strictly less than
  bool compareAsVecs( std::vector< int > arg1, std::vector< int > arg2 )
  {
    if( arg1.size() < 3 )
    {
      if( arg2.size() < 3 )
        return true;
      else
        return false;
    }

    if( arg2.size() < 3 )
      return false;   //hopefully bad values go to back

    int r1 = arg1[0];
    int r2 = arg2[0];

    if( r1 < r2 )
      return true;

    if( r1 > r2 )
      return false;

    r1 = arg1[1];
    r2 = arg2[1];

    if( r1 < r2 )
      return true;

    return false;
  }



  /** Write an ISAW-style .peaks file
   *
   * @param filename :: filename to write to
   */
  void PeaksWorkspace::write( std::string filename )
  {
    try
    {
      std::ofstream out( filename.c_str() );
      std::string date =C_experimentDate.to_ISO8601_string();

      out <<  "Version: " <<  C_version <<  " Facility: " <<  C_Facility  ;
      out <<  " Instrument: " <<  C_Instrument <<  " Date: " ;
      out<< std::setw(date.length())<<date<< std::endl;

      out <<  "6        L1    T0_SHIFT" <<  std::endl;
      out << "7 "<< std::setw( 11 )  ;
      out <<   std::setprecision( 4 ) <<  std::fixed <<  ( C_L1*100 ) ;
      out << std::setw( 12 ) <<  std::setprecision( 3 ) <<  std::fixed  ;
      out << ( C_time_offset ) <<  std::endl;

      out <<  "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT     DEPTH   DETD  "
          <<  " CenterX   CenterY   CenterZ   BaseX    BaseY    BaseZ    "
          <<  "  UpX      UpY      UpZ" <<  std::endl;

      for(size_t i = 0 ; i < DetNames.size() ; i++ )
      {
        double* info = DetInfo[ i ];
        out <<  "5" <<  std::setw( 7 ) <<  DetNames[ i ];

        out <<  std::setw( 7 ) <<  (int)info[ 0 ] <<  std::setw( 7 );
        out  <<  (int)info[ 1 ]<<  std::setw( 9 ) <<  std::setprecision( 4 );
        out  <<  std::fixed <<  info[ 2 ];

        for( int j = 3 ; j < 15 ; j++ )
        {
          out <<  std::setw( 9 ) <<  std::setprecision( 4 ) <<  std::fixed;
          out <<  info[ j ];

        }
        out  <<  std::endl;

      }


      std::vector< std::vector< int > > SortKeys;

      for( int i = 0 ; i < rowCount() ; i++ )
      {
        std::vector<int > dat ;

        dat.push_back(getRef< int >( "run" ,  i ) );
        dat.push_back( getRef< int >( "bank" ,  i ) );

        dat.push_back( i );
        SortKeys.push_back( dat );
      }

      stable_sort( SortKeys.begin() ,  SortKeys.end() ,  compareAsVecs );

      int LastRun = -1;;
      int LastDet;

      int thisRun ,  thisReflag;
      double this_h ,  this_k ,  this_l ,   this_row ,  this_col ,
      this_chan ,  this_L2 ,  this_az ,  this_polar ,  this_time ,
      this_inti ,  this_ipk ,   this_sigi ,  this_chi ,  this_phi ,
      this_omega ,   this_monCt ,  this_tOffset ,  this_L1;

      int thisDet;
      Geometry::V3D thisSampleOrientation;
      int seqNum = 0;
      std::vector< std::vector< int > >::iterator it;

      for ( it = SortKeys.begin(); it != SortKeys.end(); ++it )
      {
        seqNum++;
        std::vector<int > elt = *it;
        int peakNum =elt[2];

        this_row =
            getRef< double  >( std::string( "row" ) ,  peakNum );
        Geometry::V3D hkl = getRef< Geometry::V3D  > ( std::string( "hkl" ) ,
            peakNum );
        this_h = hkl.X();
        this_k = hkl.Y();
        this_l = hkl.Z();
        this_col = getRef < double >( std::string( "col" ) ,  peakNum );
        this_chan = getRef< double >( std::string( "chan" ) ,  peakNum );
        this_ipk = getRef< double >( "ipeak" ,  peakNum );
        this_inti = getRef< double >( "inti" ,  peakNum );
        this_sigi = getRef< double >( "sigi" ,  peakNum );

        this_monCt = getRef< double >( std::string( "moncount" ) ,  peakNum );
        this_L1 = getRef< double >( "L1" ,  peakNum );
        this_tOffset = getRef< double >( "t_offset" ,  peakNum );
        this_time = getRef< double >( "time" ,  peakNum );
        thisRun = getRef< int >( "run" ,  peakNum );
        thisReflag = Int( peakNum ,  3 );
        thisDet = getRef< int >( "bank" ,  peakNum );

        Geometry::V3D position = getRef< Geometry::V3D >( "position" ,
            peakNum );

        double x = position.X();
        double y = position.Y();
        double z = position.Z();
        this_L2 = sqrt( x*x + y*y + z*z );
        this_polar = acos(  z/this_L2 );
        this_az = atan2( y ,  x );


        thisSampleOrientation = getRef<Geometry::V3D >( "sample_orientation" ,
            peakNum );
        double mult = 180/3.1415926535897932384626433832795;
        this_chi = thisSampleOrientation.Y()*mult;
        this_phi = thisSampleOrientation.X()*mult;
        this_omega = thisSampleOrientation.Z()*mult;

        if( thisRun != LastRun  || LastDet != thisDet )
        {
          out <<  "0 NRUN DETNUM    CHI    PHI  OMEGA MONCNT" <<  std::endl;
          out <<  "1" <<  std::setw( 5 ) <<  thisRun <<  std::setw( 7 ) <<
              std::right <<  thisDet;

          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
          <<  this_chi;

          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
          <<  this_phi;

          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )
          <<  this_omega;
          out  <<  std::setw( 7 ) <<  (int)( .5 + this_monCt ) <<  std::endl;
          LastRun = thisRun;
          LastDet = thisDet;
          out <<  "2   SEQN    H    K    L     COL     ROW    CHAN       L2  "
              <<  "2_THETA       AZ        WL        D   IPK      INTI   "
              << "SIGI RFLG" <<  std::endl;
        }

        int h = (int)floor( this_h+.5) , //assume when set via UB those
            k = (int)floor( this_k +.5 ) , //not close enuf get 0's
            l = (int)floor( this_l +.5 );


        out <<  "3" <<  std::setw( 7 ) <<  seqNum <<  std::setw( 5 ) <<  h
            <<  std::setw( 5 ) <<  k <<  std::setw( 5 ) <<  l;
        out <<  std::setw( 8 ) <<  std::fixed << std::setprecision( 2 )
        << this_col;

        out << std::setw( 8 ) << std::fixed << std::setprecision( 2 )
        << this_row;

        out << std::setw( 8 ) << std::fixed << std::setprecision( 2 )
        << this_chan;


        out << std::setw( 9 ) << std::fixed << std::setprecision( 3 )
        << ( this_L2*100 );

        out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
        << this_polar;

        out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
        << this_az;


        std::vector< double >xx ,  yy;
        xx.push_back( this_time );
        Kernel::Units::Wavelength wl;
        wl.fromTOF( xx ,  yy ,  this_L1 ,  this_L2 ,  this_polar ,
            0 ,  1.2 ,  1.2 );

        out << std::setw( 10 ) << std::fixed << std::setprecision( 6 )
        << xx[ 0 ];

        xx[ 0 ] = this_time;
        Kernel::Units::dSpacing dsp;
        dsp.fromTOF( xx ,  yy ,  this_L1 ,  this_L2 ,  this_polar ,
            0 ,  1.2 ,  1.2 );

        out << std::setw( 9 ) << std::fixed << std::setprecision( 4 )
        <<  xx[ 0 ];

        out << std::setw( 6 ) << (int)this_ipk << std::setw( 10 )
        << std::fixed << std::setprecision( 2 ) << this_inti;

        out << std::setw( 7 ) << std::fixed << std::setprecision( 2 )
        << this_sigi << std::setw( 5 ) << thisReflag << std::endl;

      }

      out.flush();
      out.close();

    }catch( char *s )
    {
      std::cout << "Exception =" << s << std::endl;
    }
   catch( std::exception &e)
   {
     std::cout << "exception =" << e.what() << std::endl;
   }catch(...)
   {
     std::cout << "Exception =???" << std::endl;
   }
  }


  /** Remove all the peaks from the workspace */
  void PeaksWorkspace::removeAllPeaks()
  {
    for( int i = rowCount() - 1 ; i >= 0 ; i-- )
      removeRow( i );
  }


  //stops at spaces and \n. strips
  std::string getWord( std::ifstream &in ,  bool consumeEOL )
  {
    std::string s;
    char c = 0;
    if( in.good() )
      for(  c = in.get() ; c == ' ' && in.good() ; c = in.get() )
      {}
    else
      return std::string();

    if( c == '\n' )
    {
      if( !consumeEOL )
        in.putback( c );

      return std::string();
    }

    s.push_back( c );

    if( in.good() )
      for(  c = in.get() ; in.good()&&c != ' ' && c != '\n' ; c = in.get() )
        s.push_back( c );

    if( c == '\n' && !consumeEOL )
      in.putback( c );

    return s;
  }


  void readToEndOfLine( std::ifstream& in ,  bool ConsumeEOL )
  {
    while( in.good() && getWord( in ,  false ).length() > 0  )
      getWord( in ,  false );

    if( !ConsumeEOL )
      return ;

    getWord( in ,  true );
  }


  /** Reads the header of a .peaks file
   *
   * @param in :: stream of the input file
   * @return TODO: I don't know what here
   */
  std::string PeaksWorkspace::readHeader( std::ifstream& in )
  {

    std::string r = getWord( in ,  false );

    if( r.length() < 1 )
      throw std::logic_error( std::string( "No first line of Peaks file" ) );

    if( r.compare( std::string( "Version:" ) ) != 0 )
      throw std::logic_error(
          std::string( "No Version: on first line of Peaks file" ) );

    C_version = getWord( in ,  false );

    if( C_version.length() < 1 )
      throw  std::logic_error( std::string( "No Version for Peaks file" ) );

    C_Facility = getWord( in ,  false );

    if( C_Facility.length() < 1  )
      throw  std::logic_error(
          std::string( "No Facility tag for Peaks file" ) );

    if( C_Facility.compare( std::string( "Facility:" ) ) != 0 )
      throw  std::logic_error(
          std::string( "No Facility tag for Peaks file" ) );

    C_Facility = getWord( in ,  false );

    if( C_Facility.length() < 1 )
      throw  std::logic_error( std::string( "No Facility in Peaks file" ) );

    C_Instrument = getWord( in ,  false );

    if( C_Instrument.length() < 1 )
      throw  std::logic_error(
          std::string( "No Instrument tag for Peaks file" ) );

    if( C_Instrument.compare( std::string( "Instrument:" ) ) != 0 )
      throw  std::logic_error(
          std::string( "No Instrument tag for Peaks file" ) );

    C_Instrument = getWord( in ,  false );

    if( C_Instrument.length() < 1 )
      throw std::logic_error(
          std::string( "No Instrument for Peaks file" ) );

    std::string date = getWord( in ,  false );

    if(date .length()< 1)
    {
      Kernel::DateAndTime x = Kernel::DateAndTime::get_current_time();
      C_experimentDate.set_from_time_t(x.to_time_t());
    }

    else
      if( date.compare( std::string( "Date:" ) ) == 0 )
      {
        date = getWord( in ,  false );
        C_experimentDate = Kernel::DateAndTime( date );
      }


    while( getWord( in ,  false ).length() > 0 )
      getWord( in ,  false );

    // Now get info on detector banks

    getWord( in ,  true );
    std::string s = getWord( in ,  false );
    if( s.compare( "6" ) != 0 )
      throw std::logic_error(
          std::string( "Peaks File has no L0 and T0 header info" ) );

    readToEndOfLine( in ,  true );
    s = getWord( in ,  false );

    if( s.compare( "7" ) != 0 )
      throw std::logic_error(
          std::string( "Peaks File has no L0 and T0 info" ) );

    C_L1 = strtod( getWord( in ,  false ).c_str() ,  0 )/100;
    C_time_offset = strtod( getWord( in ,  false ).c_str() ,  0 );


    readToEndOfLine( in ,  true );
    for( s = getWord( in ,  false ) ; s.length() < 1 ; getWord( in ,  true ) )
    {
      s = getWord( in ,  false );
    }

    if( s.compare( "4" ) != 0 )
      throw std::logic_error( std::string( "Peaks File has no bank descriptor line" ) );

    readToEndOfLine( in ,  true );
    s = getWord( in ,  false );
    while( s.length() < 1 && in.good() )
    {
      s = getWord( in ,  true );
      s = getWord( in ,  false );
    }

    if(  !in.good() )
      throw std::runtime_error( std::string( "Peaks File Quit too fast" ) );

    if( s.compare( std::string( "5" ) ) != 0 )
      throw std::logic_error( "No information on banks given" );//log this only


    bool err = false;
    while( s.compare( std::string( "5" ) ) == 0  )
    {
      double* data = new double[ 15 ];//Use boost pointers here??? does gc
      s = getWord( in ,  false );

      if( s.length() < 1 )
        err = true;
      else
        DetNames.push_back( s );

      for( int i = 0 ; i < 15 && !err ; i++ )
      {
        s = getWord( in ,  false );
        if( s.length() < 1 )
          err = true;
        else
          data[ i ] = strtod( s.c_str() ,  0 );

      }

      if( err )
        s = std::string( "2" );
      else
      {
        DetInfo.push_back( data );

        for( s = getWord( in ,  false ) ; s.length() > 0 ; s = getWord( in ,  false ) )
        {}

        s = getWord( in ,  true );
        s = getWord( in ,  false );//no blank lines will be allowed so far
      }
    }


    if( err )
      return std::string();

    return s;
  }



  std::string readPeak( std::string lastStr ,  std::ifstream& in ,
      double& h , double& k ,  double& l ,  double& col ,
      double& row , double& chan ,  double& L2 ,
      double&  ScatAng , double& Az ,  double& wl ,
      double& D ,  double& IPK , double& Inti ,
      double& SigI ,  int &iReflag )
  {

    std::string s = lastStr;

    if( s.length() < 1 && in.good() )//blank line
    {
      readToEndOfLine( in ,  true );

      s = getWord( in ,  false );;
    }

    if( s.length() < 1 )
      return 0;


    if( s.compare( "2" ) == 0 )
    {
      readToEndOfLine( in ,  true );

      for( s = getWord( in ,  false ) ; s.length() < 1 && in.good() ;
          s = getWord( in ,  true ) )
      {
        s = getWord( in ,  false );
      }
    }

    if( s.length() < 1 )
      return 0;

    if( s.compare( "3" ) != 0 )
      return s;

    int seqNum = atoi( getWord( in ,  false ).c_str() );

    h = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    k = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    l = strtod( getWord( in ,  false ).c_str() ,  0 ) ;

    col = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    row = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    chan = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    L2 = strtod( getWord( in ,  false ).c_str() ,  0 )/100 ;
    ScatAng = strtod( getWord( in ,  false ).c_str() ,  0 ) ;

    Az = strtod( getWord( in , false ).c_str() , 0 ) ;
    wl = strtod( getWord( in , false ).c_str() , 0 ) ;
    D = strtod( getWord( in , false ).c_str() , 0 ) ;
    IPK = strtod( getWord( in , false ).c_str() , 0 ) ;

    Inti = strtod( getWord( in , false ).c_str() , 0 ) ;
    SigI = strtod( getWord( in , false ).c_str() , 0 ) ;
    iReflag = atoi( getWord( in , false ).c_str() ) ;


    readToEndOfLine( in ,  true );
    return getWord( in , false );
  }


  std::string readPeakBlockHeader( std::string lastStr ,  std::ifstream &in ,
      int&run ,int& detName ,
      double&chi , double&phi ,
      double&omega , double&monCount )
  {
    std::string s = lastStr;

    if( s.length() < 1 && in.good() )//blank line
    {
      readToEndOfLine( in ,  true );
      s = getWord( in , false );
    }

    if( s.length() < 1 )
      return std::string();

    if( s.compare( "0" ) == 0 )
    {
      readToEndOfLine( in ,  true );
      s = getWord( in , false );
      while( s.length() < 1 )
      {
        readToEndOfLine( in ,  true );
        s = getWord( in , false );
      }
    }

    if( s.compare( std::string( "1" ) ) != 0 )
      return s;

    run = atoi( getWord( in , false ).c_str() );
    detName =atoi( getWord( in , false ).c_str());
    chi = strtod( getWord( in , false ).c_str() , 0 );
    phi = strtod( getWord( in , false ).c_str() , 0 );

    omega = strtod( getWord( in , false ).c_str() , 0 );
    monCount = strtod( getWord( in , false ).c_str() , 0 );
    readToEndOfLine( in ,  true );

    return getWord( in , false );

  }
  //special formatted file Use clear peaks to no append

  /** Append the peaks from a .peaks file into the workspace
   *
   * @param filename :: path to the .peaks file
   */
  void PeaksWorkspace::append( std::string filename )
  {
    try
    {

      std::ifstream in( filename.c_str() );

      std::string s = readHeader( in );

      if( !in.good() || s.length() < 1 )
        throw std::runtime_error(
            std::string( "End of Peaks file before peaks" ) );

      if( s.compare( std::string( "0" ) ) != 0 )
        throw std::logic_error(
            std::string( "No header for Peak segments" ) );

      readToEndOfLine( in ,  true );
      s = getWord( in , false );

      int run , Reflag, detName;
      double chi , phi , omega , monCount;

      double h , k , l , col , row , chan , L2 , ScatAng , Az , wl , D ,
      IPK , Inti , SigI;
      double x , y , z , r , time;

      while( in.good() )
      {
        s = readPeakBlockHeader( s ,  in  , run , detName , chi , phi ,
            omega , monCount );

        s = readPeak( s , in , h , k , l , col , row , chan , L2 ,
            ScatAng , Az , wl , D , IPK , Inti , SigI , Reflag );

        chi *= pi/180;
        phi *= pi/180;
        omega *= pi/180;

        z = L2*cos( ScatAng );
        r = sqrt( L2*L2 - z*z );
        x = r*cos( Az );
        y = r*sin( Az );
        //Would use the V3D spherical stuff, but it seemed weird
        std::vector< double >xx , yy;
        xx.push_back( wl );

        Kernel::Units::Wavelength WL;
        WL.toTOF( xx , yy , C_L1 , L2 , ScatAng , 12 , 12.1 , 12.1 );
        time = xx[ 0 ];


        this->addPeak( Geometry::V3D( x , y , z ) , time ,
            Geometry::V3D( h , k , l ) ,
            Geometry::V3D( phi , chi , omega ) ,
            Reflag , run , monCount , detName , IPK ,
            row , col , chan , L2, Inti , SigI );
      }
  }catch( std::exception & xe)
  {
     std::cout<< "exception "<<xe.what()<<std::endl;
  }catch( char* str)
  {

    std::cout<< "err "<<str<<std::endl;
  }catch(...)
  {

    std::cout<< "???? "<<std::endl;
  }

  }


  /** Get the d spacing of the given peak
   *
   * @param peakNum :: index of the peak
   * @return double, d_spacing
   */
  double  PeaksWorkspace::get_dspacing( int peakNum )
  {
    double time = cell< double >(peakNum, ItimeCol);
    double L1   =cell< double >(peakNum, IL1Col );
    double L2   =cell< double >(peakNum, IL2Col );
    Geometry::V3D position =
        cell< Geometry::V3D >( peakNum , IpositionCol);

    double rho, polar,az;
    position.getSpherical( rho, az, polar);
    std::vector< double >xx , yy;
    xx.push_back( time );

    Kernel::Units::dSpacing dsp;
    dsp.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
    return xx[ 0 ];

  }

  double  PeaksWorkspace::get_wavelength( int peakNum )
  {
    double time = cell< double >(peakNum, ItimeCol);
    double L1   =cell< double >(peakNum, IL1Col );
    double L2   =cell< double >(peakNum, IL2Col );
    Geometry::V3D position = cell< Geometry::V3D >( peakNum , IpositionCol);
    double rho, polar,az;
    position.getSpherical( rho, az, polar);
    std::vector< double >xx , yy;
    xx.push_back( time );

    Kernel::Units::Wavelength wl;
    wl.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
    return xx[ 0 ];
  }


  //I believe their |Q| = 2pi/d.  This is what is returned.
  double  PeaksWorkspace::get_Qmagnitude( int peakNum )
  {


    double time = cell< double >(peakNum, ItimeCol);
    double L1   =cell< double >(peakNum, IL1Col );
    double L2   =cell< double >(peakNum, IL2Col );
    Geometry::V3D position = cell< Geometry::V3D >( peakNum , IpositionCol);
    double rho, polar,az;
    position.getSpherical( rho, az, polar);
    std::vector< double >xx , yy;
    xx.push_back( time );

    Kernel::Units::MomentumTransfer Q;
    Q.fromTOF( xx , yy , L1 , L2 , polar , 12 , 12.1 , 12.1 );
    return xx[ 0 ];
  }

  Geometry::V3D    PeaksWorkspace::get_Qlab( int peakNum )
  {
    double MagQ = get_Qmagnitude( peakNum);


    Geometry::V3D position =
        cell< Geometry::V3D >( peakNum , IpositionCol);
    position =Geometry::V3D(position);
    position /=position.norm();
    position.setZ( position.Z()-1);
    position *=MagQ;
    return position;

    return Geometry::V3D( );
  }

  Geometry::V3D     PeaksWorkspace::get_QXtal( int peakNum )
  {

    Geometry::V3D Qvec= Geometry::V3D( get_Qlab( peakNum) );

    Geometry::V3D sampOrient =
        cell< Geometry::V3D >( peakNum , IsamplePositionCol );
    //phi, chi, omega
    Geometry::Quat rot;
    rot.setAngleAxis( -sampOrient[2],Geometry::V3D( 0,1,0));

    rot.rotate( Qvec );

    rot.setAngleAxis( -sampOrient[1] ,Geometry::V3D( 0,0,1));
    rot.rotate( Qvec );

    rot.setAngleAxis( -sampOrient[0],Geometry::V3D( 0,1,0));

    rot.rotate( Qvec );


    return Qvec;
  }

  Geometry::V3D   PeaksWorkspace::get_hkl( int peakNum )
  {

    return Geometry::V3D( cell< Geometry::V3D >(peakNum , IhklCol ));
  }

  double     PeaksWorkspace::get_row(int peakNum )
  {
    return cell< double >( peakNum , IPeakRowCol );
  }

  double     PeaksWorkspace::get_ipk( int peakNum )
  {

    return cell< double >( peakNum , IPeakIntensityCol );
  }

  double     PeaksWorkspace::get_column( int peakNum )
  {

    return cell< double >( peakNum , IPeakColCol );
  }

  double     PeaksWorkspace::get_time_channel( int peakNum )
  {

    return cell< double >( peakNum , IPeakChanCol );
  }

  double  PeaksWorkspace::get_time_offset( int peakNum )
  {

    return cell< double >( peakNum ,ItimeOffsetChanCol) ;;
  }

  double  PeaksWorkspace::get_L1(int peakNum )
  {

    return cell< double >( peakNum , IL1Col );
  }

  double  PeaksWorkspace::get_L2(int peakNum )
  {

    return cell< double >( peakNum , IL2Col );
  }

  int    PeaksWorkspace::get_Bank( int peakNum )
  {

    return cell< int >( peakNum , IDetBankCol );
  }

  Geometry::V3D     PeaksWorkspace::getPosition( int peakNum )
  {
    return cell< Geometry::V3D >( peakNum , IpositionCol );
  }

  double  PeaksWorkspace::getPeakCellCount( int peakNum )
  {
    return cell< double >( peakNum , IPeakIntensityCol );
  }

  double  PeaksWorkspace::getPeakIntegrationCount( int peakNum )
  {
    return cell< double >( peakNum , IPeakIntegrateCol );
  }

  double  PeaksWorkspace::getPeakIntegrationError( int peakNum )
  {
    return cell< double >( peakNum , IPeakIntegrateErrorCol );
  }

  void    PeaksWorkspace::sethkls( Geometry::Matrix< double >UB ,   bool tolerance ,
      bool SetOnlyUnset )
  {
    //TODO
  }

  void    PeaksWorkspace::clearhkls( )
  {
    for( int i=0; i< rowCount(); i++)
      cell< Geometry::V3D >( i , IhklCol ) = Geometry::V3D(0,0,0);
  }

  void    PeaksWorkspace::sethkl( const Geometry::V3D hkl , int peakNum )
  {
    cell< Geometry::V3D >( peakNum , IhklCol ) = Geometry::V3D( hkl );
  }

  void    PeaksWorkspace::setPeakCount( double count ,  int peakNum )
  {
    cell< double >( peakNum , IPeakIntensityCol ) = count;
  }

  void    PeaksWorkspace::setPeakIntegrateCount( double count ,  int peakNum )
  {
    cell< double >( peakNum ,IPeakIntegrateCol) = count;
  }

  void    PeaksWorkspace::setPeakIntegrateError( double count ,  int peakNum )
  {
    cell< double >( peakNum ,IPeakIntegrateErrorCol) = count;
  }

  void    PeaksWorkspace::setPeakPos( Geometry::V3D position, int peakNum )
  {
    cell< Geometry::V3D >( peakNum , IpositionCol ) = Geometry::V3D( position );
  }

  void    PeaksWorkspace::setReflag( int newValue , int peakNum )
  {
    cell< int >( peakNum ,IreflagCol ) = newValue;
  }

}
}
