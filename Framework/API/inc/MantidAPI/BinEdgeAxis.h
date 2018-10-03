// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_BINEDGEAXIS_H_
#define MANTID_API_BINEDGEAXIS_H_

#include "MantidAPI/NumericAxis.h"

namespace Mantid {
namespace API {

/**
Stores numeric values that are assumed to be bin edge values.

It overrides indexOfValue to search using the values as bin edges are than
centre points
*/
class MANTID_API_DLL BinEdgeAxis : public NumericAxis {
public:
  BinEdgeAxis(const std::size_t &length);
  BinEdgeAxis(const std::vector<double> &edges);

  Axis *clone(const MatrixWorkspace *const parentWorkspace) override;
  Axis *clone(const std::size_t length,
              const MatrixWorkspace *const parentWorkspace) override;

  std::vector<double> createBinBoundaries() const override;
  void setValue(const std::size_t &index, const double &value) override;
  size_t indexOfValue(const double value) const override;

private:
  /// Private, undefined copy assignment operator
  const BinEdgeAxis &operator=(const BinEdgeAxis &);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_BINEDGEAXIS_H_ */
