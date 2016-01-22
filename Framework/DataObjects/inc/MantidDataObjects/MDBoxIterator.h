#ifndef MANTID_DATAOBJECTS_MDBOXITERATOR_H_
#define MANTID_DATAOBJECTS_MDBOXITERATOR_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidDataObjects/SkippingPolicy.h"

namespace Mantid {
namespace DataObjects {
// Forward declaration.
class SkippingPolicy;

/** MDBoxIterator: iterate through MDBoxBase
 * hierarchy down to a given maximum depth.
 *
 * @author Janik Zikovsky
 * @date 2011-06-03
 */
TMDE_CLASS
class DLLExport MDBoxIterator : public Mantid::API::IMDIterator {
public:
  MDBoxIterator(API::IMDNode *topBox, size_t maxDepth, bool leafOnly,
                Mantid::Geometry::MDImplicitFunction *function = NULL);
  MDBoxIterator(API::IMDNode *topBox, size_t maxDepth, bool leafOnly,
                SkippingPolicy *skippingPolicy,
                Mantid::Geometry::MDImplicitFunction *function = NULL);
  MDBoxIterator(std::vector<API::IMDNode *> &boxes, size_t begin, size_t end);
  void init(std::vector<API::IMDNode *> &boxes, size_t begin, size_t end);
  virtual ~MDBoxIterator();

  /// Return a pointer to the current box pointed to by the iterator.
  MDBoxBase<MDE, nd> *getBox() const { return (m_current); }

  /// ------------ IMDIterator Methods ------------------------------
  size_t getDataSize() const;

  bool valid() const;

  void jumpTo(size_t index);

  bool next();

  bool next(size_t skip);

  signal_t getNormalizedSignal() const;

  signal_t getNormalizedError() const;

  signal_t getSignal() const;

  signal_t getError() const;

  coord_t *getVertexesArray(size_t &numVertices, const size_t outDimensions,
                            const bool *maskDim) const;

  coord_t *getVertexesArray(size_t &numVertices) const;

  Mantid::Kernel::VMD getCenter() const;

  size_t getNumEvents() const;

  uint16_t getInnerRunIndex(size_t index) const;

  int32_t getInnerDetectorID(size_t index) const;

  coord_t getInnerPosition(size_t index, size_t dimension) const;

  signal_t getInnerSignal(size_t index) const;

  signal_t getInnerError(size_t index) const;

  bool getIsMasked() const;

  /// Getter for the position of the iterator.
  size_t getPosition() const { return m_pos; }

  virtual std::vector<size_t> findNeighbourIndexes() const;

  virtual std::vector<size_t> findNeighbourIndexesFaceTouching() const;

  virtual size_t getLinearIndex() const;

  virtual bool isWithinBounds(size_t index) const;

private:
  /// Common code run my a few of the constructors.
  void commonConstruct(API::IMDNode *topBox, size_t maxDepth, bool leafOnly,
                       Mantid::Geometry::MDImplicitFunction *function);

  void getEvents() const;

  void releaseEvents() const;

  /// Current position in the vector of boxes
  size_t m_pos;

  /// Max pos = length of the boxes vector.
  size_t m_max;

  /// Vector of all the boxes that will be iterated.
  std::vector<API::IMDNode *> m_boxes;

  /// Box currently pointed to
  MDBoxBase<MDE, nd> *m_current;

  /// MDBox currently pointed to
  mutable MDBox<MDE, nd> *m_currentMDBox;

  /// Pointer to the const events vector. Only initialized when needed.
  mutable const std::vector<MDE> *m_events;

  // Skipping policy, controlls recursive calls to next().
  SkippingPolicy_scptr m_skippingPolicy;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_MDBOXITERATOR_H_ */
