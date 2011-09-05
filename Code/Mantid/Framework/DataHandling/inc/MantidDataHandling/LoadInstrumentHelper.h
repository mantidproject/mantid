#ifndef MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include <boost/shared_ptr.hpp>

//----------------------------------------------------------------------
// Poco Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco 
{
  namespace XML 
  {
    class Element;
  }
}
/// @endcond

namespace Mantid
{
  //----------------------------------------------------------------------
  // Mantid Forward declaration
  //----------------------------------------------------------------------
  namespace Geometry
  {
    class ISpectraDetectorMap;
    class Instrument;
    class IComponent;
  }
  namespace Kernel
  {
    class V3D;
  }
  namespace API
  {
    class MatrixWorkspace;
  }  

  namespace DataHandling
  {

    /** Stripped down vector that holds position in terms of spherical coordinates,
      *  Needed when processing instrument definition files that use the 'Ariel format'
      */
    struct SphVec
    {
      ///@cond Exclude from doxygen documentation
      double r,theta,phi;
      SphVec() : r(0.0), theta(0.0), phi(0.0) {}
      SphVec(const double& r, const double& theta, const double& phi) : r(r), theta(theta), phi(phi) {}
      ///@endcond
    };

    /** @class LoadInstrumentHelper LoadInstrumentHelper.h DataHandling/LoadInstrumentHelper.h

    Contains method for assisting the loading of an instrument definition file (IDF)

    @author Anders Markvardsen, ISIS, RAL
    @date 29/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadInstrumentHelper 
    {
    public:
      /// Default constructor
      LoadInstrumentHelper() {};

      /// Destructor
      virtual ~LoadInstrumentHelper() 
      {
        // Don't need this anymore (if it was even used) so empty it out to save memory
        //m_tempPosHolder.clear();
      }

      /// Return from an IDF the values of the valid-from and valid-to attributes
      static void getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom,
                                 std::string& outValidTo);
      /// Return full path of IDF given instrument name (e.g. GEM) and a date
      static std::string getInstrumentFilename(const std::string& instName, const std::string& date);
      /// Return workspace start date as an ISO 8601 string
      static std::string getWorkspaceStartDate(const boost::shared_ptr<API::MatrixWorkspace>& workspace);

      /// Set location (position) of comp as specified in XML location element
      static void setLocation(Geometry::IComponent* comp, Poco::XML::Element* pElem, const double angleConvertConst,
                              const bool deltaOffsets=false);

      /// Calculate the position of comp relative to its parent from info provided by \<location\> element
      static Kernel::V3D getRelativeTranslation(const Geometry::IComponent* comp, const Poco::XML::Element* pElem,
                                     const double angleConvertConst, const bool deltaOffsets=false);

      /// Get parent component element of location element
      static Poco::XML::Element* getParentComponent(Poco::XML::Element* pLocElem);

    private:
      /// static reference to the logger class
      static Kernel::Logger& g_log;

    public:
      /// Map to store positions of parent components in spherical coordinates
      static std::map<const Geometry::IComponent*,SphVec> m_tempPosHolder;
    };


  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENTHELPER_H_*/

