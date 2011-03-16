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
namespace Kernel
{
  class Logger;
}

namespace DataObjects
{
/**
 *  PeaksWorkspace stores information about a set of SCD peaks.
 */
class DLLExport PeaksWorkspace: public DataObjects::TableWorkspace
{
  public:

     virtual const std::string id() const {return "PeaksWorkspace";}

     PeaksWorkspace();

     /**
          * Initializes calibration values for instrument
          *
          * @param L1              Initial path length in m
          * @parma time_offset     time offset in microseconds
          * @param  Faciity        Facility name
          * @param  Instrument     Instrument name
          * @param  experimentDate Date of experiment
          * @param version         version number for the peaks file format
          * @param PanelNames      The names of the panels
          * @param PanelInfo       panels information as follows:
          *                        #rows,#cols,width(m),height(m),depth(m),detD(m),
          *                        center x,y,z; base x,y,z; up x,y,z
          *                        where x,y(vert),z(beam) is in m McStas coords
          */
         void initialize(double      L1,              //cm
                         double      time_offset,     //seconds
                         std::string Facility,
                         std::string Instrument,
                         Kernel::DateAndTime  experimentDate,
                         std::string version,
                         std::vector<std::string> &PanelNames,
                         std::vector< double*> &PanelInfo);

         /**
          * Initializes calinration values for instrument from an Isaw-like DetCal file
          * @param DetCalFileName   The name of the DetCal file.
          */
         void initialize(  std::string DetCalFileName);
    
     virtual ~PeaksWorkspace();

     /**
      * Add a peak to the list of peaks
      *
      * @param position                  The position of the peak in meters and McStas Coordinates
      * @param time                      The time in microseconds of the peak
      * @param hkl                       The h,k, and l values for the peak. (0,0,0) if not indexed
      * @param sampe_orientation         The sample orientation in radians, using the NeXus convention
      * @param reflag                    The reflag for the status of various calculations for this peak.
      * @param runNum                    The run number associated with the peak
      * @param monCount                  The monitor count for the run
      * @param bankName                  The bankName as an int
      * @param PeakCellCount             The count for the intensity at middle pixel(detector)
      * @param row                       The row for the peak, if there is one, otherwise -1
      * @param col                       The column for the peak, if there is one, otherwise -1.
      * @param chan                      The time channel for the peak, if there is one
      * @param PeakIntegrateCount        The count for the intensity-background of all pixels(detectors) spanned by the peak
      * @param PeakIntegrateError        The error in the PeakIntegrateCount value.
      */
     void addPeak( const Geometry::V3D position,
                   const double time,
                   const Geometry::V3D hkl,
                   const Geometry::V3D sample_orientation,
                   const int  reflag,
                   const int  runNum,
                   const double  monCount,
                   const int bankName,
                   const double PeakCellCount,
                   const double row=0,
                   const double col=0,
                   const double chan=0,
                   const double L2=0,
                   const double PeakIntegrateCount=0,
                   const double PeakIntegrateError=0);

     /**
      * @return the number of peaks
      */
     int getNumberPeaks() const;

     /**
      * Removes the indicated peak
      * @param peakNum  the peak to remove. peakNum starts at 0
      */
     void removePeak( int peakNum);
     

     //column names are hkl,position,sample_orientation,reflag,time,L1,t_offset,
     //                  bank,run,ipeak,inti,sigi,moncount,det,row,col,chan.

     /**
      * @return the d-spacing in Angstroms for the indicated peak
      */
     double  get_dspacing( int peakNum);

     /**
      * @return the wavelength in Angstroms of the indicated peak
      */
     double  get_wavelength( int peakNum);

     /**
      *   @return The magnitude of Q in inv Angstroms for the indicated peak. This magnitude = 1/d spacing
      */
     double  get_Qmagnitude( int peakNum);

     /**
      * @return The vector Q in inv Angstroms(magnitude =1/d spacing) of the indicated peak.
      *         This vector is at the given sample orientation
      */
     Geometry::V3D   get_Qlab( int peakNum);

     /**
      * @return The vector Q in inv Angstroms(magnitude =1/d spacing) of the indicated peak after
      *         adjusting for the sample orientation.
      *
      */
     Geometry::V3D   get_QXtal( int peakNum);


     /**
      * @return the h,k, and l value for the given peak
      */
     Geometry::V3D   get_hkl( int peakNum);

     /**
      * @return the row of the given peak
      */
     double     get_row(int peakNum);

     /**
      * @return the column of the given peak
      */
     double     get_column( int peakNum);

     /**
      * @return the time channel of the given peak
      */
     double     get_time_channel( int peakNum);

     /**
      * @return the time offset of the given peak in microseconds
      */
     double  get_time_offset( int peakNum);

     /**
      * @return peak intensity at "center"
      */
     double  get_ipk(int peakNum);

     /**
      * @return the initial path length for the peak in m
      */
     double  get_L1(int peakNum);

     /**
      * @return the sample to peak length for the peak in m
      */
     double  get_L2(int peakNum);


     /**
      * @return the bank number associated with the given peak
      */
     int    get_Bank(int peakNum);

     /**
      * @return the position of the given peak in m and in McStas coordinates
      */
     Geometry::V3D     getPosition( int peakNum);

     /**
      * @return the intensity of the "middle" cell of the peak
      */
     double  getPeakCellCount( int peakNum);

     /**
      * @return the total intensity -background for all the cells associated with the peak
      */
     double  getPeakIntegrationCount( int peakNum);

     /**
      *  @return the error in the PeakIntegration count
      */
     double  getPeakIntegrationError( int peakNum);


     /**
      * Sets the h,k, and l values for all peaks using the orientation matrix
      *
      * @param UB the orientation matrix that maps the column hkl vector to the column
      *            q vector where |q|=1/d-spacing and  x,y(up),z(beam) are in McStas coordinates
      *
      *@param tolerance  Only index peaks if the h,k, and l values are within tolerance of an integer
      *                   otherwise set to 0,0,0
      *@param SetOnlyUnset  if true, only peaks with h,k,l values of 0,0,0 will be set.
      *                     Used for twinned peaks.
      *
      *@param reflag  If SetOnlyUnset is true, gives the new reflag value for 2nd digit
      */
     void    sethkls( Geometry::Matrix<double>UB,  double tolerance, bool SetOnlyUnset, int reflag);

     /**
      * Sets all the h,k,l values to 0,0,0
      */
     void    clearhkls( );

     /**
      *  sets the h,k,l value for the given peak
      */
     void    sethkl( const Geometry::V3D hkl,int peakNum);


     /**
      * Sets the Peak Intensity count for the indicated peak
      */
     void    setPeakCount( double count, int peakNum);

     /**
      *  Sets the total peak intensity-background for the indicated peak
      */
     void    setPeakIntegrateCount( double count, int peakNum);

     /**
      * Sets the error in the PeakIntegrate Count
      */
     void    setPeakIntegrateError( double count, int peakNum);

     /**
      * Sets the peak position( in m and McStas) for the indicated peak
      */
     void    setPeakPos( Geometry::V3D position, int peakNum);

     /**
      * Sets the status flag for the indicated peak
      */
     void    setReflag( int newValue, int peakNum);

     /**
      *  Writes the peak information to a peaks file
      */
     void    write( std::string filename );

     /**
      * Reads in new peaks from a Peaks file and appends it to the current peaks.
      * Use clearhkls to restart
      */
     void    append( std::string filename);



     /**
      * Removes all peaks
      */
     void    removeAllPeaks();


     bool   addColumn   (   const std::string &      type,
                     const std::string &    name
      ) {return false;}

     static const int IhklCol =0;  /** Column number where V3D hkl value is stored*/
     static const int IpositionCol =1;/** Column number where V3D xyz position is stored*/
     static const int IsamplePositionCol =2;/**Column where V3d sample orientation is stored*/
     static const int IreflagCol =3; /** Column where the reflag is stored */
     static const int IDetBankCol =4; /** Column where the Bank number is store */
     static const int IrunNumCol =5; /** Column where the run number is stored */
     static const int IPeakIntensityCol =6; /** Column where intensity of middle cell is stored */
     static const int IPeakIntegrateCol =7;/** Column where intensity-backgound of all cells in peak is stored */
     static const int IPeakIntegrateErrorCol =8; /** Column where the error in the integrated intensity is stored */
     static const int IMonitorCountCol =9;    /** Column where the monitor count is stored */
     static const int IPeakRowCol =10;      /** Column where the row of the peak is stored */
     static const int IPeakColCol =11;     /** Column where the column of the peak is stored */
     static const int IPeakChanCol =12;      /** Column where the time channel of the peak is stored */
     static const int IL1Col =13;              /** Column where the initial path of the peak is stored */
     static const int IL2Col =14;               /** Column where the L2 of the peak is stored */
     static const int ItimeCol =15;                 /** Column where the time of the peak is stored */
     static const int ItimeOffsetChanCol =16;  /** Column where the time offset of the peak is stored */







  private:


       double C_L1;          /** Last L1 value in m used */
       double C_time_offset;   /** Last time offset used( in usec) */
       std::string C_Facility; /** Last Facility set or read */
       std::string C_Instrument; /** Last Instrument set or read */
       std::string C_version;   /** Last version set or read */

       std::string readHeader( std::ifstream& in );

       void ClearDeleteCalibrationData();
       static Kernel::Logger& g_log;

       std::vector< std::string > DetNames;/** Names of panels for calibration info*/

        std::vector< double* > DetInfo; /** Calibration info for panels */

        Kernel::DateAndTime  C_experimentDate;/** Date associated with peaks file */
};


/// Typedef for a shared pointer to a peaks workspace.
typedef boost::shared_ptr<PeaksWorkspace> PeaksWorkspace_sptr;

}
}
#endif


    


