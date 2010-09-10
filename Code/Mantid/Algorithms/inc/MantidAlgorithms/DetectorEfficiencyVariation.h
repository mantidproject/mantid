#ifndef MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_
#define MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <climits>
#include <string>
#include <vector>

namespace Mantid
{
  namespace Algorithms
  {
    /**
    Required Properties:
    <UL>
    <LI> WhiteBeamBase - Name of a white beam vanadium workspace </LI>
    <LI> WhiteBeamCompare - Name of a matching second white beam vanadium run from the same instrument </LI>
    <LI> OutputWorkspace - Each histogram from the input workspace maps to a histogram in this workspace with one value that indicates if there was a dead detector </LI>
    <LI> Variation - Identify spectra whose total number of counts has changed by more than this factor of the median change between the two input workspaces </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> StartWorkspaceIndex - The index number of the first entry in the Workspace to include in the calculation </LI>
    <LI> EndWorkspaceIndex - The index number of the last entry in the Workspace to include in the calculation </LI>
    <LI> RangeLower - No bin with a boundary at an x value less than this will be included in the summation used to decide if a detector is 'bad' </LI>
    <LI> RangeUpper - No bin with a boundary at an x value higher than this value will be included in the summation used to decide if a detector is 'bad' </LI>
    <LI> OutputFile - The name of a file to write the list of dead detector UDETs </LI>
    <LI> BadDetectorIDs - Output array </LI>
    </UL>
    
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
    class DLLExport DetectorEfficiencyVariation : public API::Algorithm
    {
    public:
      /// Default constructor initialises all data members and runs the base class constructor
      DetectorEfficiencyVariation() : API::Algorithm(),                                   //call the base class constructor
          m_fracDone(0.0), m_TotalTime(RTTotal), m_usableMaskMap(true)
      {};
      /// Destructor
      virtual ~DetectorEfficiencyVariation() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "DetectorEfficiencyVariation";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
      
      // The different steps of the calculation, all called by exec()
      /// Loads and checks the values passed to the algorithm
      void retrieveProperties( API::MatrixWorkspace_sptr &whiteBeam1,
        API::MatrixWorkspace_sptr &whiteBeam2, double &vari,
        int &minSpec, int &maxSpec );
      /// Calculates the sum counts in each histogram
      API::MatrixWorkspace_sptr getTotalCounts(API::MatrixWorkspace_sptr input,
        int firstSpec, int lastSpec );
      /// Finds the median of values in single bin histograms
      double getMedian(API::MatrixWorkspace_const_sptr input) const;
      /// Overwrites the first workspace with bad spectrum information, also outputs an array and a file
      std::vector<int> findBad(API::MatrixWorkspace_sptr a, API::MatrixWorkspace_const_sptr b, const double average, double variation, const std::string &fileName);
      void writeFile(const std::string &fname, const std::vector<int> &badList, const std::vector<int> &problemIndices, const API::Axis * const SpecNums);

      /// the number of numbers on each line of the output file
      static const int LINESIZE = 10;

      //a lot of data and functions for the progress bar
      /// For the progress bar, estimates of how many additions (and equilivent) member functions will do for each spectrum assuming large spectra where progressing times are likely to be long
      enum RunTime
      {
        /// Estimate of the work required from Integtrate for each spectrum in _each_ workspace
        RTGetTotalCounts = 5000,
        /// Time taken to find failing detectors
        RTMarkDetects = 100,
        /// time estimate for writing the output file
        RTWriteFile = 200,
        /// The total of all run times
        RTTotal = 2*RTGetTotalCounts + RTMarkDetects
      };
      /// An estimate of the percentage of the algorithm runtimes that has been completed 
      double m_fracDone;
      /// An estimate total number of additions or equilivent are require to compute a spectrum 
      int m_TotalTime;
      /// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
      double advanceProgress(double toAdd);
    private:
      /// when this is set to false reading and writing to the detector map is disabled, this is done if there is no map in the workspace
      bool m_usableMaskMap;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_*/
