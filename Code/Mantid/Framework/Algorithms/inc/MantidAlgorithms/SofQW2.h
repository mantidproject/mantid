#ifndef MANTID_ALGORITHMS_SOFQW2_H_
#define MANTID_ALGORITHMS_SOFQW2_H_
/*WIKI* 

Converts a 2D workspace from units of spectrum number/energy transfer to  the intensity as a function of momentum transfer and energy. The rebinning is done as a weighted  sum of overlapping polygons.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQW.h" 
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/IDetector.h"
#include <list>

namespace Mantid
{
  namespace Algorithms
  {

    /**
    Converts a 2D workspace that has axes of energy transfer against spectrum number to 
    one that gives intensity as a function of momentum transfer against energy. This version
    uses proper parallelpiped rebinning to compute the overlap of the various overlapping
    weights

    Required Properties:
    <UL>
    <LI> InputWorkspace  - Reduced data in units of energy transfer. Must have common bins. </LI>
    <LI> OutputWorkspace - The name to use for the q-w workspace. </LI>
    <LI> QAxisBinning    - The bin parameters to use for the q axis. </LI>
    <LI> Emode           - The energy mode (direct or indirect geometry). </LI>
    <LI> Efixed          - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV). </LI>
    </UL>

    @author Martyn Giggg
    @date 2011-07-15

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
    class DLLExport SofQW2  : public SofQW
    {
    public:
      /// Default constructor
      SofQW2();
      /// Algorithm's name for identification 
      virtual const std::string name() const { return "SofQW2"; }
      /// Algorithm's version for identification 
      virtual int version() const { return 1; };
      /// Algorithm's category for identification
      virtual const std::string category() const { return "Inelastic";}

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
      /// Run the algorithm
      void exec();
      /// Calculate the Y and E values for the given possible overlap
      std::pair<double,double> calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                                           const Geometry::ConvexPolygon & outputPoly) const;
      /// Calculate the Y and E values from the given overlaps
      std::pair<double,double> calculateYE(API::MatrixWorkspace_const_sptr inputWS,
                                           const std::vector<BinWithWeight> & overlaps) const;
      /// Calculate the Y and E values from the given overlaps for a distribution
      std::pair<double,double> calculateDistYE(API::MatrixWorkspace_const_sptr inputWS,
                                               const std::vector<BinWithWeight> & overlaps,
                                               const double newBinWidth) const;
      /// Find the overlap of the inputWS with the given polygon
      std::vector<BinWithWeight> findIntersections(API::MatrixWorkspace_const_sptr inputWS,
                                                   const Geometry::ConvexPolygon & poly) const;
      /// Init variables cache base on the given workspace
      void initCachedValues(API::MatrixWorkspace_const_sptr workspace);
      /// Init the theta index
      void initQCache(API::MatrixWorkspace_const_sptr workspace);
      /// Q value struct
      struct QValues
      {
	QValues() : lowerLeft(0.0), lowerRight(0.0), 
		    upperRight(0.0), upperLeft(0.0) {}
	QValues(const double lLeft, const double lRight,
		const double uRight, const double uLeft) 
	  : lowerLeft(lLeft), lowerRight(lRight), 
	    upperRight(uRight), upperLeft(uLeft) {}
	
	/// Q values
	double lowerLeft, lowerRight, upperRight, upperLeft;
      };
      /// Calculate the corner Q values
      QValues calculateQValues(Geometry::IDetector_const_sptr det,
			       const double dEMin, const double dEMax) const;
      /// Calculate the Kf vectors
      std::pair<Kernel::V3D, Kernel::V3D> calculateScatterDir(Geometry::IDetector_const_sptr det) const;
      /// Calculate a single Q value
      double calculateQ(const Kernel::V3D scatterDir, const double energy) const;
      /// E Mode
      int m_emode;
      /// EFixed
      double m_efixed;
      /// Beam direction
      Kernel::V3D m_beamDir;
      /// Sample position
      Kernel::V3D m_samplePos;
      /// Progress reporter
      boost::shared_ptr<API::Progress> m_progress;
      /// Small caching struct
      struct QRangeCache
      {
        QRangeCache(const size_t index, const double wght)
          : wsIndex(index), weight(wght), qValues(0) {}
        /// workspace index origin
        const size_t wsIndex;
        /// Weight
        const double weight;
	/// QValues for each energy bin
	std::vector<QValues> qValues;
      };
      /// The cache
      std::list<QRangeCache> m_qcached;
    };


  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SOFQW2_H_ */
