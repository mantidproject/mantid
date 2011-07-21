#ifndef MANTID_ALGORITHMS_REBIN2D_H_
#define MANTID_ALGORITHMS_REBIN2D_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
  //------------------------------------------------------------------------------
  // Forward declarations
  //------------------------------------------------------------------------------    
  namespace Geometry
  {
    class ConvexPolygon;
  }

  namespace Algorithms
  {

    /** 
    Rebins both axes of a two-dimensional workspace to the given parameters.
    
    @author Martyn Gigg, Tessella plc
    
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
    class DLLExport Rebin2D  : public API::Algorithm
    {
    public:
      /// Algorithm's name for identification 
      virtual const std::string name() const { return "Rebin2D";}
      /// Algorithm's version for identification 
      virtual int version() const { return 1;}
      /// Algorithm's category for identification
      virtual const std::string category() const { return "Rebin";}
      
    private:
      /// A struct to store information about an intersection
      struct BinWithWeight
      {
        /** Constructor
         * @param i :: The index in the Y direction of the data bin
         * @param j :: The index in the X direction of the data bin
         * @param pointWeight :: The weight this point carries
         */
        BinWithWeight(const size_t i, const size_t j, const double pointWeight)
          : yIndex(i), xIndex(j), weight(pointWeight) {}
        /// The index in the Y direction of the data bin
        size_t yIndex;
        /// The index in the X direction of the data bin
        size_t xIndex;
        /// The weight this point carries
        double weight;            
      };

      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Initialise the properties
      void init();
      /// Run the algorithm
      void exec();
      /// Calculate the Y and E values for the given possible overlap
      std::pair<double,double> calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                                           const MantidVec & oldYBins,
                                           const Geometry::ConvexPolygon & outputPoly) const;
      /// Calculate the Y and E values from the given overlaps
      std::pair<double,double> calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                                           const std::vector<BinWithWeight> & overlaps) const;
      /// Calculate the Y and E values from the given overlaps for a distribution
      std::pair<double,double> calculateDistYE(API::MatrixWorkspace_const_sptr inputWS,
                                               const std::vector<BinWithWeight> & overlaps,
                                               const double newBinWidth) const;
      /// Find the overlap of the inputWS with the given polygon
      std::vector<BinWithWeight> findIntersections(const MantidVec & oldAxis1, 
                                                   const MantidVec & oldAxis2,
                                                   const Geometry::ConvexPolygon & poly) const;
      /// Setup the output workspace 
      API::MatrixWorkspace_sptr createOutputWorkspace(API::MatrixWorkspace_const_sptr parent,
                                                      MantidVec &newXBins, 
                                                      MantidVec &newYBins) const;

    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REBIN2D_H_ */
