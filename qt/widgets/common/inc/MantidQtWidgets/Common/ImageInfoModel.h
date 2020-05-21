// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/Workspace.h"
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModel {

public:
  /** Creates a list with information about the coordinates in the workspace.
  @param x: x data coordinate
  @param y: y data coordinate, for a matrix workspace this will be the spectrum
  number
  @param signal: the signal value at x,y
  @param includeValues: if false the returned list will contain the information
  headers with "-" for each of the values
  @return a vector containing pairs of strings
  */
  virtual std::vector<std::string> getInfoList(const double x, const double y,
                                               const double z,
                                               bool includeValues = true) = 0;

protected:
  void addNameAndValue(const std::string &label, const double value,
                       const int precision, std::vector<std::string> &list,
                       bool includeValues = true);
};

} // namespace MantidWidgets
} // namespace MantidQt
