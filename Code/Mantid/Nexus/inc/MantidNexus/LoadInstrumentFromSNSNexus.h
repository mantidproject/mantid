#ifndef MANTID_NEXUS_LOADINSTRUMENTFROMSNSNEXUS_H_
#define MANTID_NEXUS_LOADINSTRUMENTFROMSNSNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{

  namespace Geometry
  {
    class CompAssembly;
    class Component;
    class Instrument;
  }

  namespace NeXus
  {
    /** @class LoadInstrumentFromSNSNexus LoadInstrumentFromSNSNexus.h Nexus/LoadInstrumentFromSNSNexus.h

    Attempt to load information about the instrument from a ISIS nexus file. In particular attempt to
    read L2 and 2-theta detector position values and add detectors which are positioned relative
    to the sample in spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also adds dummy source
    and samplepos components to instrument.
    As this information appears to be absent from the existing nexus Muon sample files, it seems that
    little to this can be done at present. The new version of Muon nexus files may be more useful.

    LoadInstrumentFromSNSNexus is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.
    LoadInstrumentFromSNSNexus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NEXUS file </LI>
    <LI> Workspace - The name of the workspace in which to use as a basis for any data to be added.</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL (LoadInstrumentFromRaw)
    @date 2/5/2008
    @author Ronald Fowler, ISIS, RAL (LoadInstrumentFromNexus)

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadInstrumentFromSNSNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadInstrumentFromSNSNexus();

      /// Destructor
      virtual ~LoadInstrumentFromSNSNexus() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadInstrumentFromSNSNexus";};

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus\\Instrument";}

    private:

      /// Overwrites Algorithm method. Does nothing at present
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;


      /// Class for comparing bank names in format "bank123" according to the numeric part of the name
      class CompareBanks
      {
      public:
          /** Compare operator
          *  @param s1 First argument
          *  @param s2 Second argument
          *  @return true if s1<s2
          */
          bool operator()(const std::string& s1, const std::string& s2)
          {
              int i1 = atoi( s1.substr(4,s1.size()-4).c_str() );
              int i2 = atoi( s2.substr(4,s2.size()-4).c_str() );
              return i1 < i2;
          }
      };

      /// The l1
      double m_L1;

      /// load the instrument
      void loadInstrument(API::Workspace_sptr localWS,
                          NXEntry entry);

      /// Get bank's position and orientation
      void getBankOrientation(NXDetector nxDet, Geometry::V3D& shift, Geometry::Quat& rot);

      /// Calculate rotation axis from direction cosines
      void calcRotation(const Geometry::V3D& X,const Geometry::V3D& Y,const Geometry::V3D& Z,double& angle, Geometry::V3D& axis);

      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);
    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADINSTRUMENTFROMSNSNEXUS_H_*/

