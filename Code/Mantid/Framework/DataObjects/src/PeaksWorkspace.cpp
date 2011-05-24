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
    UNUSED_ARG(type)
    // Create the PeakColumn.
    columns.push_back( boost::shared_ptr<Mantid::DataObjects::PeakColumn>(new Mantid::DataObjects::PeakColumn( this->peaks, name) ) );
    // Cache the names
    columnNames.push_back(name);
    return true;
  }

  //---------------------------------------------------------------------------------------------
  /// @return the index of the column with the given name.
  int PeaksWorkspace::getColumnIndex(const std::string& name)
  {
    for (int i=0; i < int(columns.size()); i++)
      if (columns[i]->name() == name)
        return i;
    throw std::invalid_argument("Column named " + name + " was not found in the PeaksWorkspace.");
  }

  //---------------------------------------------------------------------------------------------
  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<Mantid::API::Column> PeaksWorkspace::getColumn(int index)
  {
    if (index >= static_cast<int>(columns.size())) throw std::invalid_argument("PeaksWorkspace::getColumn() called with invalid index.");
    return columns[index];
  }


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


}
}

///\cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
    Mantid::DataObjects::PeaksWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::PeaksWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected PeaksWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::PeaksWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::PeaksWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const PeaksWorkspace.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
