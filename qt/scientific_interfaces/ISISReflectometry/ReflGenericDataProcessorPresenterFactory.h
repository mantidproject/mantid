// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H

#include "DllConfig.h"
#include "ReflDataProcessorPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflGenericDataProcessorPresenterFactory

ReflGenericDataProcessorPresenterFactory creates a Reflectometry
GenericDataProcessorPresenter
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflGenericDataProcessorPresenterFactory {
public:
  ReflGenericDataProcessorPresenterFactory() = default;
  virtual ~ReflGenericDataProcessorPresenterFactory() = default;

  /**
   * Creates a Reflectometry Data Processor Presenter
   */
  std::unique_ptr<ReflDataProcessorPresenter> create(int group);
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /*MANTID_ISISREFLECTOMETRY_REFLGENERICDATAPROCESSORPRESENTERFACTORY_H*/
