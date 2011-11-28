#ifndef MANTID_ALGORITHMS_SmoothNeighbours_H_
#define MANTID_ALGORITHMS_SmoothNeighbours_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace Algorithms
  {
    /*
    Abstract weighting strategy, which can be applied to calculate individual 
    weights for each pixel based upon disance from epicenter.
    */
    class DLLExport WeightingStrategy
    {
    public:
      /// Constructor
      WeightingStrategy(double& cutOff) : m_cutOff(cutOff){};
      /// Constructor
      WeightingStrategy() : m_cutOff(0){};
      /// Destructor
      virtual ~WeightingStrategy(){};
      /**
      Calculate the weight at distance from epicenter.
      @param distance : absolute distance from epicenter
      @return calculated weight
      */
      virtual double weightAt(double& distance) = 0;

      /**
      Calculate the weight at distance from epicenter.
      @param adjX : The number of Y (vertical) adjacent pixels to average together
      @param ix : current index x
      @param adjY : The number of X (vertical) adjacent pixels to average together
      @param iy : current index y
      */
      virtual double weightAt(int& adjX, int& ix, int& adjY, int& iy) = 0;
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
      FlatWeighting() : WeightingStrategy(){}
      virtual ~FlatWeighting(){};
      virtual double weightAt(int&, int&, int&, int&){return 1;}
      double weightAt(double&){ return 1;}
    };

    /*
    Linear weighting strategy.
    */
    class DLLExport LinearWeighting : public WeightingStrategy
    {
    public: 
      LinearWeighting(double &cutOff) : WeightingStrategy(cutOff){}
      virtual ~LinearWeighting(){};
      double weightAt(double& distance)
      {
        return 1 - (distance/m_cutOff);
      }
      virtual double weightAt(int& adjX, int& ix, int& adjY, int& iy)
      {
        return static_cast<double>(adjX - std::abs(ix) + adjY - std::abs(iy) + 1);
      }
    };

    /*
    Null weighting strategy.
    */
    class DLLExport NullWeighting : public WeightingStrategy
    {
    public:
      NullWeighting() : WeightingStrategy(){}
      virtual ~NullWeighting(){};
      double weightAt(double&)
      {
        throw std::runtime_error("NullWeighting strategy cannot be used to evaluate weights.");
      }
      virtual double weightAt(int&, int&, int&, int&)
      {
        throw std::runtime_error("NullWeighting strategy cannot be used to evaluate weights.");
      }
    };

  /** Smooth neighboring pixels.

    @authors Janik Zikovsky, Vickie Lynch, SNS
    @date Oct 2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport SmoothNeighbours : public API::Algorithm
{
public:
  /// Default constructor
  SmoothNeighbours();
  /// Destructor
  virtual ~SmoothNeighbours() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SmoothNeighbours";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  void execWorkspace2D(Mantid::API::MatrixWorkspace_sptr ws);
  void execEvent(Mantid::DataObjects::EventWorkspace_sptr ws);

  void findNeighboursRectangular();
  void findNeighboursUbiqutious();

  /// Sets the weighting stragegy.
  void setWeightingStrategy(const std::string strategyName, double& cutOff);

  /// Pixels in the detector
  int XPixels;
  /// Pixels in the detector
  int YPixels;

  /// Number to sum
  int AdjX;
  /// Number to sum
  int AdjY;
  /// Edge pixels to ignore
  int Edge;
  /// Radius to search nearest neighbours
  double Radius;
  /// Weight the neighbours during summing
  boost::scoped_ptr<WeightingStrategy> WeightedSum;
  /// PreserveEvents
  bool PreserveEvents;

  /// Input workspace
  Mantid::API::MatrixWorkspace_sptr inWS;

  /// Each neighbours is specified as a pair with workspace index, weight.
  typedef std::pair<size_t, double> weightedNeighbour;

  /// Vector of list of neighbours (with weight) for each workspace index.
  std::vector< std::vector< weightedNeighbour > > m_neighbours;

  /// Progress reporter
  Mantid::API::Progress * m_prog;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SmoothNeighbours_H_*/
