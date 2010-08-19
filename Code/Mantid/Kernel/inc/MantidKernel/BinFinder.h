#ifndef BINFINDER_H
#define BINFINDER_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <vector>
#include <functional>
#include <cmath>

namespace Mantid
{
namespace Kernel
{

  /**
   * The BinFinder class is a helper class that allows one to quickly
   * find a bin index for many events.
   *
   * The class is constructed by giving it rebinning parameters. This sets
   * it up to be called repeatedly later to return the bin index.
   *
   * Only works for linear and logarithmic binning - not arbitrary.
   *
   * Does work for consecutive bins of different steps, or mixing lin and log binning.
   */
  class DLLExport BinFinder
  {
  public:
    BinFinder(const std::vector<double>& binParams);

    ~BinFinder();

    int bin(double value);

  private:
    /// Boundaries between binning regions
    std::vector<double> boundaries;
    /// Step sizes in binning regions; 1 smaller than boundaries
    std::vector<double> stepSizes;
    /// Log of the step size (used by log binning)
    std::vector<double> logSteps;
    /// Log of the boundary (used by log binning)
    std::vector<double> logBoundaries;
    //How many regions?
    int numRegions;


  };

} // namespace Kernel
} // namespace Mantid

#endif /*BINFINDER_H*/
