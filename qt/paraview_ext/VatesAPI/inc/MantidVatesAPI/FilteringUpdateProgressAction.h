// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PARAVIEWPROGRESSACTION_H_
#define PARAVIEWPROGRESSACTION_H_

#include "MantidKernel/System.h"
#include "MantidVatesAPI/ProgressAction.h"

/** Adapter for action specific to ParaView RebinningCutter filter. Handles
 progress actions raised by underlying Mantid Algorithms.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011
 */

namespace Mantid {
namespace VATES {

/// Template argument is the exact filter/source/reader providing the public
/// UpdateAlgorithmProgress method.
template <typename Filter>
class DLLExport FilterUpdateProgressAction : public ProgressAction {

public:
  FilterUpdateProgressAction(Filter *filter, const std::string &message)
      : m_filter(filter), m_message(message) {}
  FilterUpdateProgressAction &operator=(FilterUpdateProgressAction &) = delete;
  FilterUpdateProgressAction(FilterUpdateProgressAction &) = delete;
  void eventRaised(double progress) override {
    m_filter->updateAlgorithmProgress(progress, m_message);
  }

  ~FilterUpdateProgressAction() {}

private:
  Filter *m_filter;

  /// Message associated with the progress action
  std::string m_message;
};
} // namespace VATES
} // namespace Mantid

#endif /* PARAVIEWPROGRESSACTION_H_ */
