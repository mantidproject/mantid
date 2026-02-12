// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt::CustomInterfaces {

struct StretchRunData {
  const std::string sampleName;
  const std::string resolutionName;
  const std::string backgroundName;
  const double eMin{-0.2};
  const double eMax{0.2};
  double startBeta{0.5};
  double endBeta{1.0};
  double startFWHM{0.01};
  double endFWHM{0.1};
  const int beta{3};
  const int sigma{3};
  int sampleBinning{1};
  const bool elasticPeak{false};
  bool sequentialFit{false};

  StretchRunData() = default;
  StretchRunData(const std::string &sample, const std::string &resolution, const std::string &bg, double min,
                 double max, int beta, int sigma, bool elastic)
      : sampleName(sample), resolutionName(resolution), backgroundName(bg), eMin(min), eMax(max), beta(beta),
        sigma(sigma), elasticPeak(elastic) {}
};

struct CurrentPreviewData {
  const std::string sampleName;
  const bool hasSample;

  CurrentPreviewData(const std::string &sampleName, bool hasSample) : sampleName(sampleName), hasSample(hasSample) {}
};

} // namespace MantidQt::CustomInterfaces
