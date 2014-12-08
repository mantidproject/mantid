#ifndef MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_
#define MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/IComponent.h"

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
      
      Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
    class DLLExport DetectorDiagnostic : public API::Algorithm
    {
    public:
      /// Default constructor
      DetectorDiagnostic();
      /// Virtual destructor
      virtual ~DetectorDiagnostic() {};

      /// Algorithm's category for identification
      virtual const std::string category() const;
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Identifies histograms and their detectors that have total numbers of counts over a user defined maximum or less than the user define minimum.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const;
      

    private:
      // Overridden Algorithm methods
      virtual void init();
      virtual void exec();
      /// Apply a given mask
      void applyMask(API::MatrixWorkspace_sptr inputWS, API::MatrixWorkspace_sptr maskWS);
      /// Perform checks on detector vanadium
      API::MatrixWorkspace_sptr doDetVanTest(API::MatrixWorkspace_sptr inputWS, int & nFails);

    protected:
      /// Get the total counts for each spectra
      API::MatrixWorkspace_sptr integrateSpectra(API::MatrixWorkspace_sptr inputWS, const int indexMin,
          const int indexMax, const double lower,
          const double upper, const bool outputWorkspace2D = false);

      DataObjects::MaskWorkspace_sptr generateEmptyMask(API::MatrixWorkspace_const_sptr inputWS);

      /// Calculate the median of the given workspace. This assumes that the input workspace contains 
      /// integrated counts
      std::vector<double> calculateMedian(API::MatrixWorkspace_sptr input, bool excludeZeroes,std::vector<std::vector<size_t> > indexmap);
      /// Convert to a distribution
      API::MatrixWorkspace_sptr convertToRate(API::MatrixWorkspace_sptr workspace);
      /// method to check which spectra should be grouped when calculating the median
      std::vector<std::vector<size_t> >makeMap(API::MatrixWorkspace_sptr countsWS);
      /// method to create the map with all spectra
      std::vector<std::vector<size_t> >makeInstrumentMap(API::MatrixWorkspace_sptr countsWS);

      /** @name Progress reporting */
      //@{
      /// For the progress bar, estimates of how many additions, 
      /// or equivalent, member functions will do for each spectrum
      enum RunTime
      {
        /// An estimate of how much work SolidAngle will do for each spectrum
        RTGetSolidAngle = 15000,
        /// Estimate of the work required from Integrate for each spectrum
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

      /// number of parents up, 0 go to instrument
      int m_parents;
      /// The number of tests to be run
      double m_progStepWidth;
      /// Starting workspace index to run tests on
      int m_minIndex;
      /// Ending workspace index to run tests on
      int m_maxIndex;
      /// Starting x-axis value for integrations
      double m_rangeLower;
      /// Ending x-axis value for integrations
      double m_rangeUpper;
      //@}

      
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_DETECTORDIAGNOSTIC_H_*/
