// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MD_LOADING_VIEW_ADAPTER_H
#define MANTID_VATES_MD_LOADING_VIEW_ADAPTER_H

#include "MantidVatesAPI/MDLoadingView.h"

namespace Mantid {
namespace VATES {

/**
@class MDLoadingViewAdapter
Templated type for wrapping non-MDLoadingView types. Adapter pattern.
@author Owen Arnold, Tessella plc
@date 07/09/2011
*/
template <typename ViewType> class MDLoadingViewAdapter : public MDLoadingView {
private:
  ViewType *m_adaptee;

public:
  MDLoadingViewAdapter(ViewType *adaptee) : m_adaptee(adaptee) {}

  double getTime() const override { return m_adaptee->getTime(); }

  size_t getRecursionDepth() const override {
    return m_adaptee->getRecursionDepth();
  }

  bool getLoadInMemory() const override { return m_adaptee->getLoadInMemory(); }

  ~MDLoadingViewAdapter() override {
    // Do not delete adaptee.
  }
};
} // namespace VATES
} // namespace Mantid

#endif
