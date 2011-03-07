#ifndef MANTID_DATAOBJECTS_PEAKSPACE_H_
#define MANTID_DATAOBJECTS_PEAKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include "MantidKernel/Logger.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/Matrix.h"


namespace Mantid
{
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/*namespace Kernel
{
  class Logger;
}
*/
namespace DataObjects
{

class DLLExport PeaksWorkspace: public DataObjects::TableWorkspace
{
  public:

     virtual const std::string id() const {return "PeaksWorkspace";}

     PeaksWorkspace();

     void initialize(double      L0,              //cm
                     double      time_offset,     //seconds
                     std::string Facility,
                     std::string Instrument,
                     Kernel::DateAndTime  experimentDate,
                     std::string version= std::string("1"),
                     std::string InstrumentDescriptorFileName =0);
    
     virtual ~PeaksWorkspace();

     void addPeak( const Geometry::V3D position,  //in cm McStas coordinates
                   const double time,   //in seconds
                   const Geometry::V3D hkl,
                   const Geometry::V3D sample_orientation, //radians, order as in NeXus file
                   const int  reflag,
                   const int  runNum,
                   const double  monCount,
                   const int bankName,
                   const double PeakCellCount,
                   const double row=0,
                   const double col=0,
                   const double chan=0,
                   const double PeakIntegrateCount=0,
                   const double PeakIntegrateError=0);

     int getNumberPeaks() const;

     void removePeak( int peakNum);
     //iterator comes from table work space, 
     // Examine and change cells with table workspace.
     
     //column names are hkl,position,sample_orientation,reflag,time,L0,t_offset,
     //                  bank,run,ipeak,inti,sigi,moncount,det,row,col,chan.

     double  get_dspacing( int peakNum);
     double  get_wavelength( int peakNum);
     double  get_Qmagnitude( int peakNum);
     Geometry::V3D   get_Qlab( int peakNum);
     Geometry::V3D   get_QXtal( int peakNum);
     Geometry::V3D   get_hkl( int peakNum);
     double     get_row(int peakNum);//?
     double     get_column( int peakNum);//?
     double     get_time_channel( int peakNum);//?
     double  get_time_offset( int peakNum);
     double  get_L0(int peakNum);
     int    get_Bank(int peakNum);
     Geometry::V3D     getPosition( int peakNum);
     double  getPeakCellCount( int peakNum);
     double  getPeakIntegrationCount( int peakNum);
     double  getPeakIntegrationError( int peakNum);
     void    sethkls( Geometry::Matrix<double>UB,  bool tolerance, bool SetOnlyUnset);
     void    clearhkls( );
     void    sethkl( const Geometry::V3D hkl,int peakNum);
     void    setPeakCount( double count, int peakNum);
     void    setPeakIntegrateCount( double count, int peakNum);
     void    setPeakPos( Geometry::V3D position, int peakNum);
     void    setReflag( int newValue, int peakNum);

     void    write( std::string filename );
     void    append( std::string filename);
     void    removeAllPeaks();

//     bool   Mantid::API::ITableWorkspace::addColumn  	(  	const std::string &   	 type,
//		                 const std::string &  	name
//	    ) {return false;}

     static const int IhklCol =0;  //Column number where V3D hkl value is stored
     static const int IpositionCol =1;//Column number where V3D xyz position is stored
     static const int IsamplePositionCol =2;//Column where V3d sample orientation is stored
     static const int IreflagCol =3;
     static const int IDetBankCol =4;
     static const int IrunNumCol =5;
     static const int IPeakIntensityCol =6;
     static const int IPeakIntegrateCol =7;
     static const int IPeakIntegrateErrorCol =8;
     static const int IMonitorCountCol =9;
     static const int IPeakRowCol =10;
     static const int IPeakColCol =11;
     static const int IPeakChanCol =12;
     static const int IL1Col =13;
     static const int IL2Col =14;
     static const int ItimeCol =15;
     static const int ItimeOffsetChanCol =16;


       // >>   and << add later
       // getCell should be disabled or describe column numbers with static strings
       //typedefs???  boost ptr to peaks object
       //  getRow, a row is a peak ??? Does not quite fit. TableRow NG
       // copy "All" methods using const.



  //private:
        // ?? placing what here reduces circular dependencies and/or link times???

};


}
}
#endif


    


