#ifndef FITTESTHELPERS_H_
#define FITTESTHELPERS_H_

#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid;

namespace FitTestHelpers {

enum CurveBenchmarks { SingleB2BPeak, SmoothishGaussians };

// forward-declare all helper functions
static API::MatrixWorkspace_sptr generateCurveDataForFit(CurveBenchmarks ctype);

static std::string generateFunctionDescrForFit(CurveBenchmarks ctype);

static API::MatrixWorkspace_sptr generatePeaksCurveWorkspace();

static API::MatrixWorkspace_sptr generateSmoothCurveWorkspace();

/// Run fit on a (single spectrum) matrix workspace, using the given type
/// of function and minimizer option
static Mantid::API::IAlgorithm_sptr
runFitAlgorithm(MatrixWorkspace_sptr dataToFit, CurveBenchmarks ctype,
                const std::string minimizer = "Levenberg-MarquardtMD") {

  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->initialize();

  fit->setProperty("Minimizer", minimizer);
  auto funcDescr = generateFunctionDescrForFit(ctype);
  fit->setProperty("Function", funcDescr);
  fit->setProperty("InputWorkspace", dataToFit);
  fit->setProperty("CreateOutput", true);

  fit->execute();

  return fit;
}

/// Produces a workspace with data ready to be Fit-ted with the type of function
/// passed
static API::MatrixWorkspace_sptr
generateCurveDataForFit(CurveBenchmarks ctype) {

  if (SingleB2BPeak == ctype)
    return generatePeaksCurveWorkspace();
  else if (SmoothishGaussians == ctype)
    return generateSmoothCurveWorkspace();
  else
    throw std::invalid_argument(
        "Unknown curve type when trying to generate curve data: " +
        boost::lexical_cast<std::string>(ctype));
}

/// Produces a string description of a function with parameters and values, as
/// you can edit in the Fit browser
static std::string generateFunctionDescrForFit(CurveBenchmarks ctype) {

  if (SingleB2BPeak == ctype)
    return "name=BackToBackExponential, X0=8500, S=800";
  else if (SmoothishGaussians == ctype)
    return "name=BSpline, Order=20, StartX=0, EndX=10";
  else
    throw std::invalid_argument("Unknown curve type when trying to generate a "
                                "function description string: " +
                                boost::lexical_cast<std::string>(ctype));
}

// Equivalent python script. Create data with a peak and a bit of noise:
// pws = CreateSampleWorkspace(Function="User Defined",
// UserDefinedFunction="name=BackToBackExponential, I=15000, A=1, B=1.2,
// X0=10000, S=400", NumBanks=1, BankPixelWidth=1, Random=True)
static API::MatrixWorkspace_sptr generatePeaksCurveWorkspace() {

  Mantid::API::IAlgorithm_sptr sampleAlg =
      Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
  sampleAlg->initialize();
  sampleAlg->setChild(true);
  sampleAlg->setProperty("Function", "User Defined");
  sampleAlg->setProperty(
      "UserDefinedFunction",
      "name=BackToBackExponential, I=15000, A=1, B=1.2, X0=10000, S=400");
  sampleAlg->setProperty("NumBanks", 1);
  sampleAlg->setProperty("BankPixelWidth", 1);
  sampleAlg->setProperty("XMin", 0.0);
  sampleAlg->setProperty("XMax", 100.0);
  sampleAlg->setProperty("BinWidth", 0.1);
  sampleAlg->setProperty("Random", true);
  sampleAlg->setPropertyValue("OutputWorkspace", "sample_peak_curve_ws");

  sampleAlg->execute();
  API::MatrixWorkspace_sptr ws = sampleAlg->getProperty("OutputWorkspace");

  return ws;
}

// Equivalent python script. Create smooth-ish data curve:
// ws = CreateSampleWorkspace(Function="User Defined",
// UserDefinedFunction="name=LinearBackground, A0=0.4, A1=0.4; name=Gaussian,
// PeakCentre=1.3, Height=7, Sigma=1.7; name=Gaussian, PeakCentre=5,
// Height=10, Sigma=0.7; name=Gaussian, PeakCentre=8, Height=9, Sigma=1.8",
// NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.01, Random=True)
static API::MatrixWorkspace_sptr generateSmoothCurveWorkspace() {

  Mantid::API::IAlgorithm_sptr sampleAlg =
      Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
  sampleAlg->initialize();
  sampleAlg->setChild(true);
  sampleAlg->setProperty("Function", "User Defined");
  sampleAlg->setProperty(
      "UserDefinedFunction",
      "name=LinearBackground, A0=0.4, A1=0.4; name=Gaussian, PeakCentre=1.3, "
      "Height=7, Sigma=1.7; name=Gaussian, PeakCentre=5, Height=10, "
      "Sigma=0.7; name=Gaussian, PeakCentre=8, Height=9, Sigma=1.8");
  sampleAlg->setProperty("NumBanks", 1);
  sampleAlg->setProperty("BankPixelWidth", 1);
  sampleAlg->setProperty("XMin", 0.0);
  sampleAlg->setProperty("XMax", 10.0);
  sampleAlg->setProperty("BinWidth", 0.01);
  sampleAlg->setProperty("Random", true);
  sampleAlg->setPropertyValue("OutputWorkspace", "sample_smooth_curve_ws");

  sampleAlg->execute();
  API::MatrixWorkspace_sptr ws = sampleAlg->getProperty("OutputWorkspace");

  return ws;
}
} // namespace FitTestHelpers

#endif /* FITTESTHELPERS_H_ */
