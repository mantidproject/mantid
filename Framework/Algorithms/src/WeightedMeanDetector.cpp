// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <iostream>
#include <fstream>
#include <sstream>

#include "MantidAlgorithms/WeightedMeanDetector.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(WeightedMeanDetector)

std::map<std::string, std::string> WeightedMeanDetector::validateInputs() {
  std::map<std::string, std::string> issues;

  API::MatrixWorkspace_sptr DCSWorkspace = getProperty("DCSWorkspace");
  API::MatrixWorkspace_sptr SLFWorkspace = getProperty("SLFWorkspace");
  const size_t spectra_num_DCS = DCSWorkspace->spectrumInfo().size();
  const size_t spectra_num_SLF = SLFWorkspace->spectrumInfo().size();
  if (spectra_num_DCS != spectra_num_SLF) {
    issues["SLFWorkspace"] =
        "SLFWorkspace detector number does not match DCSWorkspace";
  }

  std::vector<std::string> cor_files = {"AlfFile", "LinFile", "LimFile"};
  for (const std::string &param : cor_files) {
    std::string line;
    int detector_num;
    std::string dir;
    dir = getProperty(param);
    std::ifstream file(dir);
    std::getline(file, line);
    std::stringstream ss(line);
    ss >> detector_num;
    if (detector_num != spectra_num_DCS) {
      issues[param] = param + " Detector number does not DCSWorkspace";
    }
  }
  return issues;
}

void WeightedMeanDetector::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "DCSWorkspace", "", Kernel::Direction::Input),
                  "The workspace containing the corrected total scattering for "
                  "each detector to be summed.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "SLFWorkspace", "", Kernel::Direction::Input),
                  "The workspace containing the self Scattering correction for "
                  "each detector.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "Workspace to contain merged spectra.");
  declareProperty(
      std::make_unique<API::FileProperty>("AlfFile", "", API::FileProperty::Load),
      "Path to a .alf file containing Alpha values for each detector");
  declareProperty(std::make_unique<API::FileProperty>("LimFile", "",
                                                      API::FileProperty::Load),
                  "Path to a .lim file containing minimum and maximum values "
                  "to sum between for each detector");
  declareProperty(std::make_unique<API::FileProperty>("LinFile", "",
                                                      API::FileProperty::Load),
                  "Path to a .lin file containing the gradient and intercept "
                  "of a liniar background to be subtracted from each detector");
};

void WeightedMeanDetector::exec() {
  Mantid::MantidVec q;
  std::vector<Mantid::MantidVec> DCS;
  std::vector<Mantid::MantidVec> SLF;
  std::vector<double> merge;

  API::MatrixWorkspace_sptr DCSWorkspace = getProperty("DCSWorkspace");
  API::MatrixWorkspace_sptr SLFWorkspace = getProperty("SLFWorkspace");
  API::MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");

  const size_t spectra_num = DCSWorkspace->spectrumInfo().size();
  const std::string alf_dir = getProperty("AlfFile");
  const std::string lin_dir = getProperty("LinFile");
  const std::string lim_dir = getProperty("LimFile");
  std::map<int, double> alphas = read_alf_file(alf_dir, spectra_num);
  std::map<int, std::vector<double>> linears =
      read_lin_file(lin_dir, spectra_num);
  std::map<int, std::vector<double>> limits =
      read_lim_file(lim_dir, spectra_num);

  const size_t n_bins = DCSWorkspace->readX(0).size();
  double max_limit = -1.0;
  double min_limit = -1.0;
  for (auto it = limits.begin(); it != limits.end(); it++) {
    if (max_limit == -1.0) {
      min_limit = it->second[1];
      max_limit = it->second[2];
    } else {
      min_limit = std::min(min_limit, it->second[1]);
      max_limit = std::max(max_limit, it->second[2]);
    }
  }

  const double new_binwidth = (max_limit - min_limit) / n_bins;
  std::ostringstream binParams;
  binParams << min_limit << "," << new_binwidth << "," << max_limit;
  API::MatrixWorkspace_sptr DCSWorkspace_rebined =
      rebin(DCSWorkspace, binParams.str());
  API::MatrixWorkspace_sptr SLFWorkspace_rebined =
      rebin(SLFWorkspace, binParams.str());

  q = DCSWorkspace_rebined->readX(0);
  for (size_t i = 0; i < spectra_num; i++) {
    DCS.push_back(DCSWorkspace_rebined->readY(i));
    SLF.push_back(SLFWorkspace_rebined->readY(i));
  }

  for (size_t i = 0; i < (q.size()-1); i++) {
    double merge_q = 0.0;
    double num_det = 0.0;
    for (auto it = limits.begin(); it != limits.end(); it++) {
      int detector = it->first;
      int det_index = detector - 1;
      double use_det = limits[detector][0];
      if (use_det == 1) {
        num_det++;
        double q_min = limits[detector][1];
        double q_max = limits[detector][2];
        if (q[i] > q_min && q[i+1] < q_max) {
          double alpha = alphas[detector];
          double grad = linears[detector][1];
          double intercept = linears[detector][2];
          double background = grad * (q[i] + new_binwidth / 2.0) + intercept;
          double corrected =
              alpha * DCS[det_index][i] - SLF[det_index][i] + background;
          merge_q += corrected;
        }
	  }
    }
    merge.push_back(merge_q / num_det);
  }
  Mantid::API::Algorithm_sptr ChildAlg =
      createChildAlgorithm("CreateWorkspace");
  ChildAlg->setProperty("DataX", q);
  ChildAlg->setProperty("DataY", merge);
  ChildAlg->setProperty("UnitX", "Angstrom");
  ChildAlg->setProperty("NSpec", 1);
  ChildAlg->setProperty("ParentWorkspace", DCSWorkspace);
  ChildAlg->setProperty("Distribution", true);
  ChildAlg->execute();
  outWS = ChildAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outWS);
};

const std::map<int, double> WeightedMeanDetector::read_alf_file(std::string dir,
                                                         size_t spectra_num) {
  std::map<int, double> alpha;
  std::ifstream alf_file(dir);
  std::string line;
  bool first_line = TRUE;
  while (std::getline(alf_file, line)) {
    if (first_line == FALSE) {
      std::stringstream ss(line);
      int detector;
      double value;
      ss >> detector;
      ss >> value;
      alpha[detector] = value;
    } else {
      first_line = FALSE;
      std::stringstream ss(line);
      int detector;
      ss >> detector;
      if (detector != spectra_num) {
        try {
          throw "Invalid .alf file";
        } catch (std::string e) {
          std::cout << "An exception occurred." << e << '\n';
        }
      }
    }
  }
  return alpha;
}

const std::map<int, std::vector<double>>
WeightedMeanDetector::read_lin_file(std::string dir, size_t spectra_num) {
  std::map<int, std::vector<double>> linear;
  std::ifstream lin_file(dir);
  std::string line;
  bool first_line = TRUE;
  while (std::getline(lin_file, line)) {
    if (first_line == FALSE) {
      std::stringstream ss(line);
      int detector;
      double has_linear;
      double grad;
      double intercept;
      ss >> detector;
      ss >> has_linear;
      if (has_linear == 0.0) {
        grad = 0.0;
        intercept = 0.0;
      } else {
        ss >> grad;
        ss >> intercept;
      }
      std::vector<double> value = {has_linear, grad, intercept};
      linear[detector] = value;
    } else {
      first_line = FALSE;
      std::stringstream ss(line);
      int detector;
      ss >> detector;
      if (detector != spectra_num) {
        try {
          throw "Invalid .lin file";
        } catch (std::string e) {
          std::cout << "An exception occurred." << e << '\n';
        }
      }
    }
  }
  return linear;
}

const std::map<int, std::vector<double>>
WeightedMeanDetector::read_lim_file(std::string dir, size_t spectra_num) {
  std::map<int, std::vector<double>> limit;
  std::ifstream lim_file(dir);
  std::string line;
  bool first_line = TRUE;
  while (std::getline(lim_file, line)) {
    if (first_line == FALSE) {
      std::stringstream ss(line);
      int detector;
      double use_det;
      double q_min;
      double q_max;
      ss >> detector;
      ss >> use_det;
      if (use_det == 0.0) {
        q_min = 0.0;
        q_max = 0.0;
      } else {
        ss >> q_min;
        ss >> q_max;
      }
      std::vector<double> value = {use_det, q_min, q_max};
      limit[detector] = value;
    } else {
      first_line = FALSE;
      std::stringstream ss(line);
      int detector;
      ss >> detector;
      if (detector != spectra_num) {
        try {
          throw "Invalid .lim file";
        } catch (std::string e) {
          std::cout << "An exception occurred." << e << '\n';
        }
      }
    }
  }
  return limit;
}

const API::MatrixWorkspace_sptr
WeightedMeanDetector::rebin(API::MatrixWorkspace_sptr input,
	std::string params) {
  auto childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty("InputWorkspace", input);
  childAlg->setProperty("OutputWorkspace", "blank");
  childAlg->setPropertyValue("Params", params);
  childAlg->executeAsChildAlg();
  API::MatrixWorkspace_sptr rebined =
      childAlg->getProperty("OutputWorkspace");
  return rebined;
}

} // namespace Algorithms
} // namespace Mantid
