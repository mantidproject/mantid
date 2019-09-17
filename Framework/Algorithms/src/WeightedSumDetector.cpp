// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <iostream>
#include <fstream>
#include <sstream>

#include "MantidAlgorithms/WeightedSumDetector.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(WeightedSumDetector)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
WeightedSumDetector::WeightedSumDetector() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
WeightedSumDetector::~WeightedSumDetector() = default;

//----------------------------------------------------------------------------------------------

void WeightedSumDetector::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "DCSWorkspace", "", Kernel::Direction::Input,
                      boost::make_shared<API::CommonBinsValidator>()),
                  "The workspace containing the spectra to be summed.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "SLFWorkspace", "", Kernel::Direction::Input,
                      boost::make_shared<API::CommonBinsValidator>()),
                  "The workspace containing the self Scattering correction.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Kernel::Direction::Output,
                      boost::make_shared<API::CommonBinsValidator>()),
                  "Workspace to contain merged spectra.");
  declareProperty(
      "Alpha", "",
      "Path to a .alf file containing Alpha values for each detector");
  declareProperty("Limits", "",
                  "Path to a .lim file containing minimum and maximum values "
                  "to sum between for each detector");
  declareProperty("Background", "",
                  "Path to a .lin file containing the gradient and intercept "
                  "of a liniar background to be subtracted from each detector");
};

void WeightedSumDetector::exec() {
  Mantid::MantidVec q;
  std::vector<Mantid::MantidVec> DCS;
  std::vector<Mantid::MantidVec> SLF;
  std::vector<double> merge;

  API::MatrixWorkspace_sptr DCSWorkspace = getProperty("DCSWorkspace");
  API::MatrixWorkspace_sptr SLFWorkspace = getProperty("SLFWorkspace");
  API::MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");
  std::string alf_dir = getProperty("Alpha");
  std::string lin_dir = getProperty("Background");
  std::string lim_dir = getProperty("Limits");

  q = DCSWorkspace->readX(0);
  size_t spectra_num = DCSWorkspace->spectrumInfo().size();
  for (size_t i = 0; i < spectra_num; i++) {
    DCS.push_back(DCSWorkspace->readY(i));
    SLF.push_back(SLFWorkspace->readY(i));
  }
  std::map<int, double> alphas = read_alf_file(alf_dir);
  std::map<int, std::vector<double>> linears = read_lin_file(lin_dir);
  std::map<int, std::vector<double>> limits = read_lim_file(lim_dir);

  for (size_t i = 0; i < q.size(); i++) {
    for (auto it = limits.begin(); it != limits.end(); it++) {
      int detector = it->first;
      double use_det = limits[detector][0];
      double merge_q = 0.0;
      double num_det = 0.0;
      if (use_det == 1) {
        num_det++;
        double q_min = limits[detector][1];
        double q_max = limits[detector][2];
        if (q[i] > q_min && q[i] < q_max) {
          double alpha = alphas[detector];
          double grad = linears[detector][1];
          double intercept = linears[detector][2];
          double background = grad * q[i] + intercept;
          double corrected =
              alpha * DCS[detector][i] - SLF[detector][i] + background;
          merge_q += corrected;
        }
	  }
      merge.push_back(merge_q / num_det);
    }
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

std::map<int, double> WeightedSumDetector::read_alf_file(std::string dir) {
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
    }
  }
  return alpha;
}

std::map<int, std::vector<double>>
WeightedSumDetector::read_lin_file(std::string dir) {
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
    }
  }
  return linear;
}

std::map<int, std::vector<double>>
WeightedSumDetector::read_lim_file(std::string dir) {
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
    }
  }
  return limit;
}

} // namespace Algorithms
} // namespace Mantid
