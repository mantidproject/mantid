// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
class QString;

namespace MantidQt {
namespace MantidWidgets {
class IAlgorithmProgressWidget {
public:
  IAlgorithmProgressWidget() = default;
  virtual ~IAlgorithmProgressWidget() = default;

  virtual void algorithmStarted() = 0;
  virtual void algorithmEnded() = 0;
  virtual void updateProgress(const double progress, const QString &message, const double estimatedTime,
                              const int progressPrecision) = 0;
  virtual void showDetailsDialog() = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
