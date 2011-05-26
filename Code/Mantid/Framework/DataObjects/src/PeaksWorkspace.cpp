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
