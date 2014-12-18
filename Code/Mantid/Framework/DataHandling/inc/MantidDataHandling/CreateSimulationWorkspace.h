#ifndef MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACE_H_
#define MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid
{
  namespace DataHandling
  {

    /**

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport CreateSimulationWorkspace : public API::Algorithm
    {
    public:

      virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Create a blank workspace for a given instrument.";}

      virtual int version() const;
      virtual const std::string category() const;

    private:
      void init();
      void exec();

      /// Create the instrument
      void createInstrument();
      /// Creates the output workspace
      void createOutputWorkspace();
      /// Creates the detector grouping list
      size_t createDetectorMapping();
      /// Create a one to one mapping from the spectrum numbers to detector IDs
      void createOneToOneMapping();
      /// Load the detector mapping from a file
      void loadMappingFromFile(const std::string & filename);
      /// Load the detector mapping from a RAW file
      void loadMappingFromRAW(const std::string & filename);
      /// Load the detector mapping from a NXS file
      void loadMappingFromISISNXS(const std::string & filename);
      /// Create the grouping map from the tables
      void createGroupingsFromTables(int *specTable, int *udetTable, int ndets);
      /// Returns new Xbins
      MantidVecPtr createBinBoundaries() const;
      /// Apply the created mapping to the workspace
      void applyDetectorMapping();
      /// Apply any instrument adjustments from the file
      void adjustInstrument(const std::string & filename);

      /// Pointer to a progress object
      boost::shared_ptr<API::Progress> m_progress;
      /// Pointer to the new instrument
      Geometry::Instrument_const_sptr m_instrument;
      /// Pointer to the new workspace
      API::MatrixWorkspace_sptr m_outputWS;
      /// List of detector groupings
      std::map<specid_t, std::set<detid_t> > m_detGroups;
    };


  } // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_CREATESIMULATIONWORKSPACE_H_ */
