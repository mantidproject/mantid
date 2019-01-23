// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERFACTORY_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERFACTORY_H

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class GenericDataProcessorPresenterFactory

GenericDataProcessorPresenterFactory provides a common interface to
concrete factories creating a GenericDataProcessorPresenter.
*/
class GenericDataProcessorPresenterFactory {
public:
  /**
  Constructor
  */
  GenericDataProcessorPresenterFactory() = default;
  virtual ~GenericDataProcessorPresenterFactory() = default;
  /**
  Creates a GenericDataProcessorPresenter
  */
  virtual std::unique_ptr<GenericDataProcessorPresenter> create() = 0;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERFACTORY_H*/
