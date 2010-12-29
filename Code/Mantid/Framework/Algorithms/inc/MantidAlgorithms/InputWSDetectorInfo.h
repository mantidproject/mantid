#ifndef MANTID_ALGORITHM_INPUTWSDETECTORINFO_H_
#define MANTID_ALGORITHM_INPUTWSDETECTORINFO_H_

#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    Provides functions to get detector lists for a spectrum and check and
    change detector masking.  Pass a shared pointer to the constructor
    when you make the object and then all of the functions act on a
    spectra and its detectors within that workspace.

    @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
    @date 15/06/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    
    */

    /// Functionality to read detector masking
    /** Inteprets the instrument and detector map for a
    * workspace making it possible to see the status of a detector
    * with one function call
    */
    class InputWSDetectorInfo
    {
    public:
      explicit InputWSDetectorInfo(API::MatrixWorkspace_const_sptr input);
      bool aDetecIsMaskedinSpec(int SpecIndex) const;
      void maskAllDetectorsInSpec(int SpecIndex);
      int getSpecNum(int SpecIndex) const;
      std::vector<int> getDetectors(int SpecIndex) const;
    protected:
      /// a pointer the workspace with the detector information
      const API::MatrixWorkspace_const_sptr m_Input;
      /// following the example MaskDetectors write to this version of the instrument
      boost::shared_ptr<Geometry::Instrument> m_WInstru;
      /// when we read need to read from here
      Geometry::IInstrument_sptr m_RInstru;
      /// pointer to the map that links detectors to their masking with the input workspace
      Geometry::ParameterMap* m_Pmap;
    };
  }
}

#endif /*MANTID_ALGORITHM_INPUTWSDETECTORINFO_H*/
