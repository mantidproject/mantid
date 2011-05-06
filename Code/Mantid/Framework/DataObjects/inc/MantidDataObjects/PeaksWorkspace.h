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

    //---------------------------------------------------------------------------------------------
    /** Removes the indicated peak
     * @param peakNum  the peak to remove. peakNum starts at 0
     */
    void removePeak(const int peakNum)
    {
      if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0 ) throw std::invalid_argument("PeaksWorkspace::removePeak(): peakNum is out of range.");
      peaks.erase(peaks.begin()+peakNum);
    }

    //---------------------------------------------------------------------------------------------
    /** Add a peak to the list
     * @param peak :: Peak object to add (copy) into this.
     */
    void addPeak(const Peak peak)
    {
      peaks.push_back(peak);
    }

    //---------------------------------------------------------------------------------------------
    /** Return a reference to the Peak
     * @param peakNum :: index of the peak to get.
     * @return a reference to a Peak object.
     */
    Peak & getPeak(const int peakNum)
    {
      if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0 ) throw std::invalid_argument("PeaksWorkspace::getPeak(): peakNum is out of range.");
      return peaks[peakNum];
    }

    //---------------------------------------------------------------------------------------------
    /** Return a reference to the Peaks vector */
    std::vector<Peak> & getPeaks()
    {
      return peaks;
    }

    //---------------------------------------------------------------------------------------------
    /// Number of columns in the workspace.
    virtual int columnCount() const
    {
      return static_cast<int>(columns.size());
    }

    //---------------------------------------------------------------------------------------------
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


    //---------------------------------------------------------------------------------------------
    /// Returns a vector of all column names.
    virtual std::vector<std::string> getColumnNames()
    {
      return this->columnNames;
    }

    //---------------------------------------------------------------------------------------------
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

    /** Column shared pointers. */
    std::vector<boost::shared_ptr<Mantid::DataObjects::PeakColumn> > columns;

    /** Column names */
    std::vector<std::string> columnNames;


  public:

    virtual bool addColumn(const std::string& /*type*/, const std::string& /*name*/);

    // ===== Methods that are not implemented (read-only table) ==========

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

  };


  /// Typedef for a shared pointer to a peaks workspace.
  typedef boost::shared_ptr<PeaksWorkspace> PeaksWorkspace_sptr;

  /// Typedef for a shared pointer to a const peaks workspace.
  typedef boost::shared_ptr<const PeaksWorkspace> PeaksWorkspace_const_sptr;

}
}
#endif





