#ifndef MANTID_ALGORITHM_WBVMEDIANTEST_H_
#define MANTID_ALGORITHM_WBVMEDIANTEST_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <Poco/NObserver.h>
#include <climits>
#include <cfloat>
#include <string>
#include <vector>

namespace Mantid
{
  namespace Algorithms
  {
    using API::MatrixWorkspace_sptr;
    using API::MatrixWorkspace_const_sptr;
    using API::SpectraDetectorMap;
    /**
    Takes a workspace as input and finds all the detectors with solid angle corrected signals
    that deviate far enough from median value of all detectors to be suspious.  The factors used
    to define detectors as reading too low or reading too high are selectable by setting the
    "Low" and "High" properties.  By default the median value is calculated using the entire
    spectrum but a region can be selected by setting startX and endX.  The output workspace
    contains one value for each spectrum in the input workspace, a value of 0 marks a spectrum
    with no problems and 100 a spectrum that failed.  The algorithm also returns an
    array with a list of the detector IDs asociated with failing spectra and if the OutputFile
    property is set the same list is written to that file.

    Required Properties:
    <UL>
    <LI> WhiteBeamWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> LowThreshold - Detectors with signals of this proportion of the median value, or less, will be labeled as reading low (default 0.1)</LI>
    <LI> HighThreshold - Detectors with signals this number of times, or more, than the median signal will be labeled as reading high (default 1.5)</LI>
    <LI> StartSpectrum - The index number of the first spectrum to include in the calculation (default 0)</LI>
    <LI> EndSpectrum - The index number of the last spectrum to include in the calculation (default the last histogram) </LI>
    <LI> RangeLower - Start the integration at the bin above the one that this value is in (default: the start of each spectrum)</LI>
    <LI> RangeUpper - Stop the integration at the bin before the one that contains this x value (default: the end of each spectrum)</LI>
    <LI> OutputFile - (Optional) A filename to which to write the list of dead detector UDETs </LI>
    </UL>

    @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
    @date 15/06/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
    class DLLExport MedianDetectorTest : public API::Algorithm
    {
    public:
      /// Default constructor initialises all values to zero and runs the base class constructor
      MedianDetectorTest() :
          API::Algorithm(),
          m_Low(0.1), m_High(1.5), m_MinSpec(0), m_MaxSpec(UNSETINT),
          m_usableMaskMap(true), m_fracDone(0.0), m_TotalTime(RTTotal)
      {};
      /// Destructor
      virtual ~MedianDetectorTest() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "MedianDetectorTest";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    private:
      /// A pointer to the input workspace
      MatrixWorkspace_const_sptr m_InputWS;
      /// The proportion of the median value below which a detector is considered under-reading
      double m_Low;
      /// The factor of the median value above which a detector is considered over-reading
      double m_High;
      ///The index of the first spectrum to calculate
      int m_MinSpec;
      /// The index of the last spectrum to calculate
      int m_MaxSpec;
      /// when this is set to false reading and writing to the detector map is disabled, this is done if there is no map in the workspace
      bool m_usableMaskMap;

      // Overridden Algorithm methods
      void init();
      void exec();
      
      // The different steps of the calculation, all called by exec()
      /// Loads and checks the values passed to the algorithm
      void retrieveProperties();
      /// Calculates the sum of soild angles of detectors for each histogram
      MatrixWorkspace_sptr getSolidAngles(int firstSpec, int lastSpec);
      /// Calculates the sum counts in each histogram
      MatrixWorkspace_sptr getTotalCounts(int firstSpec, int lastSpec);
      /// Converts numbers of particle counts into count rates
      MatrixWorkspace_sptr getRate(MatrixWorkspace_sptr counts);
      /// Finds the median of values in single bin histograms
      double getMedian(MatrixWorkspace_const_sptr input) const;
      /// Produces a workspace of single value histograms that indicate if the spectrum is within limits
      void FindDetects(MatrixWorkspace_sptr responses, const double baseNum, std::vector<int> &badDets, const std::string &filename);
      void createOutputArray(const std::vector<int> &lows, const std::vector<int> &highs, const SpectraDetectorMap &detMap, std::vector<int> &total) const;
      void writeFile(const std::string &fname, const std::vector<int> &lowList, const std::vector<int> &highList, const std::vector<int> &notFound);
      void logFinds(std::vector<int>::size_type missing, std::vector<int>::size_type low, std::vector<int>::size_type high, int alreadyMasked);

      /// the number of numbers on each line of the output file
      static const int LINESIZE = 10;
      ///a flag int value to indicate that the value wasn't set by users
      static const int UNSETINT = INT_MAX-15;

      //data and functions for the progress bar 
      /// An estimate of the percentage of the algorithm runtimes that has been completed 
      double m_fracDone;
      /// For the progress bar, estimates of how many additions, or equilivent, member functions will do for each spectrum
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
      /// An estimate total number of additions or equilivent required to compute a spectrum 
      int m_TotalTime;
      /// Update the fraction complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
      double advanceProgress(double toAdd);
      /// Update the fraction complete estimate assuming that the algorithm aborted a task with estimated RunTime toAdd
      void failProgress(RunTime aborted);
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_WBVMEDIANTEST_H_*/
