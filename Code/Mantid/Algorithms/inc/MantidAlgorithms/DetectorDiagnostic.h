#ifndef MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_
#define MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <set>

namespace Mantid
{
  namespace Algorithms
  {
    /** 
      A base class for a detector diagnostic algorithm. It has not exec implemenation but provides functions
      that are common among these algorithms such as calculating the median and writing to a file.
      
      @author Martyn Gigg, Tessella plc
      @date 2010-12-09
      
      Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class DLLExport DetectorDiagnostic : public API::Algorithm
    {
    public:
      /// Default constructor
      DetectorDiagnostic();
      /// Virtual destructor
      virtual ~DetectorDiagnostic() {};

      /// Algorithm's category for identification
      virtual const std::string category() const { return "Diagnostics";}

    protected:
      /// Get the total counts for each spectra
      API::MatrixWorkspace_sptr integrateSpectra(API::MatrixWorkspace_sptr inputWS, const int indexMin,
						 const int indexMax, const double lower,
						 const double upper);
      /// Calculate the median of the given workspace. This assumes that the input workspace contains 
      /// integrated counts
      double calculateMedian(API::MatrixWorkspace_sptr workspace, std::set<int> & skippedIndices);

      /** @name Progress reporting */
      //@{
      /// For the progress bar, estimates of how many additions, 
      /// or equilivent, member functions will do for each spectrum
      enum RunTime
      {
        /// An estimate of how much work SolidAngle will do for each spectrum
        RTGetSolidAngle = 15000,
        /// Estimate of the work required from Integtrate for each spectrum
        RTGetTotalCounts = 5000,
        /// Work required by the ConvertToDistribution algorithm
        RTGetRate = 100,
        /// Time taken to find failing detectors
        RTMarkDetects = 200,
        /// Time taken to find failing detectors
        RTWriteFile = 200,
        /// The total of all run times
        RTTotal = RTGetSolidAngle + RTGetTotalCounts + RTGetRate + RTMarkDetects + RTWriteFile
      };

      /// Update the fraction complete estimate assuming that the algorithm has completed 
      /// a task with estimated RunTime toAdd
      double advanceProgress(double toAdd);
      /// Update the fraction complete estimate assuming that the algorithm 
      /// aborted a task with estimated RunTime toAdd
      void failProgress(RunTime aborted);

      /// An estimate of the percentage of the algorithm runtimes that has been completed 
      double m_fracDone;
      /// An estimate total number of additions or equilivent required to compute a spectrum 
      int m_TotalTime;

      //@}

      
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_*/
