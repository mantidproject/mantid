#ifndef MANTID_DATAHANDLING_LOADDETECTORINFO_H_
#define MANTID_DATAHANDLING_LOADDETECTORINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**

    Copyright &copy; 2008-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport LoadDetectorInfo : public API::Algorithm
    {
    public:
      LoadDetectorInfo();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadDetectorInfo"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads delay times, tube pressures, tube wall thicknesses and, if necessary, the detectors positions from a given special format file";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const{return "DataHandling";}

    private:

      /// Simple data holder for passing the detector info around when
      /// dealing with the NeXus data
      struct DetectorInfo
      {
        std::vector<detid_t> ids;
        std::vector<int32_t> codes;
        std::vector<double> delays;
        std::vector<double> l2, theta, phi;
        std::vector<double> pressures, thicknesses;
      };

      void init();
      void exec();

      /// Cache the user input that will be frequently accessed
      void cacheInputs();
      /// Use a .dat or .sca file as input
      void loadFromDAT(const std::string & filename);
      /// Use a .raw file as input
      void loadFromRAW(const std::string & filename);
      /// Use a isis raw nexus or event file as input
      void loadFromIsisNXS(const std::string & filename);
      /// Read data from old-style libisis NeXus file
      void readLibisisNxs(::NeXus::File & nxsFile, DetectorInfo & detInfo) const;
      /// Read data from old-style libisis NeXus file
      void readNXSDotDat(::NeXus::File & nxsFile, DetectorInfo & detInfo) const;

      /// Update the parameter map with the new values for the given detector
      void updateParameterMap(Geometry::ParameterMap & pmap, const Geometry::IDetector_const_sptr & det,
                              const double l2, const double theta, const double phi,
                              const double delay, const double pressure, const double thickness) const;

      /// Cached instrument for this workspace
      Geometry::Instrument_const_sptr m_baseInstrument;
      /// Cached sample position for this workspace
      Kernel::V3D m_samplePos;
      /// If set to true then update the detector positions base on the information in the given file
      bool m_moveDets;
      /// store a pointer to the user selected workspace
      API::MatrixWorkspace_sptr m_workspace;

      /// Delta value that has been set on the instrument
      double m_instDelta;
      /// Pressure value set on the instrument level
      double m_instPressure;
      /// Wall thickness value set on the instrument level
      double m_instThickness;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADDETECTORINFO_H_*/
