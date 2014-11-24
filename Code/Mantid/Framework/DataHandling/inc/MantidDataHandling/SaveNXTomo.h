#ifndef MANTID_DATAHANDLING_SAVENXTOMO_H_
#define MANTID_DATAHANDLING_SAVENXTOMO_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include <vector>
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"


namespace Mantid
{
  namespace DataHandling
  {

    /**
     * Saves a workspace into a NeXus/HDF5 NXTomo file.
     * File format is defined here: http://download.nexusformat.org/sphinx/classes/applications/NXtomo.html
     *
     * Required properties:
     * <ul>
     * <li> InputWorkspace - The workspace to save. </li>
     * <li> Filename - The filename for output </li>
     * </ul>
     *
     * @author John R Hill, RAL 
     * @date 10/09/2014
     *
     * This file is part of Mantid.
     *
     *   Mantid is free software; you can redistribute it and/or modify
     *   it under the terms of the GNU General Public License as published by
     *   the Free Software Foundation; either version 3 of the License, or
     *   (at your option) any later version.
     *
     *   Mantid is distributed in the hope that it will be useful,
     *   but WITHOUT ANY WARRANTY; without even the implied warranty of
     *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     *   GNU General Public License for more details.
     *
     *   You should have received a copy of the GNU General Public License
     *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
     *
     *   File change history is stored at: <https://github.com/mantidproject/mantid>
     *   Code Documentation is available at: <http://doxygen.mantidproject.org>
     *
     */

    class DLLExport SaveNXTomo: public API::Algorithm
    {
    public:
      SaveNXTomo();
      /// Virtual dtor
      virtual ~SaveNXTomo() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const {  return "SaveNXTomo"; }

      /// Summary of algorithms purpose
      virtual const std::string summary() const {return "Writes a MatrixWorkspace to a file in the NXTomo format.";}

      /// Algorithm's version
      virtual int version() const  {  return (1);  }

      /// Algorithm's category for identification
      virtual const std::string category() const { return "DataHandling\\Nexus;DataHandling\\Tomo;Diffraction";  }

      /// Run instead of exec when operating on groups
      bool processGroups();

    private:      
      /// Initialisation code
      void init();
      /// Execution code : Single workspace
      void exec();

      /// Creates the format for the output file if it doesn't exist
      ::NeXus::File setupFile(bool resizeData = false);

      /// Writes a single workspace into the file
      void writeSingle(const DataObjects::Workspace2D_sptr workspace, ::NeXus::File &nxFile);

      /// Main exec routine, called for group or individual workspace processing.
      void processAll();

      /// Fetch all rectangular Detector objects defined for an instrument
      std::vector<boost::shared_ptr<const Mantid::Geometry::RectangularDetector>> getRectangularDetectors(const Geometry::Instrument_const_sptr &instrument);

      /// Populate dims_array with the dimensions defined in the rectangular detector in the instrument
      std::vector<int64_t> getDimensionsFromDetector(const std::vector<boost::shared_ptr<const Mantid::Geometry::RectangularDetector>> &rectDetectors, size_t useDetectorIndex = 0);
           
      // Include error data in the written file
      bool m_includeError;
      bool m_overwriteFile;
      size_t m_spectraCount;
      std::vector<int64_t> m_slabStart;
      std::vector<int64_t> m_slabSize;
      /// The filename of the output file
      std::string m_filename;                     
      // Dimensions for axis in nxTomo file.
      std::vector<int64_t> m_dimensions;

      /// file format version
      static const std::string NXTOMO_VER;
    
      std::vector<DataObjects::Workspace2D_sptr> m_workspaces;    
    };

  } // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVENXTOMO_H_
