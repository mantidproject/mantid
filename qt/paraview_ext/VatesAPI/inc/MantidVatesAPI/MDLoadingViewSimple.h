// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H
#define MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H

#include "MantidVatesAPI/MDLoadingView.h"

namespace Mantid {
namespace VATES {

/** MDLoadingViewSimple : Provides an almost hollow MDLoadingView
which is used by non-paraview based implementations such as the
SaveMDWorkspaceToVTK algorithm.
*/
class DLLExport MDLoadingViewSimple : public MDLoadingView {
public:
  double getTime() const override;
  void setTime(double time);

  size_t getRecursionDepth() const override;
  void setRecursionDepth(size_t recursionDepth);

  bool getLoadInMemory() const override;
  void setLoadInMemory(bool loadInMemory);

private:
  double m_time = 0.0;
  size_t m_recursionDepth = 5;
  bool m_loadInMemory = true;
};
} // namespace VATES
} // namespace Mantid

#endif
