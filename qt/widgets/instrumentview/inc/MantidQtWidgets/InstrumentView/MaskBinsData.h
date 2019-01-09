// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MASKBINSDATA_H_
#define MASKBINSDATA_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <vector>

#include <QList>
#include <QMap>

namespace MantidQt {
namespace MantidWidgets {

/// Range of x values to mask in a spectrum. (Using MaskBins)
struct BinMask {
  BinMask(double s = 0.0, double e = 0.0) : start(s), end(e) {}
  double start;
  double end;
  std::vector<size_t> spectra;
};

/**
Class for storing information on masked bins in a workspace.
*/
class MaskBinsData {
public:
  void addXRange(double start, double end, const std::vector<size_t> &indices);
  void mask(const std::string &wsName) const;
  bool isEmpty() const;
  void subtractIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace,
                                 std::vector<double> &spectraIntgrs) const;
  void clear();
  /// Load the state of the bin masks from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the bin masks to a Mantid project file.
  std::string saveToProject() const;

private:
  QList<BinMask> m_masks;
  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MASKBINSDATA_H_*/
