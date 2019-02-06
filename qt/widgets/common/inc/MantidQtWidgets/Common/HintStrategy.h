// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_HINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_HINTSTRATEGY_H

#include "DllOption.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
/** HintStrategy : Provides an interface for generating hints to be used by a
HintingLineEdit.
*/
class EXPORT_OPT_MANTIDQT_COMMON HintStrategy {
public:
  HintStrategy(){};
  virtual ~HintStrategy() = default;

  /** Create a list of hints for auto completion.
      This implementation does nothing as it is intended to be overwritten.
      However, if we make this an abstract class we cannot easily add the method
      on the python side, as in basic_hint_strategy.py

      @returns A map of keywords to short descriptions for the keyword.
   */
  virtual std::vector<Hint> createHints() {
    return std::vector<Hint>();
  };
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_HINTSTRATEGY_H */
