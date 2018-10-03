// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IFMDITERATOR_H_
#define MANTID_API_IFMDITERATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"
#include <vector>

namespace Mantid {

namespace API {

/** Enum describing different ways to normalize the signal
 * in a MDWorkspace.
 */
enum MDNormalization {
  /// Don't normalize = return raw counts
  NoNormalization = 0,
  /// Divide the signal by the volume of the box/bin
  VolumeNormalization = 1,
  /// Divide the signal by the number of events that contributed to it.
  NumEventsNormalization = 2
};

/** This is an interface to an iterator of an IMDWorkspace

    @author Roman Tolchenov, Tessella Support Services plc
    @date 15/03/2011
*/
class MANTID_API_DLL IMDIterator {
public:
  IMDIterator();
  virtual ~IMDIterator() = default;

  void setNormalization(Mantid::API::MDNormalization normalization);
  Mantid::API::MDNormalization getNormalization() const;

  /// Get the size of the data (number of entries that will be iterated through)
  virtual size_t getDataSize() const = 0;

  /// Advance to the next cell. If the current cell is the last one in the
  /// workspace
  /// do nothing and return false.
  virtual bool next() = 0;

  /// Is the current position of the iterator valid?
  virtual bool valid() const = 0;

  /// Jump to the index^th cell.
  virtual void jumpTo(size_t index) = 0;

  /// Advance, skipping a certain number of cells.
  virtual bool next(size_t skip) = 0;

  /// Returns the normalized signal for this box
  virtual signal_t getNormalizedSignal() const = 0;

  /// Returns the normalized error for this box
  virtual signal_t getNormalizedError() const = 0;

  /// Returns the total signal for this box
  virtual signal_t getSignal() const = 0;

  /// Returns the total error for this box
  virtual signal_t getError() const = 0;

  /// Return a list of vertexes defining the volume pointed to
  virtual std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices) const = 0;

  /// Return a list of vertexes defining the volume pointed to, enable masking
  /// of dimensions.
  virtual std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices, const size_t outDimensions,
                   const bool *maskDim) const = 0;

  /// Returns the position of the center of the box pointed to.
  virtual Mantid::Kernel::VMD getCenter() const = 0;

  /// Returns the number of events/points contained in this box
  virtual size_t getNumEvents() const = 0;

  /// For a given event/point in this box, return the run index
  virtual uint16_t getInnerRunIndex(size_t index) const = 0;

  /// For a given event/point in this box, return the detector ID
  virtual int32_t getInnerDetectorID(size_t index) const = 0;

  /// Returns the position of a given event for a given dimension
  virtual coord_t getInnerPosition(size_t index, size_t dimension) const = 0;

  /// Returns the signal of a given event
  virtual signal_t getInnerSignal(size_t index) const = 0;

  /// Returns the error of a given event
  virtual signal_t getInnerError(size_t index) const = 0;

  /// Returns true if masking is used.
  virtual bool getIsMasked() const = 0;

  /// Find neighbouring indexes vertex touching.
  virtual std::vector<size_t> findNeighbourIndexes() const = 0;

  /// Find neighbouring indexes face touching.
  virtual std::vector<size_t> findNeighbourIndexesFaceTouching() const = 0;

  /// Get the linear index.
  virtual size_t getLinearIndex() const = 0;

  /// Is index reachable by the iterator.
  virtual bool isWithinBounds(size_t index) const = 0;

protected:
  /// Normalization method for getNormalizedSignal()
  Mantid::API::MDNormalization m_normalization;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFMDITERATOR_H_*/
