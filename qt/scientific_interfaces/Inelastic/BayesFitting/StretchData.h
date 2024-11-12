// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt::CustomInterfaces {

struct StretchRunData {
  const std::string sampleName;
  const std::string resolutionName;
  const std::string backgroundName;
  const double eMin;
  const double eMax;
  const int beta;
  const bool elasticPeak;
  const int sigma;
  const int sampleBinning;
  const bool sequentialFit;

  StretchRunData(const std::string &sample, const std::string &resolution, double min, double max, int b, bool elastic,
                 const std::string &bg, int sig, int binning, bool sequential)
      : sampleName(sample), resolutionName(resolution), eMin(min), eMax(max), beta(b), elasticPeak(elastic),
        backgroundName(bg), sigma(sig), sampleBinning(binning), sequentialFit(sequential) {}
};

struct CurrentPreviewData {
  const std::string sampleName;
  const bool hasSample;

  CurrentPreviewData(const std::string sampleName, bool hasSample) : sampleName(sampleName), hasSample(hasSample) {}
};

} // namespace MantidQt::CustomInterfaces
