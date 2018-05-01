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
                Mantid::Geometry::MDImplicitFunction *function = nullptr);
  MDBoxIterator(API::IMDNode *topBox, size_t maxDepth, bool leafOnly,
                SkippingPolicy *skippingPolicy,
                Mantid::Geometry::MDImplicitFunction *function = nullptr);
  MDBoxIterator(std::vector<API::IMDNode *> &boxes, size_t begin, size_t end);
  void init(std::vector<API::IMDNode *> &boxes, size_t begin, size_t end);
  ~MDBoxIterator() override;

  /// Return a pointer to the current box pointed to by the iterator.
  MDBoxBase<MDE, nd> *getBox() const { return (m_current); }

  /// ------------ IMDIterator Methods ------------------------------
  size_t getDataSize() const override;

  bool valid() const override;

  void jumpTo(size_t index) override;

  bool next() override;

  bool next(size_t skip) override;

  signal_t getNormalizedSignal() const override;

  signal_t getNormalizedError() const override;

  signal_t getSignal() const override;

  signal_t getError() const override;

  std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices, const size_t outDimensions,
                   const bool *maskDim) const override;

  std::unique_ptr<coord_t[]>
  getVertexesArray(size_t &numVertices) const override;

  Mantid::Kernel::VMD getCenter() const override;

  size_t getNumEvents() const override;

  uint16_t getInnerRunIndex(size_t index) const override;

  int32_t getInnerDetectorID(size_t index) const override;

  coord_t getInnerPosition(size_t index, size_t dimension) const override;

  signal_t getInnerSignal(size_t index) const override;

  signal_t getInnerError(size_t index) const override;

  bool getIsMasked() const override;

  /// Getter for the position of the iterator.
  size_t getPosition() const { return m_pos; }

  std::vector<size_t> findNeighbourIndexes() const override;

  std::vector<size_t> findNeighbourIndexesFaceTouching() const override;

  size_t getLinearIndex() const override;

  bool isWithinBounds(size_t index) const override;

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
