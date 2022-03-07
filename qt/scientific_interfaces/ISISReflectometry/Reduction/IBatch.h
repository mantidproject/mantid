// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "LookupRow.h"
#include "Slicing.h"

#include <boost/optional.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class Experiment;
class Instrument;
class Row;

class IBatch {
public:
  virtual ~IBatch() = default;

  virtual Experiment const &experiment() const = 0;
  virtual Instrument const &instrument() const = 0;
  virtual Slicing const &slicing() const = 0;

  virtual boost::optional<LookupRow> findWildcardLookupRow() const = 0;
  virtual boost::optional<LookupRow> findLookupRow(Row const &row) const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
