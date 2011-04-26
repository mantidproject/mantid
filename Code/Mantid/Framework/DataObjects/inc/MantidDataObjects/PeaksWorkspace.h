#ifndef MANTID_DATAOBJECTS_PEAKSPACE_H_
#define MANTID_DATAOBJECTS_PEAKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Column.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakColumn.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include <string>


//IsamplePosition should be IsampleOrientation
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
  //==========================================================================================
  /** @class Mantid::DataObjects::PeaksWorkspace

     The class PeaksWorkspace stores information about a set of SCD peaks.

      @author Ruth Mikkelson, SNS ORNL
      @date 3/10/2010

      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

      This file is part of Mantid.

      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.

      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.

      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
   */
  class DLLExport PeaksWorkspace: public Mantid::API::ITableWorkspace
  {
  public:

    virtual const std::string id() const {return "PeaksWorkspace";}

    PeaksWorkspace();

    virtual ~PeaksWorkspace();

    void appendFile( std::string filename, Mantid::Geometry::IInstrument_sptr inst);

    /** Sets the default instrument for new peaks */
    void setInstrument(Mantid::Geometry::IInstrument_sptr inst)
    { m_defaultInst = inst; }

    /** Returns the default instrument for new peaks */
    Mantid::Geometry::IInstrument_sptr getInstrument()
    { return m_defaultInst; }

    //---------------------------------------------------------------------------------------------
    /** @return the number of peaks
     */
    int getNumberPeaks() const
    {
      return int(peaks.size());
    }

    /** Removes the indicated peak
     * @param peakNum  the peak to remove. peakNum starts at 0
     */
    void removePeak(const int peakNum)
    {
      if (peakNum >= static_cast<int>(peaks.size()) ) throw std::invalid_argument("PeaksWorkspace::removePeak(): peakNum is out of range.");
      peaks.erase(peaks.begin()+peakNum);
    }

    /** Add a peak to the list
     * @param peak :: Peak object to add (copy) into this.
     */
    void addPeak(const Peak peak)
    {
      peaks.push_back(peak);
    }

    /** Return a reference to the Peaks vector */
    std::vector<Peak> & getPeaks()
    {
      return peaks;
    }

    /// Number of columns in the workspace.
    virtual int columnCount() const
    {
      return static_cast<int>(columns.size());
    }

    /// Return the memory used in bytes
    virtual size_t getMemorySize() const
    {
      return getNumberPeaks() * sizeof(Peak);
    }

    //---------------------------------------------------------------------------------------------
    /// Gets the shared pointer to a column by name.
    virtual boost::shared_ptr<Mantid::API::Column> getColumn(const std::string& name)
    { return getColumn(getColumnIndex(name)); }

    /// @return the index of the column with the given name.
    virtual int getColumnIndex(const std::string& name);

    /// Gets the shared pointer to a column by index.
    virtual boost::shared_ptr<Mantid::API::Column> getColumn(int index);


    /// Returns a vector of all column names.
    virtual std::vector<std::string> getColumnNames()
    {
      return this->columnNames;
    }

    /// Number of rows in the workspace.
    virtual int rowCount() const
    {
      return getNumberPeaks();
    }




  private:
    /** Vector of Peak contained within. */
    std::vector<Peak> peaks;

    /// Default instrument for new peaks
    Mantid::Geometry::IInstrument_sptr m_defaultInst;

    /** Cached list of column names in order */
    std::vector<std::string> columnNames;

    /** Cached list of column names in order */
    std::vector<std::string> columnTypes;

    /** Column shared pointers. */
    std::vector<boost::shared_ptr<Mantid::DataObjects::PeakColumn> > columns;




//
//    /** Last L1 value in m used */
//    double C_L1;
//
//    /** Last time offset used( in usec) */
//    double C_time_offset;
//
//    /** Last Facility set or read */
//    std::string C_Facility;
//
//    /** Last Instrument set or read */
//    std::string C_Instrument;
//
//    /** Last version set or read */
//    std::string C_version;
//
//    std::string readHeader( std::ifstream& in );
//
//    void ClearDeleteCalibrationData();
//    static Kernel::Logger& g_log;
//
//    std::vector< std::string > DetNames;/** Names of panels for calibration info*/
//
//    std::vector< double* > DetInfo; /** Calibration info for panels */
//
//    Kernel::DateAndTime  C_experimentDate;/** Date associated with peaks file */
//
//    void buildColumns();


  public:

    // ===== Methods that are not implemented (read-only table) ==========

    virtual bool addColumn(const std::string& /*type*/, const std::string& /*name*/);

    virtual bool addColumns(const std::string& /*type*/, const std::string& /*name*/, int /*n*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace structure is read-only. Cannot add column."); }

    virtual void removeColumn( const std::string& /*name*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace structure is read-only. Cannot remove column."); }

    virtual void setRowCount(int /*count*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace structure is read-only. Cannot setRowCount"); }

    virtual int insertRow(int /*index*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace structure is read-only. Cannot insertRow"); }

    virtual void removeRow(int /*index*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace structure is read-only. Cannot removeRow."); }

    /// find method to get the index of integer cell value in a table workspace
    virtual void find(int /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }
    /// find method to get the index of  double cell value in a table workspace
    virtual void find (double /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }
    /// find method to get the index of  float cell value in a table workspace
    virtual void find(float /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }
    /// find method to get the index of  API::Boolean value cell in a table workspace
    virtual void find(API::Boolean /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }
    /// find method to get the index of cellstd::string  value in a table workspace
    virtual void find(std::string /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }
    /// find method to get the index of  Mantid::Geometry::V3D cell value in a table workspace
    virtual void find(Mantid::Geometry::V3D /*value*/,int& /*row*/,const int & /*col*/)
    { throw Mantid::Kernel::Exception::NotImplementedError("PeaksWorkspace::find() not implemented."); }


//
//    /**
//     * Initializes calibration values for instrument
//     *
//     * @param L1              Initial path length in m
//     * @parma time_offset     time offset in microseconds
//     * @param  Faciity        Facility name
//     * @param  Instrument     Instrument name
//     * @param  experimentDate Date of experiment
//     * @param version         version number for the peaks file format
//     * @param PanelNames      The names of the panels
//     * @param PanelInfo       panels information as follows:
//     *                        #rows,#cols,width(m),height(m),depth(m),detD(m),
//     *                        center x,y,z; base x,y,z; up x,y,z
//     *                        where x,y(vert),z(beam) is in m McStas coords
//     */
//    void initialize(const double      L1,              //cm
//        const double time_offset,     //seconds
//        const std::string Facility,
//        const std::string Instrument,
//        const Kernel::DateAndTime  experimentDate,
//        const std::string version,
//        const std::vector<std::string> &PanelNames,
//        const std::vector< double*> &PanelInfo);
//
//    /**
//     * Initializes calinration values for instrument from an Isaw-like DetCal file
//     * @param DetCalFileName   The name of the DetCal file.
//     */
//    void initialize(  const std::string DetCalFileName);
    //
    //     virtual ~PeaksWorkspace();
    //
    //     /**
    //      * Add a peak to the list of peaks
    //      *
    //      * @param position                  The position of the peak in meters and McStas Coordinates
    //      * @param time                      The time in microseconds of the peak
    //      * @param hkl                       The h,k, and l values for the peak. (0,0,0) if not indexed
    //      * @param sampe_orientation         The sample orientation in radians, using the NeXus convention
    //      * @param reflag                    The reflag for the status of various calculations for this peak.
    //      * @param runNum                    The run number associated with the peak
    //      * @param monCount                  The monitor count for the run
    //      * @param bankName                  The bankName as an int
    //      * @param PeakCellCount             The count for the intensity at middle pixel(detector)
    //      * @param row                       The row for the peak, if there is one, otherwise -1
    //      * @param col                       The column for the peak, if there is one, otherwise -1.
    //      * @param chan                      The time channel for the peak, if there is one
    //      * @param PeakIntegrateCount        The count for the intensity-background of all pixels(detectors) spanned by the peak
    //      * @param PeakIntegrateError        The error in the PeakIntegrateCount value.
    //      */
    //     void addPeak( const Geometry::V3D position,
    //                   const double time,
    //                   const Geometry::V3D hkl,
    //                   const Geometry::V3D sample_orientation,
    //                   const int  reflag,
    //                   const int  runNum,
    //                   const double  monCount,
    //                   const int bankName,
    //                   const double PeakCellCount,
    //                   const double row=0,
    //                   const double col=0,
    //                   const double chan=0,
    //                   const double PeakIntegrateCount=0,
    //                   const double PeakIntegrateError=0);
    //
    //     //column names are hkl,position,sample_orientation,reflag,time,L1,t_offset,
    //     //                  bank,run,ipeak,inti,sigi,moncount,det,row,col,chan.
    //
    //     /**
    //      * @return the d-spacing in Angstroms for the indicated peak
    //      */
    //     double  get_dspacing( const int peakNum);
    //
    //     /**
    //      * @return the wavelength in Angstroms of the indicated peak
    //      */
    //     double  get_wavelength( const int peakNum);
    //
    //     /**
    //      *   @return The magnitude of Q in inv Angstroms for the indicated peak. This magnitude = 1/d spacing
    //      */
    //     double  get_Qmagnitude( const int peakNum);
    //
    //     /**
    //      * @return The vector Q in inv Angstroms(magnitude =1/d spacing) of the indicated peak.
    //      *         This vector is at the given sample orientation
    //      */
    //     Geometry::V3D   get_Qlab( const int peakNum);
    //
    //     /**
    //      * @return The vector Q in inv Angstroms(magnitude =1/d spacing) of the indicated peak after
    //      *         adjusting for the sample orientation.
    //      *
    //      */
    //     Geometry::V3D   get_QXtal( const int peakNum);
    //
    //
    //     /**
    //      * @return the h,k, and l value for the given peak
    //      */
    //     Geometry::V3D   get_hkl( const int peakNum);
    //
    //     /**
    //      * @return the row of the given peak
    //      */
    //     double     get_row(const int peakNum) ;
    //
    //     /**
    //      * @return the column of the given peak
    //      */
    //     double     get_column( const int peakNum) ;
    //
    //     /**
    //      * @return the time channel of the given peak
    //      */
    //     double     get_time_channel( const int peakNum) ;
    //
    //     /**
    //      * @return the time offset of the given peak in microseconds
    //      */
    //     double  get_time_offset( const int peakNum ) ;
    //
    //     /**
    //      * Another name for getPeakCellCount
    //      * @return peak intensity at "center"
    //      */
    //     double  get_ipk( const int peakNum);
    //
    //     /**
    //      * @return the initial path length for the peak in m
    //      */
    //     double  get_L1(const int peakNum);
    //
    //     /**
    //      * @return the sample to peak length for the peak in m
    //      */
    //     double  get_L2(const int peakNum);
    //
    //
    //     /**
    //      * @return the bank number associated with the given peak
    //      */
    //     int    get_Bank(const int peakNum);
    //
    //     /**
    //      * @return the position of the given peak in m and in McStas coordinates
    //      */
    //     Geometry::V3D   getPosition(const int peakNum);
    //
    //     /**
    //      * @return the sample orientation of the given peak in radians
    //      */
    //     Geometry::V3D  getSampleOrientation( const int peakNum);
    //
    //     /**
    //      * @return the run number of the associated peak
    //      */
    //     int  getRunNumber( const int peakNum);
    //
    //     /**
    //      * @return the reflag( status flag) for the given peak
    //      */
    //     int getReflag( const int peakNum );
    //
    //     /**
    //      * @return the monitor count for the run associated with the given peak
    //      */
    //     double getMonitorCount(const int peakNum);
    //     /**
    //      * @return the intensity of the "middle" cell of the peak
    //      */
    //     double  getPeakCellCount( const int peakNum);
    //
    //     /**
    //      * @return the total intensity -background for all the cells associated with the peak
    //      */
    //     double  getPeakIntegrationCount( const int peakNum);
    //
    //     /**
    //      *  @return the error in the PeakIntegration count
    //      */
    //     double  getPeakIntegrationError( const int peakNum);
    //
    //     /**
    //      * @return the time of this peak in microseconds
    //      */
    //     double getTime( const int peakNum)
    //     {
    //       return cell<double>(peakNum, ItimeCol);
    //     }
    //
    //     /**
    //      * Sets the time( hence wavelength and d spacing) for this peak
    //      * @param newTime  the new time in microseconds
    //      * @param peakNum  the peak number starting at 0;
    //      */
    //     void setTime( const double newTime, const int peakNum)
    //     {
    //       cell<double>(peakNum, ItimeCol) = newTime;
    //     }
    //
    //     void setRowColChan( const double row, const double col,
    //                         const double chan, const int peakNum)
    //     {
    //       cell<double>(peakNum, IPeakRowCol)= row;
    //       cell<double>( peakNum, IPeakColCol )= col;
    //       cell<double>( peakNum, IPeakChanCol )= chan;
    //     }
    //
    //     /**
    //      * Sets the h,k, and l values for all peaks using the orientation matrix
    //      *
    //      * @param UB the orientation matrix that maps the column hkl vector to the column
    //      *            q vector where |q|=1/d-spacing and  x,y(up),z(beam) are in McStas coordinates
    //      *
    //      *@param tolerance  Only index peaks if the h,k, and l values are within tolerance of an integer
    //      *                   otherwise set to 0,0,0
    //      *@param SetOnlyUnset  if true, only peaks with h,k,l values of 0,0,0 will be set.
    //      *                     Used for twinned peaks.
    //      *
    //      *@param reflag  If SetOnlyUnset is true, gives the new reflag value for 2nd digit
    //      */
    //     void    sethkls( const Geometry::Matrix<double>UB, const double tolerance, const bool SetOnlyUnset, const int reflag);
    //
    //     /**
    //      * Sets all the h,k,l values to 0,0,0
    //      */
    //     void    clearhkls( );
    //
    //     /**
    //      *  sets the h,k,l value for the given peak
    //      */
    //     void    sethkl( const Geometry::V3D hkl, const int peakNum);
    //
    //
    //     /**
    //      * Sets the Peak Intensity count for the indicated peak
    //      */
    //     void    setPeakCount(const double count, const int peakNum);
    //
    //     /**
    //      *  Sets the total peak intensity-background for the indicated peak
    //      */
    //     void    setPeakIntegrateCount( const double count, const int peakNum);
    //
    //     /**
    //      * Sets the error in the PeakIntegrate Count
    //      */
    //     void    setPeakIntegrateError( const double count, const int peakNum);
    //
    //     /**
    //      * Sets the peak position( in m and McStas) for the indicated peak
    //      */
    //     void    setPeakPos( const Geometry::V3D position, const int peakNum);
    //
    //     /**
    //      * Sets the status flag for the indicated peak
    //      */
    //     void    setReflag( const int newValue, const int peakNum);
    //
    //     /**
    //      *  Writes the peak information to a peaks file
    //      */
    //     void    write( const std::string filename );
    //
    //     /**
    //      * Reads in new peaks from a Peaks file and appends it to the current peaks.
    //      * Use clearhkls to restart
    //      */
    //     void    append( const std::string filename);
    //
    //
    //
    //     /**
    //      * Removes all peaks
    //      */
    //     void    removeAllPeaks();
    //
    //
    //     bool   addColumn   (   const std::string &      /*type*/,
    //                     const std::string &   /* name*/
    //      ) {return false;}

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

    static const int ItimeCol =14;                 /** Column where the time of the peak is stored */
    static const int ItimeOffsetChanCol =15;  /** Column where the time offset of the peak is stored */

    static const int numColumns = 16;  /** Number of columns */


  };


/// Typedef for a shared pointer to a peaks workspace.
typedef boost::shared_ptr<PeaksWorkspace> PeaksWorkspace_sptr;

}
}
#endif





