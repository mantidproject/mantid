// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_REFLDATAPROCESSORMAINPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_REFLDATAPROCESSORMAINPRESENTER_H

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflDataProcessorMainPresenter
*/
class ReflDataProcessorMainPresenter {
public:
  virtual ~ReflDataProcessorMainPresenter(){};

  /// Return true if event analysis has to be performed
  virtual bool eventAnalysis() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTIDQTMANTIDWIDGETS_REFLDATAPROCESSORMAINPRESENTER_H */
