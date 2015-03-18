//-----------------------------
// Includes
//----------------------------
#include "MantidAlgorithms/CorrectToFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorrectToFile)
// estimate that this algorithm will spend half it's time loading the file
const double CorrectToFile::LOAD_TIME = 0.5;

void CorrectToFile::init() {
  declareProperty(new API::WorkspaceProperty<>("WorkspaceToCorrect", "",
                                               Kernel::Direction::Input),
                  "Name of the input workspace");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load),
      "The file containing the correction factors");

  std::vector<std::string> propOptions =
      Kernel::UnitFactory::Instance().getKeys();
  propOptions.push_back("SpectrumNumber");
  declareProperty("FirstColumnValue", "Wavelength",
                  boost::make_shared<Kernel::StringListValidator>(propOptions),
                  "The units of the first column of the correction file "
                  "(default wavelength)");

  std::vector<std::string> operations(1, std::string("Divide"));
  operations.push_back("Multiply");
  declareProperty("WorkspaceOperation", "Divide",
                  boost::make_shared<Kernel::StringListValidator>(operations),
                  "Allowed values: Divide, Multiply (default is divide)");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                               Kernel::Direction::Output),
                  "Name of the output workspace to store the results in");
}

void CorrectToFile::exec() {
  // The input workspace is the uncorrected data
  MatrixWorkspace_sptr toCorrect = getProperty("WorkspaceToCorrect");
  // This workspace is loaded from the RKH compatible file
  MatrixWorkspace_sptr rkhInput = loadInFile(getProperty("Filename"));
  // Only create the output workspace if it's not the same as the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != toCorrect) {
    outputWS = WorkspaceFactory::Instance().create(toCorrect);
  }
  const std::string operation = getProperty("WorkspaceOperation");

  if (getPropertyValue("FirstColumnValue") == "SpectrumNumber") {
    // the workspace (probably) contains many spectra but each with only 1 bin
    doWkspAlgebra(toCorrect, rkhInput, operation, outputWS);
  } else // interpolation the correction values and divide or multiply the input
         // by these values
  {      // the correction values should be all contained in 1 spectrum
    // Check that the workspace to rebin has the same units as the one that we
    // are matching to
    // However, just print a warning if it isn't, don't abort (since user
    // provides the file's unit)
    if (toCorrect->getAxis(0)->unit()->unitID() !=
        rkhInput->getAxis(0)->unit()->unitID()) {
      g_log.warning("Unit on input workspace is different to that specified in "
                    "'FirstColumnValue' property");
    }

    // Get references to the correction factors
    const MantidVec &Xcor = rkhInput->readX(0);
    const MantidVec &Ycor = rkhInput->readY(0);
    const MantidVec &Ecor = rkhInput->readE(0);

    const bool histogramData = outputWS->isHistogramData();
    const bool divide = (operation == "Divide") ? true : false;
    double Yfactor, correctError;

    const int64_t nOutSpec =
        static_cast<int64_t>(outputWS->getNumberHistograms());
    const size_t nbins = outputWS->blocksize();
    // Set the progress bar
    Progress prg(this, 0 /*LOAD_TIME*/, 1.0, nOutSpec);

    for (int64_t i = 0; i < nOutSpec; ++i) {
      MantidVec &xOut = outputWS->dataX(i);
      MantidVec &yOut = outputWS->dataY(i);
      MantidVec &eOut = outputWS->dataE(i);

      const MantidVec &xIn = toCorrect->readX(i);
      const MantidVec &yIn = toCorrect->readY(i);
      const MantidVec &eIn = toCorrect->readE(i);

      for (size_t j = 0; j < nbins; ++j) {
        const double currentX =
            histogramData ? (xIn[j] + xIn[j + 1]) / 2.0 : xIn[j];
        // Find out the index of the first correction point after this value
        MantidVec::const_iterator pos =
            std::lower_bound(Xcor.begin(), Xcor.end(), currentX);
        const size_t index = pos - Xcor.begin();
        if (index == Xcor.size()) {
          // If we're past the end of the correction factors vector, use the
          // last point
          Yfactor = Ycor[index - 1];
          correctError = Ecor[index - 1];
        } else if (index) {
          // Calculate where between the two closest points our current X value
          // is
          const double fraction =
              (currentX - Xcor[index - 1]) / (Xcor[index] - Xcor[index - 1]);
          // Now linearly interpolate to find the correction factors to use
          Yfactor =
              Ycor[index - 1] + fraction * (Ycor[index] - Ycor[index - 1]);
          correctError =
              Ecor[index - 1] + fraction * (Ecor[index] - Ecor[index - 1]);
        } else {
          // If we're before the start of the correction factors vector, use the
          // first point
          Yfactor = Ycor[0];
          correctError = Ecor[0];
        }

        // Now do the correction on the current point
        if (divide) {
          yOut[j] = yIn[j] / Yfactor;
          // the proportional error is equal to the sum of the proportional
          // errors
          //  re-arrange so that you don't get infinity if leftY==0. Sa = error
          //  on a, etc.
          // c = a/b
          // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
          // (Sa c/a)2 + (Sb c/b)2 = (Sc)2
          // = (Sa 1/b)2 + (Sb (a/b2))2
          // (Sc)2 = (1/b)2( (Sa)2 + (Sb a/b)2 )
          eOut[j] =
              sqrt(pow(eIn[j], 2) + pow(yIn[j] * correctError / Yfactor, 2)) /
              Yfactor;
        } else {
          yOut[j] = yIn[j] * Yfactor;
          // error multiplying two uncorrelated numbers, re-arrange so that you
          // don't get infinity if leftY or rightY == 0
          //  Sa = error on a, etc.
          // c = a*b
          // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
          // (Sc)2 = (Sa c/a)2 + (Sb c/b)2 = (Sa b)2 + (Sb a)2
          eOut[j] =
              sqrt(pow(eIn[j] * Yfactor, 2) + pow(correctError * yIn[j], 2));
        }

        // Copy X value over
        xOut[j] = xIn[j];
      }
      if (histogramData)
        xOut[nbins] = xIn[nbins];
      prg.report("CorrectToFile: applying " + operation);
    }
  }

  // Set the resulting workspace
  setProperty("Outputworkspace", outputWS);
}
/** Load in the RKH file for that has the correction information
*  @param corrFile :: the name of the correction to load
*  @return workspace containing the loaded data
*  @throw runtime_error if load algorithm fails
*/
MatrixWorkspace_sptr CorrectToFile::loadInFile(const std::string &corrFile) {
  g_log.information() << "Loading file " << corrFile << std::endl;
  progress(0, "Loading file");
  IAlgorithm_sptr loadRKH =
      createChildAlgorithm("LoadRKH", 0, 1.0 /*LOAD_TIME*/);
  std::string rkhfile = getProperty("Filename");
  loadRKH->setPropertyValue("Filename", rkhfile);
  loadRKH->setPropertyValue("OutputWorkspace", "rkhout");
  std::string columnValue = getProperty("FirstColumnValue");
  loadRKH->setPropertyValue("FirstColumnValue", columnValue);
  loadRKH->executeAsChildAlg();

  g_log.debug() << corrFile << " loaded\n";
  return loadRKH->getProperty("OutputWorkspace");
}
/** Multiply or divide the input workspace as specified by the user
*  @param[in] lhs the first input workspace
*  @param[in] rhs the last input workspace
*  @param[in] algName the name of the algorithm to use on the input files
*  @param[out] result the output workspace
*  @throw NotFoundError if requested algorithm requested doesn't exist
*  @throw runtime_error if algorithm encounters an error
*/
void CorrectToFile::doWkspAlgebra(API::MatrixWorkspace_sptr lhs,
                                  API::MatrixWorkspace_sptr rhs,
                                  const std::string &algName,
                                  API::MatrixWorkspace_sptr &result) {
  g_log.information() << "Initalising the algorithm " << algName << std::endl;
  progress(LOAD_TIME, "Applying correction");
  IAlgorithm_sptr algebra = createChildAlgorithm(algName, LOAD_TIME, 1.0);
  algebra->setProperty("LHSWorkspace", lhs);
  algebra->setProperty("RHSWorkspace", rhs);
  algebra->setProperty("OutputWorkspace", result);

  try {
    algebra->execute();
  } catch (std::runtime_error &) {
    g_log.warning() << "Error encountered while running algorithm " << algName
                    << std::endl;
    g_log.error() << "Correction file "
                  << getPropertyValue("Filename") +
                         " can't be used to correct workspace "
                  << getPropertyValue("WorkspaceToCorrect") << std::endl;
    g_log.error() << "Mismatched number of spectra?" << std::endl;
    throw std::runtime_error("Correct to file failed, see log for details");
  }

  result = algebra->getProperty("OutputWorkspace");
  g_log.debug() << algName << " complete\n";
}
}
}
