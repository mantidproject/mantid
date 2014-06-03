#ifndef MANTID_ALGORITHMS_STITCH1D_H_
#define MANTID_ALGORITHMS_STITCH1D_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <boost/tuple/tuple.hpp>

namespace Mantid
{
  namespace Algorithms
  {

    /** Stitch1D : Stitches two Matrix Workspaces together into a single output.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Stitch1D  : public API::Algorithm
    {
    public:
      /// Default constructor
      Stitch1D(){};
      /// Destructor
      virtual ~Stitch1D(){};
      /// Algorithm's name for identification. @see Algorithm::name
      virtual const std::string name() const {return "Stitch1D";}
      /// Algorithm's version for identification. @see Algorithm::version
      virtual int version() const {return 4;}
      /// Algorithm's category for identification. @see Algorithm::category
      virtual const std::string category() const {return "Reflectometry";}
      ///Summary of algorithm's purpose
      virtual const std::string summary() const {return "Stitches single histogram matrix workspaces together";}

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method.
      void exec();
      /**Gets the start of the overlapping region
      @param intesectionMin :: The minimum possible value for the overlapping region to inhabit
      @param intesectionMax :: The maximum possible value for the overlapping region to inhabit
      @return a double contianing the start of the overlapping region
      */
      double getStartOverlap(const double& intesectionMin, const double& intesectionMax) const;
      /**Gets the end of the overlapping region
      @param intesectionMin :: The minimum possible value for the overlapping region to inhabit
      @param intesectionMax :: The maximum possible value for the overlapping region to inhabit
      @return a double contianing the end of the overlapping region
      */
      double getEndOverlap(const double& intesectionMin, const double& intesectionMax) const;
      /**Determines if a workspace has non zero errors
      @param ws :: The input workspace
      @return True if there are any non-zero errors in the workspace
      */
      bool hasNonzeroErrors(Mantid::API::MatrixWorkspace_sptr& ws) const;
      /**Gets the rebinning parameters and calculates any missing values
      @param lhsWS :: The left hand side input workspace
      @param rhsWS :: The right hand side input workspace
      @return a vector<double> contianing the rebinning parameters
      */
      Mantid::MantidVec getRebinParams(Mantid::API::MatrixWorkspace_sptr& lhsWS, Mantid::API::MatrixWorkspace_sptr& rhsWS) const;
      /**Runs the Rebin Algorithm as a child
      @param input :: The input workspace
      @param params :: a vector<double> containing rebinning parameters
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr rebin(Mantid::API::MatrixWorkspace_sptr& input, const Mantid::MantidVec& params);
      /**Runs the Integration Algorithm as a child
      @param input :: The input workspace
      @param start :: a double defining the start of the region to integrate
      @param stop :: a double defining the end of the region to integrate
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr integration(Mantid::API::MatrixWorkspace_sptr& input, const double& start, const double& stop);
      /**Runs the MultiplyRange Algorithm as a child defining an end bin
      @param input :: The input workspace
      @param startBin :: The first bin int eh range to multiply
      @param endBin :: The last bin in the range to multiply
      @param factor :: The multiplication factor
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr multiplyRange(Mantid::API::MatrixWorkspace_sptr& input, const int& startBin, const int& endBin, const double& factor);
      /**Runs the MultiplyRange Algorithm as a child
      @param input :: The input workspace
      @param startBin :: The first bin int eh range to multiply
      @param factor :: The multiplication factor
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr multiplyRange(Mantid::API::MatrixWorkspace_sptr& input, const int& startBin, const double& factor);
      /**Runs the CreateSingleValuedWorkspace Algorithm as a child
      @param val :: The double to convert to a single value workspace
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr singleValueWS(double val);
      /**Runs the WeightedMean Algorithm as a child
      @param inOne :: The first input workspace
      @param inTwo :: The second input workspace
      @return A shared pointer to the resulting MatrixWorkspace
      */
      Mantid::API::MatrixWorkspace_sptr weightedMean(Mantid::API::MatrixWorkspace_sptr& inOne, Mantid::API::MatrixWorkspace_sptr& inTwo);
      /**finds the bins containing the ends of the overlappign region
      @param startOverlap :: The start of the overlapping region
      @param endOverlap :: The end of the overlapping region
      @param workspace :: The workspace to determine the overlaps inside
      @return a boost::tuple<int,int> containing the bin indexes of the overlaps
      */
      boost::tuple<int,int> findStartEndIndexes(double startOverlap, double endOverlap, Mantid::API::MatrixWorkspace_sptr& workspace);
      ///the range tollerence constant to apply to overlap values
      static const double range_tolerance;

    };


  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_STITCH1D_H_ */
