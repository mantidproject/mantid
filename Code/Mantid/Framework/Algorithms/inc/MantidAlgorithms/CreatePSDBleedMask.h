#ifndef MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_
#define MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
     
      This algorithm implements a "bleed" diagnostic for PSD detectors (i.e. long tube-based detectors).
      Required Properties:
      <UL>
      <LI> InputWorkspace  - The name of the input workspace. </LI>
      <LI> OutputMaskWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
      <LI> MaxTubeRate - The maximum rate allowed for a tube. </LI>
      <LI> NIgnoredCentralPixels - The number of pixels about the centre to ignore. </LI>
      </UL>

      @author Martyn Gigg, Tessella plc
      @date 2011-01-10
      
      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class DLLExport CreatePSDBleedMask : public DetectorDiagnostic
    {
    public:
      /// Default constructor
      CreatePSDBleedMask();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CreatePSDBleedMask";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// Process a tube
      bool performBleedTest(const std::vector<int> & tubeIndices,
			    API::MatrixWorkspace_const_sptr inputWS);
      /// Mask a tube with the given workspace indices
      void maskTube(const std::vector<int> & tubeIndices, API::MatrixWorkspace_sptr workspace);
      /// Mark a tubes data values as passing the tests
      void markAsPassed(const std::vector<int> & tubeIndices, API::MatrixWorkspace_sptr workspace);

      /// Maximum allowed rate
      double m_maxRate;
      /// Number of ignored pixels
      int m_numIgnoredPixels;
      /// Is the input a distribution or raw counts. If true then bin width division is necessary when calculating the rate
      bool m_isRawCounts;
    };

  } // namespace Algorithms
} // namespace Mantid

#endif //MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_
