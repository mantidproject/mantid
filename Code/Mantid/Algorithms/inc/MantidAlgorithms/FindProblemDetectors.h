#ifndef MANTID_ALGORITHM_FINDPROBLEMDETECTORS_H_
#define MANTID_ALGORITHM_FINDPROBLEMDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/NObserver.h>
#include <climits>
#include <string>
#include <vector>

namespace Mantid
{
  namespace Algorithms
  {
    /**
    Takes a workspace as input and finds all the detectors with solid angle corrected signals
    that deviate far enough from median value of all detectors to be suspious.  The factors used
    to define detectors as reading too low or reading too high are selectable by setting the
    "Low" and "High" properties.  By default the median value is calculated using the entire
    spectrum but a region can be selected by setting startX and endX.  The algorithm returns an
    array with a list of detectors and produces a workspace where the ???????.  If the
    OutputFile property is set to a valid name the list of dead detector with be written to that
    file.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> Low - Detectors with signals of this proportion of the median value, or less, will be labeled as reading low (default 0.1)</LI>
    <LI> High - Detectors with signals this number of times, or more, than the median signal will be labeled as reading high (default 1.5)</LI>
    <LI> StartX - Start the integration at the bin above the one that this value is in (default: the start of each spectrum)</LI>
    <LI> EndX - Stop the integration at the bin before the one that contains this x value (default: the end of each spectrum)</LI>
    <LI> LiveValue - The value to assign to an integrated spectrum flagged as 'live' (default 0.0)</LI>
    <LI> DeadValue - The value to assign to an integrated spectrum flagged as 'dead' (default 100.0)</LI>
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
    class DLLExport FindProblemDetectors : public API::Algorithm
    {
    public:
      /// Default constructor initialised all values to zero and runs the base class constructor
      FindProblemDetectors() :
          API::Algorithm(),
          m_Low(0.1), m_High(1.5), m_MinSpec(0), m_MaxSpec(UNSETINT),
          m_PercentDone(0.0), m_TotalTime(RTTotal)
// STEVES Allow users to change the failure codes?  m_liveValue(???), double m_deadValue(???)
      {};
      /// Destructor
      virtual ~FindProblemDetectors() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FindProblemDetectors";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
      
      // The different steps of the calculation, all called by exec()
      void retrieveProperties();
      /// Uses the SolidAngle algorithm to get the sum of soild angles for the detectors that form each spectrum
      API::MatrixWorkspace_sptr getSolidAngles(
        API::MatrixWorkspace_sptr input, int firstSpec, int lastSpec );
      /// Uses the Integrate algorithm to sum counts in each histogram
      API::MatrixWorkspace_sptr getTotalCounts(
        API::MatrixWorkspace_sptr input, int firstSpec, int lastSpec );
      /// Uses the ConvertToDistribution algorithm divide the counts in each bin by time that counts were collected for
      API::MatrixWorkspace_sptr getRate(API::MatrixWorkspace_sptr counts);
      /// Checks though the solid angle information masking detectors with bad solid angle info
      void rejectZeros(API::MatrixWorkspace_const_sptr angles) const;

      /// Calculates the median number counts over all histograms
      double getMedian(API::MatrixWorkspace_const_sptr numbers) const;
      /// Produces a workspace of single value histograms where the value indicates if the spectrum is outside the limits or not
      std::vector<int> markDetects(API::MatrixWorkspace_sptr responses,
        double lowLim, double highLim, std::string fileName);
      
      /// Value written to the output workspace where bad spectra are found
      static const int BadVal = 100;
        /// Marks accepted spectra the output workspace
      static const int GoodVal = 0;
      ///a flag int value to indicate that the value wasn't set by users
      static const int UNSETINT = INT_MAX-15;

      //a lot of data and functions for the progress bar 
      /// An estimate of the percentage of the algorithm runtimes that has been completed 
      float m_PercentDone;
      /// For the progress bar, estimates of how many additions, or equilivent, member functions will do for each spectrum
      enum RunTime
      {
        /// An estimate of how much work SolidAngle will do for each spectrum
        RTGetSolidAngle = 15000,
        /// Estimate of the work required from Integtrate for each spectrum
        RTGetTotalCounts = 5000,
        /// Work required by the ConvertToDistribution algorithm
        RTGetRate = 100,
        /// Time taken to find failind detectors
        RTMarkDetects = 200,
        /// The total of all run times
        RTTotal = RTGetSolidAngle + RTGetTotalCounts + RTGetRate + RTMarkDetects
      };
      /// An estimate total number of additions or equilivent are require to compute a spectrum 
      int m_TotalTime;
      /// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
      float advanceProgress(int toAdd);
      /// Update the percentage complete estimate assuming that the algorithm aborted a task with estimated RunTime toAdd
      void failProgress(RunTime aborted);
    private:
      /// A pointer to the input workspace
      API::MatrixWorkspace_sptr m_InputWS;
      /// The proportion of the median value below which a detector is considered under-reading
      double m_Low;
      /// The factor of the median value above which a detector is considered over-reading
      double m_High;
      ///The index of the first spectrum to calculate
      int m_MinSpec;
      /// The index of the last spectrum to calculate
      int m_MaxSpec;

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FINDPROBLEMDETECTORS_H_*/
