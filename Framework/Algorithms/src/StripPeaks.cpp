// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/StripPeaks.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StripPeaks)

using namespace Kernel;
using namespace API;

void StripPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace.");

  auto min = std::make_shared<BoundedValidator<int>>();
  min->setLower(1);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM", 7, min,
                  "The number of points covered, on average, "
                  "by the fwhm of a peak (default 7).\nPassed "
                  "through to [[FindPeaks]].");
  // The tolerance allowed in meeting the conditions
  declareProperty("Tolerance", 4, min,
                  "A measure of the strictness desired in "
                  "meeting the condition on peak "
                  "candidates,\n"
                  "Mariscotti recommends 2 (default 4)");

  declareProperty(std::make_unique<ArrayProperty<double>>("PeakPositions"),
                  "Optional: enter a comma-separated list of the expected "
                  "X-position of the centre of the peaks. Only peaks near "
                  "these positions will be fitted.");

  declareProperty("PeakPositionTolerance", 0.01,
                  "Tolerance on the found peaks' positions against the input "
                  "peak positions. Non-positive value indicates that this "
                  "option is turned off.");

  std::vector<std::string> bkgdtypes{"Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear", std::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background. Present choices include 'Linear' and 'Quadratic'");

  declareProperty("HighBackground", true, "Flag whether the input data has high background compared to peak heights.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "If set, will remove peaks only in the given spectrum of the "
                  "workspace. Otherwise, all spectra will be searched.");

  auto mustBePositiveDbl = std::make_shared<BoundedValidator<double>>();
  mustBePositiveDbl->setLower(0.);
  declareProperty("MaximumChisq", 100., mustBePositiveDbl,
                  "The maximum chisq value for fits to remove the peak. Default 100.");
}

void StripPeaks::exec() {
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  m_maxChiSq = getProperty("MaximumChisq");

  // Call FindPeaks as a Child Algorithm
  ITableWorkspace_sptr peakslist = this->findPeaks(inputWS);

  MatrixWorkspace_sptr outputWS;
  if (peakslist->rowCount() > 0) {
    // Remove the peaks that were successfully fitted
    outputWS = this->removePeaks(inputWS, peakslist);
  } else {
    // No peaks found!!! - just assign the input workspace pointer to the output
    // one
    g_log.warning("No peaks to remove!");
    outputWS = inputWS;
  }

  // Set the output workspace
  setProperty("OutputWorkspace", outputWS);
}

/** Calls FindPeaks as a Child Algorithm.
 *  @param WS :: The workspace to search
 *  @return list of found peaks
 */
API::ITableWorkspace_sptr StripPeaks::findPeaks(const API::MatrixWorkspace_sptr &WS) {
  g_log.debug("Calling FindPeaks as a Child Algorithm");

  // Read from properties
  int fwhm = getProperty("FWHM");
  int tolerance = getProperty("Tolerance");
  int wsindex = getProperty("WorkspaceIndex");
  std::string backgroundtype = getProperty("BackgroundType");
  bool highbackground = getProperty("HighBackground");
  std::vector<double> peakpositions = getProperty("PeakPositions");
  double peakpostol = getProperty("PeakPositionTolerance");
  if (peakpostol < 0.)
    peakpostol = EMPTY_DBL();

  // Set up and execute algorithm
  bool showlog = true;
  auto findpeaks = createChildAlgorithm("FindPeaks", 0.0, 0.2, showlog);
  findpeaks->setProperty("InputWorkspace", WS);
  findpeaks->setProperty<int>("FWHM", fwhm);
  findpeaks->setProperty<int>("Tolerance", tolerance);
  findpeaks->setProperty<int>("WorkspaceIndex", wsindex);
  // Get the specified peak positions, which is optional
  findpeaks->setProperty<std::vector<double>>("PeakPositions", peakpositions);
  findpeaks->setProperty<std::string>("BackgroundType", backgroundtype);
  findpeaks->setProperty<bool>("HighBackground", highbackground);
  findpeaks->setProperty<double>("PeakPositionTolerance", peakpostol);
  findpeaks->setProperty<bool>("RawPeakParameters", true);

  findpeaks->executeAsChildAlg();

  ITableWorkspace_sptr peaklistws = findpeaks->getProperty("PeaksList");
  if (!peaklistws)
    throw std::runtime_error("Algorithm FindPeaks() returned a NULL pointer "
                             "for table workspace 'PeaksList'.");

  std::stringstream infoss;
  infoss << "Call FindPeaks() to find " << peakpositions.size() << " peaks with parameters: \n";
  infoss << "\t FWHM            = " << fwhm << ";\n";
  infoss << "\t Tolerance       = " << tolerance << ";\n";
  infoss << "\t HighBackground  = " << highbackground << ";\n";
  infoss << "\t BackgroundType  = " << backgroundtype << ";\n";
  infoss << "\t Peak positions  = ";
  for (size_t i = 0; i < peakpositions.size(); ++i) {
    infoss << peakpositions[i];
    if (i < peakpositions.size() - 1)
      infoss << ", ";
  }
  infoss << ")\n"
         << "Returned number of fitted peak = " << peaklistws->rowCount() << ".";
  g_log.information(infoss.str());

  return peaklistws;
}

/** If a peak was successfully fitted, it is subtracted from the data.
 *  THIS METHOD ASSUMES THAT THE FITTING WAS DONE TO A GAUSSIAN FUNCTION.
 *  Note that the error value is left unchanged.
 *  @param input ::     The input workspace
 *  @param peakslist :: The succesfully fitted peaks
 *  @return A workspace containing the peak-subtracted data
 */
API::MatrixWorkspace_sptr StripPeaks::removePeaks(const API::MatrixWorkspace_const_sptr &input,
                                                  const API::ITableWorkspace_sptr &peakslist) {
  g_log.debug("Subtracting peaks");
  // Create an output workspace - same size as input one
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(input);
  // Copy the data over from the input to the output workspace
  const size_t hists = input->getNumberHistograms();
  // progress 0.2 to 0.3 in this loop
  double prg = 0.2;
  for (size_t k = 0; k < hists; ++k) {
    outputWS->setHistogram(k, input->histogram(k));
    prg += (0.1 / static_cast<double>(hists));
    progress(prg);
  }

  // progress from 0.3 to 1.0 here
  prg = 0.3;
  // Loop over the list of peaks
  for (size_t i = 0; i < peakslist->rowCount(); ++i) {
    // Get references to the data - X can be const
    // but Y needs to be mutable
    auto &X = outputWS->x(peakslist->getRef<int>("spectrum", i));
    auto &Y = outputWS->mutableY(peakslist->getRef<int>("spectrum", i));
    // Get back the gaussian parameters
    const double height = peakslist->getRef<double>("Height", i);
    const double centre = peakslist->getRef<double>("PeakCentre", i);
    const double sigma = peakslist->getRef<double>("Sigma", i);
    const double chisq = peakslist->getRef<double>("chi2", i);
    // These are some heuristic rules to discard bad fits.
    // Hope to be able to remove them when we have better fitting routine
    if (height < 0) {
      g_log.error() << "Find Peak with Negative Height\n";
      continue; // Height must be positive
    }
    if (chisq > m_maxChiSq) {
      g_log.information() << "Error for fit peak at " << centre << " is " << chisq << ", which exceeds allowed value "
                          << m_maxChiSq << "\n";
      if (chisq != DBL_MAX)
        g_log.error() << "StripPeaks():  Peak Index = " << i << " @ X = " << centre
                      << "  Error: Peak fit with too high of chisq " << chisq << " > " << m_maxChiSq << "\n";
      continue;
    } else if (chisq < 0.) {
      g_log.warning() << "StripPeaks():  Peak Index = " << i << " @ X = " << centre
                      << ". Error: Peak fit with too wide peak width" << sigma << " denoted by chi^2 = " << chisq
                      << " <= 0. \n";
    }
    {
      // find the bin width at the center of the peak - average of the bins on
      // either side
      const auto center_iter = lower_bound(X.begin(), X.end(), centre);
      const double bin_width = .5 * (*(center_iter + 1) - (*(center_iter - 1)));

      // seek wikipedia if you don't believe the conversion factor
      const double fwhm = sigma * 2. * std::sqrt(2. * std::log(2.));

      if ((fwhm / bin_width) < 1.5) {
        g_log.warning() << "StripPeaks():  Peak Index = " << i << " @ X = " << centre
                        << "  Error: Peak fit with too narrow of peak"
                        << " fwhm = " << fwhm << " bin size = " << bin_width
                        << " num bins in peak = " << 2. * (fwhm / bin_width) << " <3\n";
        continue;
      }
    }

    g_log.information() << "Subtracting peak " << i << " from spectrum " << peakslist->getRef<int>("spectrum", i)
                        << " at x = " << centre << " h = " << height << " s = " << sigma << " chi2 = " << chisq << "\n";

    { // log the background function
      double a0 = 0.;
      double a1 = 0.;
      double a2 = 0.;
      const std::vector<std::string> columnNames = peakslist->getColumnNames();
      if (std::find(columnNames.begin(), columnNames.end(), "A0") != columnNames.end())
        a0 = peakslist->getRef<double>("A0", i);
      if (std::find(columnNames.begin(), columnNames.end(), "A1") != columnNames.end())
        a1 = peakslist->getRef<double>("A1", i);
      if (std::find(columnNames.begin(), columnNames.end(), "A2") != columnNames.end())
        a2 = peakslist->getRef<double>("A2", i);
      g_log.information() << "     background = " << a0 << " + " << a1 << " x + " << a2 << " x^2\n";
    }

    // Get central bin values using points
    auto binCenters = outputWS->points(peakslist->getRef<int>("spectrum", i));
    const auto spectrumLength = static_cast<int>(Y.size());
    for (int j = 0; j < spectrumLength; ++j) {
      double x = binCenters[j];
      // Skip if not anywhere near this peak
      if (x < centre - 5.0 * sigma)
        continue;
      if (x > centre + 5.0 * sigma)
        break;
      // Calculate the value of the Gaussian function at this point
      const double funcVal = height * exp(-0.5 * pow((x - centre) / sigma, 2));
      // Subtract the calculated value from the data
      Y[j] -= funcVal;
    }
    prg += (0.7 / static_cast<double>(peakslist->rowCount()));
    progress(prg);
  }

  return outputWS;
}

} // namespace Mantid::Algorithms
