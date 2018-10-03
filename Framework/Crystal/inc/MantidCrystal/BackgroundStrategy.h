// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BACKGROUNDSTRATEGY_H_
#define BACKGROUNDSTRATEGY_H_

namespace Mantid {
namespace API {
class IMDIterator;
}
namespace Crystal {
/** BackgroundStrategy : Abstract class used for identifying elements of a
 IMDWorkspace that are not considered background.
 */
class BackgroundStrategy {
public:
  virtual bool isBackground(Mantid::API::IMDIterator *const iterator) const = 0;
  virtual void
  configureIterator(Mantid::API::IMDIterator *const iterator) const = 0;
  virtual BackgroundStrategy *clone() const = 0;
  virtual ~BackgroundStrategy() = default;
};
} // namespace Crystal
} // namespace Mantid
#endif /* BACKGROUNDSTRATEGY_H_ */
