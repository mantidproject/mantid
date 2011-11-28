#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
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
  PeaksWorkspace::PeaksWorkspace()
  : IPeaksWorkspace()
  {
    initColumns();
  }

  //---------------------------------------------------------------------------------------------
  /** Virtual constructor. Clone method to duplicate the peaks workspace.
   *
   * @return PeaksWorkspace object
   */
  PeaksWorkspace* PeaksWorkspace::clone() const
  {
    //Deep copy via copy construtor.
    return new PeaksWorkspace(*this);
  }


  //---------------------------------------------------------------------------------------------
  /** Copy constructor
   *
   * @param other :: other PeaksWorkspace to copy from
   * @return
   */
  PeaksWorkspace::PeaksWorkspace(const PeaksWorkspace & other)
  : IPeaksWorkspace(other),
    peaks(other.peaks)
  {
    initColumns();
    this->peaks = other.peaks;
  }

  //---------------------------------------------------------------------------------------------
  /** Clone a shared pointer
   *
   * @param other :: other PeaksWorkspace to copy from
   * @return copy of the peaksworkspace
   */
  boost::shared_ptr<PeaksWorkspace> PeaksWorkspace::clone()
  {
    // Copy construct and return
    return boost::shared_ptr<PeaksWorkspace>(new PeaksWorkspace(*this));
  }

  //---------------------------------------------------------------------------------------------
  /** Initialize all columns */
  void PeaksWorkspace::initColumns()
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
    addColumn( "str", "BankName");
    addColumn( "double", "Row");
    addColumn( "double", "Col");
    addColumn( "V3D", "QLab");
    addColumn( "V3D", "QSample");
  }

  //=====================================================================================
  //=====================================================================================
  /** Comparator class for sorting peaks by one or more criteria
   */
  class PeakComparator : public std::binary_function<Peak,Peak,bool>
  {
  public:
    std::vector< std::pair<std::string, bool> > & criteria;

    /** Constructor for the comparator for sorting peaks
    * @param criteria : a vector with a list of pairs: column name, bool;
    *        where bool = true for ascending, false for descending sort.
    */
    PeakComparator(std::vector< std::pair<std::string, bool> > & criteria)
    : criteria(criteria)
    {
    }

    /** Compare two peaks using the stored criteria */
    inline bool operator()(const Peak& a, const Peak& b)
    {
      for (size_t i = 0; i < criteria.size(); i++)
      {
        std::string & col = criteria[i].first;
        bool ascending = criteria[i].second;
        bool lessThan = false;
        if (col == "BankName")
        {
          // If this criterion is equal, move on to the next one
          std::string valA = a.getBankName();
          std::string valB = b.getBankName();
          // Move on to lesser criterion if equal
          if (valA == valB)
            continue;
          lessThan = (valA < valB);
        }
        else
        {
          // General double comparison
          double valA = a.getValueByColName(col);
          double valB = b.getValueByColName(col);
          // Move on to lesser criterion if equal
          if (valA == valB)
            continue;
          lessThan = (valA < valB);
        }
        // Flip the sign of comparison if descending.
        if (ascending)
          return lessThan;
        else
          return !lessThan;

      }
      // If you reach here, all criteria were ==; so not <, so return false
      return false;
    }
  };


  //---------------------------------------------------------------------------------------------
  /** Sort the peaks by one or more criteria
   *
   * @param criteria : a vector with a list of pairs: column name, bool;
   *        where bool = true for ascending, false for descending sort.
   *        The peaks are sorted by the first criterion first, then the 2nd if equal, etc.
   */
  void PeaksWorkspace::sort(std::vector< std::pair<std::string, bool> > & criteria)
  {
    PeakComparator comparator(criteria);
    std::stable_sort(peaks.begin(), peaks.end(), comparator);
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
  int PeaksWorkspace::getColumnIndex(const std::string& name) const
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

  //---------------------------------------------------------------------------------------------
  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<const Mantid::API::Column> PeaksWorkspace::getColumn(int index) const
  {
    if (index >= static_cast<int>(columns.size())) throw std::invalid_argument("PeaksWorkspace::getColumn() called with invalid index.");
    return columns[index];
  }

  void PeaksWorkspace::saveNexus(::NeXus::File * file ) const
  {
	  // This is not fully implemented. Currently a partial save.
	  int pNum = getNumberPeaks();

//	  //std::vector<Peak>& myPeaks = getPeaks();
//      std::vector<Peak> my1Peaks = peaks;
//	  // Peak myPeak = getPeak(0);
//	  Peak my1Peak = peaks[0];
//	  int cNum = columnCount();
//	  int rNum = rowCount();
//	  API::Column_const_sptr col = getColumn(0);
//	  std::vector<std::string> cNames = columnNames;
//  file->writeData("Column_names", columnNames ); // This is not resolved by linker
//  file->writeData("column names",cNames); // This is not resolved by linker.
//  file->writeData( "Column_1", col ); // This is not resolved by linker.

    std::vector<int> detectorID;
    std::vector<dpuble> H;
	  for (size_t i=0; i < peaks.size(); i++)
	  {
	    Peak & p = peaks[i];
      detectorID.push_back( p.m_DetectorID );
      H.push_back( p.m_H );
      // etc.
	  }
    file->writeData("detector_id", detectorIDs);
    file->writeData("H", H);
    // etc.
  }
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
