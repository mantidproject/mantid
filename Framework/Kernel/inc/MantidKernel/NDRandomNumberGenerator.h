// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {
/**
  This class defines an interface for N dimensional random number generators.
  A call to next produces N points in an ND space

  @author Martyn Gigg, Tessella plc
  @date 19/05/2012
*/
class MANTID_KERNEL_DLL NDRandomNumberGenerator {
public:
  /// Constructor
  NDRandomNumberGenerator(const unsigned int ndims);
  /// Virtual destructor to ensure that all inheriting classes have one
  virtual ~NDRandomNumberGenerator() = default;

  /// Disable default constructor
  NDRandomNumberGenerator() = delete;

  /// Disable copy operator
  NDRandomNumberGenerator(const NDRandomNumberGenerator &) = delete;

  /// Disable assignment operator
  NDRandomNumberGenerator &operator=(const NDRandomNumberGenerator &) = delete;

  /// Returns the number of dimensions the point will be generated in, i.e. the
  /// size
  /// of the vector returned from by nextPoint()
  inline unsigned int numberOfDimensions() const { return m_ndims; }
  /// Generate the next set of values that form a point in ND space
  const std::vector<double> &nextPoint();

  /// Restarts the generator from the beginning of the sequence
  virtual void restart() = 0;
  /// Saves the current state of the generator
  virtual void save() = 0;
  /// Restores the generator to the last saved point, or the beginning if
  /// nothing has been saved
  virtual void restore() = 0;

protected:
  /// Generate the next point. Override this in you concrete implementation
  virtual void generateNextPoint() = 0;

  /// Cache a value for a given dimension index, i.e. 0->ND-1
  void cacheGeneratedValue(const size_t index, const double value);
  /// Cache the while point in one go
  void cacheNextPoint(const std::vector<double> &nextPoint);
  /// Some generators need direct access to the cache
  inline std::vector<double> &getNextPointCache() { return m_nextPoint; }

private:
  /// The number of dimensions
  const unsigned int m_ndims;
  /// Storage the next point to return
  std::vector<double> m_nextPoint;
};
} // namespace Kernel
} // namespace Mantid
