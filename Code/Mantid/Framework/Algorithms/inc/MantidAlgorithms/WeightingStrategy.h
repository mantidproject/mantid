#ifndef MANTID_ALGORITHMS_WEIGHTINGSTRATEGY_H_
#define MANTID_ALGORITHMS_WEIGHTINGSTRATEGY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
namespace Algorithms
{

  /** WeightingStrategy : 
  
    Abstract weighting strategy, which can be applied to calculate individual 
    weights for each pixel based upon disance from epicenter. Generated for use with SmoothNeighbours.
    
    @date 2011-11-30

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport WeightingStrategy
    {
    public:
      /// Constructor
      WeightingStrategy(const double cutOff);
      /// Constructor
      WeightingStrategy();
      /// Destructor
      virtual ~WeightingStrategy();
      /**
      Calculate the weight at distance from epicenter.
      @param distance : difference between the central detector location and the nearest neighbour
      @return calculated weight
      */
      virtual double weightAt(const Mantid::Kernel::V3D& distance) = 0;

      /**
      Calculate the weight at distance from epicenter.
      @param adjX : The number of Y (vertical) adjacent pixels to average together
      @param ix : current index x
      @param adjY : The number of X (vertical) adjacent pixels to average together
      @param iy : current index y
      */
      virtual double weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy) = 0;
    protected:
      /// Cutoff member.
      double m_cutOff;
    };

    /*
    Flat (no weighting) strategy. Concrete WeightingStrategy
    */
    class DLLExport FlatWeighting : public WeightingStrategy
    {
    public:
      FlatWeighting();
      virtual ~FlatWeighting();
      virtual double weightAt(const double&,const double&, const double&, const double&);
      double weightAt(const Mantid::Kernel::V3D& );
    };

    /*
    Linear weighting strategy.
    */
    class DLLExport LinearWeighting : public WeightingStrategy
    {
    public: 
      LinearWeighting(const double cutOff);
      virtual ~LinearWeighting();
      double weightAt(const Mantid::Kernel::V3D& );
      virtual double weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy);
    };

    /*
    Parabolic weighting strategy.
    */
    class DLLExport ParabolicWeighting : public WeightingStrategy
    {
    public: 
      ParabolicWeighting(const double cutOff);
      virtual ~ParabolicWeighting();
      double weightAt(const Mantid::Kernel::V3D& );
      virtual double weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy);
    };

    /*
    Null weighting strategy.
    */
    class DLLExport NullWeighting : public WeightingStrategy
    {
    public:
      NullWeighting();
      virtual ~NullWeighting();
      double weightAt(const Mantid::Kernel::V3D& );
      virtual double weightAt(const double&,const double&, const double&, const double&);
    };

    /*
    Gaussian nD Strategy. 

    y = exp(-0.5*((r./p(1)).^2) where p = sqtr(2)*sigma
    */
    class DLLExport GaussianWeightingnD : public WeightingStrategy
    {
    public:
      GaussianWeightingnD(double cutOff, double sigma);
      virtual ~GaussianWeightingnD();
      virtual double weightAt(const Mantid::Kernel::V3D& );
      virtual double weightAt(const double&,const double&, const double&, const double&);
    private:
      double calculateGaussian(const double normalisedDistanceSq);
      double m_twiceSigmaSquared;
    };

} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_WEIGHTINGSTRATEGY_H_ */
