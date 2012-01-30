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
  size_t PeaksWorkspace::getColumnIndex(const std::string& name) const
  {
    for (size_t i=0; i < columns.size(); i++)
      if (columns[i]->name() == name)
        return i;
    throw std::invalid_argument("Column named " + name + " was not found in the PeaksWorkspace.");
  }

  //---------------------------------------------------------------------------------------------
  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<Mantid::API::Column> PeaksWorkspace::getColumn(size_t index)
  {
    if (index >= static_cast<int>(columns.size())) throw std::invalid_argument("PeaksWorkspace::getColumn() called with invalid index.");
    return columns[index];
  }

  //---------------------------------------------------------------------------------------------
  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<const Mantid::API::Column> PeaksWorkspace::getColumn(size_t index) const
  {
    if (index >= static_cast<int>(columns.size())) throw std::invalid_argument("PeaksWorkspace::getColumn() called with invalid index.");
    return columns[index];
  }

  void PeaksWorkspace::saveNexus(::NeXus::File * file ) const
  {

    //Number of Peaks
    const size_t np(peaks.size());

    // Column vectors for peaks table
    std::vector<int> detectorID(np);
    std::vector<double> H(np);
    std::vector<double> K(np);
    std::vector<double> L(np);
    std::vector<double> intensity(np);
    std::vector<double> sigmaIntensity(np);
    std::vector<double> binCount(np);
    std::vector<double> initialEnergy(np);
    std::vector<double> finalEnergy(np);
    std::vector<double> waveLength(np);
    std::vector<double> scattering(np);
    std::vector<double> dSpacing(np);
    std::vector<double> TOF(np);
    std::vector<int> runNumber(np);
    std::vector<double> goniometerMatrix(9*np);

    // Populate column vectors from Peak Workspace
    for (size_t i=0; i < np; i++)
    {
      Peak p = peaks[i];
      detectorID[i] = p.getDetectorID();  
      H[i] = p.getH();
      K[i] = p.getK();
      L[i] = p.getL();
      intensity[i] = p.getIntensity();
      sigmaIntensity[i] = p.getSigmaIntensity();
      binCount[i] = p.getBinCount();
      initialEnergy[i] = p.getInitialEnergy();
      finalEnergy[i] = p.getFinalEnergy();
      waveLength[i] = p.getWavelength();
      scattering[i] = p.getScattering();
      dSpacing[i] = p.getDSpacing();
      TOF[i] = p.getTOF();
      runNumber[i] = p.getRunNumber();
      {
        Matrix<double> gm = p.getGoniometerMatrix();
        goniometerMatrix[9*i]   = gm[0][0];
        goniometerMatrix[9*i+1] = gm[1][0];
        goniometerMatrix[9*i+2] = gm[2][0];
        goniometerMatrix[9*i+3] = gm[0][1];
        goniometerMatrix[9*i+4] = gm[1][1];
        goniometerMatrix[9*i+5] = gm[2][1];
        goniometerMatrix[9*i+6] = gm[0][2];
        goniometerMatrix[9*i+7] = gm[1][2];
        goniometerMatrix[9*i+8] = gm[1][2];
      }
      // etc.
    }

    // Start Peaks Workspace in Nexus File
    std::string specifyInteger = "An integer";
    std::string specifyDouble = "A double";
    file->makeGroup("peaks_workspace", "NXentry", true);  // For when peaksWorkspace can be loaded 

    // Detectors column
    file->writeData("column_1", detectorID);
    file->openData("column_1");
    file->putAttr("name", "Dectector ID");
    file->putAttr("interpret_as", specifyInteger);
    file->putAttr("units","Not known");
    file->closeData();

    // H column
    file->writeData("column_2", H);
    file->openData("column_2");
    file->putAttr("name", "H");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // K column
    file->writeData("column_3", K);
    file->openData("column_3");
    file->putAttr("name", "K");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // L column
    file->writeData("column_4", L);
    file->openData("column_4");
    file->putAttr("name", "L");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Intensity column
    file->writeData("column_5", intensity);
    file->openData("column_5");
    file->putAttr("name", "Intensity");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Sigma Intensity column
    file->writeData("column_6", sigmaIntensity);
    file->openData("column_6");
    file->putAttr("name", "Sigma Intensity");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Bin Count column
    file->writeData("column_7", binCount);
    file->openData("column_7");
    file->putAttr("name", "Bin Count");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Initial Energy column
    file->writeData("column_8", initialEnergy );
    file->openData("column_8");
    file->putAttr("name", "Initial Energy");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Final Energy column
    file->writeData("column_9", finalEnergy );
    file->openData("column_9");
    file->putAttr("name", "Final Energy");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Wave Length Column
    file->writeData("column_10", waveLength );
    file->openData("column_10");
    file->putAttr("name", "Wave Length");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Scattering Column
    file->writeData("column_11", scattering );
    file->openData("column_11");
    file->putAttr("name", "Scattering");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // D Spacing Column
    file->writeData("column_12", dSpacing );
    file->openData("column_12");
    file->putAttr("name", "D Spacing");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // TOF Column
    file->writeData("column_13", TOF );
    file->openData("column_13");
    file->putAttr("name", "TOF");
    file->putAttr("interpret_as", specifyDouble);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    //Run Number column
    file->writeData("column_14", runNumber );
    file->openData("column_14");
    file->putAttr("name", "Run Number");
    file->putAttr("interpret_as", specifyInteger);
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    // Goniometer Matrix Column
    std::vector<int> array_dims;
    array_dims.push_back(static_cast<int>(peaks.size()));
    array_dims.push_back(9);
    file->writeData("column_15", goniometerMatrix, array_dims);
    file->openData("column_15");
    file->putAttr("name", "Goniometer Matrix");
    file->putAttr("interpret_as","A matrix of 3x3 doubles");
    file->putAttr("units","Not known");  // Units may need changing when known
    file->closeData();

    file->closeGroup(); // end of peaks workpace

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
