//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PhaseQuadMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PhaseQuadMuon::init() {

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace containing the spectra");

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace to hold squashograms");

  declareProperty(
      new API::WorkspaceProperty<API::ITableWorkspace>(
          "PhaseTable", "", Direction::Input, API::PropertyMode::Optional),
      "Name of the Phase Table");

  declareProperty(
      new PropertyWithValue<int>("PulseOver", 60, Direction::Input),
      "Bin number when the pulse is over. Mandatory if PhaseTable is provided");

  declareProperty(
      new PropertyWithValue<double>("MeanLag", 127.702, Direction::Input),
      "Average Lag value over detectors");

  declareProperty(
      new PropertyWithValue<bool>("DoublePulse", false, Direction::Input),
      "If signal is double-pulse");

  declareProperty(new API::FileProperty("PhaseList", "",
                                        API::FileProperty::OptionalLoad, ".INF",
                                        Direction::Input),
                  "A space-delimited text file with a six-row header");
}

/** Executes the algorithm
 *
 */
void PhaseQuadMuon::exec() {
  // Get input workspace
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Get input phase table
  API::ITableWorkspace_sptr phaseTable = getProperty("PhaseTable");

  // Get input phase list
  std::string filename = getPropertyValue("PhaseList");

  // Set number of histograms
  m_nHist = static_cast<int>(inputWs->getNumberHistograms());
  // Set number of data points per histogram
  m_nData = static_cast<int>(inputWs->getSpectrum(0)->readY().size());

  // Not necessary as we will be converting back to micro secs
  // Just to make sure that the code is doing exactly the same as the VAX c code
  // BUT the method also shifts the spectra, and we definitely need this shift
  convertToNanoSecs(inputWs);

  // Set time resolution
  m_res =
      inputWs->getSpectrum(0)->readX()[1] - inputWs->getSpectrum(0)->readX()[0];

  // Create a deadTimeTable to apply deadtime corrections. It will be filled by
  // loadPhaseTable or loadPhaseList
  API::ITableWorkspace_sptr deadTimeTable =
      API::WorkspaceFactory::Instance().createTable();
  deadTimeTable->addColumn("int", "Spectrum no");
  deadTimeTable->addColumn("double", "Deadtime");

  // Check that either phaseTable or PhaseList has been provided
  if (phaseTable) {
    loadPhaseTable(phaseTable, deadTimeTable);
    // If phaseTable was supplied, get m_tPulseOver, m_meanLag
    m_tPulseOver = getProperty("PulseOver");
    m_meanLag = getProperty("MeanLag");
  } else if (filename != "") {
    loadPhaseList(filename, deadTimeTable);
  } else {
    throw std::runtime_error(
        "PhaseQuad: You must provide either PhaseTable or PhaseList");
  }

  // Check if signal is double-pulse and set m_tPulseOver accordingly
  m_isDouble = getProperty("DoublePulse");
  double a = m_meanLag + m_pulseTail;
  if (m_isDouble) {
    a += m_pulseTwo * 1000;
  }
  a /= m_res;
  m_tPulseOver = static_cast<int>(a);

  // Create temporary workspace to perform operations on it
  API::MatrixWorkspace_sptr tempWs =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", m_nHist,
                                                   m_nData + 1, m_nData));

  // Apply deadtime corrections and store results in tempWs
  deadTimeCorrection(inputWs, deadTimeTable, tempWs);

  // Create output workspace with two spectra (squashograms)
  API::MatrixWorkspace_sptr outputWs =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 2,
                                                   m_nData + 1, m_nData));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  // Rescale detector efficiency to maximum value
  normaliseAlphas(m_histData);

  // Remove exponential decay and save results into tempWs
  loseExponentialDecay(tempWs);

  // Compute squashograms
  squash(tempWs, outputWs);

  // Regain exponential decay
  regainExponential(outputWs);

  // Convert and shift output and input workspaces
  convertToMicroSecs(inputWs);
  convertToMicroSecs(outputWs);

  setProperty("OutputWorkspace", outputWs);
}

//----------------------------------------------------------------------------------------------
/** Convert X units from micro-secs to nano-secs and shift to start at t=0
* @param inputWs :: [input/output] workspace to convert
*/
void PhaseQuadMuon::convertToNanoSecs(API::MatrixWorkspace_sptr inputWs) {
  for (size_t h = 0; h < inputWs->getNumberHistograms(); h++) {
    auto spec = inputWs->getSpectrum(h);
    m_tMin = spec->dataX()[0];
    for (int t = 0; t < m_nData + 1; t++) {
      spec->dataX()[t] = (spec->dataX()[t] - m_tMin) * 1000;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Convert X units from nano-secs to micro-secs and shift to start at m_tMin
* @param inputWs :: [input/output] workspace to convert
*/
void PhaseQuadMuon::convertToMicroSecs(API::MatrixWorkspace_sptr inputWs) {
  for (size_t h = 0; h < inputWs->getNumberHistograms(); h++) {
    auto spec = inputWs->getSpectrum(h);
    for (int t = 0; t < m_nData + 1; t++) {
      spec->dataX()[t] = spec->dataX()[t] / 1000 + m_tMin;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Apply dead time correction to spectra in inputWs and create temporary
* workspace with corrected spectra
* @param inputWs :: [input] input workspace containing spectra to correct
* @param deadTimeTable :: [input] table containing dead times
* @param tempWs :: [output] workspace containing corrected spectra
*/
void PhaseQuadMuon::deadTimeCorrection(API::MatrixWorkspace_sptr inputWs,
                                       API::ITableWorkspace_sptr deadTimeTable,
                                       API::MatrixWorkspace_sptr &tempWs) {

  // Apply correction only from t = m_tPulseOver
  // To do so, we first apply corrections to the whole spectrum
  // (ApplyDeadTimeCorr
  // does not allow to select a range in the spectrum)
  // Then we recover counts from 0 to m_tPulseOver

  auto alg = this->createChildAlgorithm("ApplyDeadTimeCorr", -1, -1);
  alg->initialize();
  alg->setProperty("DeadTimeTable", deadTimeTable);
  alg->setPropertyValue("InputWorkspace", inputWs->getName());
  alg->setPropertyValue("OutputWorkspace", inputWs->getName() + "_deadtime");
  bool sucDeadTime = alg->execute();
  if (!sucDeadTime) {
    g_log.error() << "PhaseQuad: Unable to apply dead time corrections"
                  << std::endl;
    throw std::runtime_error(
        "PhaseQuad: Unable to apply dead time corrections");
  }
  tempWs = alg->getProperty("OutputWorkspace");

  // Now recover counts from t=0 to m_tPulseOver
  // Errors are set to m_bigNumber
  for (int h = 0; h < m_nHist; h++) {
    auto specOld = inputWs->getSpectrum(h);
    auto specNew = tempWs->getSpectrum(h);

    for (int t = 0; t < m_tPulseOver; t++) {
      specNew->dataY()[t] = specOld->dataY()[t];
      specNew->dataE()[t] = m_bigNumber;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Load PhaseTable file to a vector of HistData.
* @param phaseTable :: [input] phase table containing detector info
* @param deadTimeTable :: [output] phase table containing dead times
*/
void PhaseQuadMuon::loadPhaseTable(API::ITableWorkspace_sptr phaseTable,
                                   API::ITableWorkspace_sptr deadTimeTable) {
  if (phaseTable->rowCount()) {
    if (phaseTable->columnCount() < 4) {
      throw std::invalid_argument(
          "PhaseQuad: PhaseTable must contain at least four columns");
    }

    // Check number of histograms in inputWs match number of detectors in phase
    // table
    if (m_nHist != static_cast<int>(phaseTable->rowCount())) {
      throw std::runtime_error("PhaseQuad: Number of histograms in phase table "
                               "does not match number of spectra in workspace");
    }

    for (size_t i = 0; i < phaseTable->rowCount(); ++i) {
      API::TableRow phaseRow = phaseTable->getRow(i);

      // The first three columns go to m_histData
      HistData tempHist;
      tempHist.detOK = phaseRow.Bool(0);
      tempHist.alpha = phaseRow.Double(1);
      tempHist.phi = phaseRow.Double(2);
      m_histData.push_back(tempHist);

      // The last column goes to deadTimeTable
      API::TableRow deadRow = deadTimeTable->appendRow();
      deadRow << static_cast<int>(i) + 1 << phaseRow.Double(3);
    }

  } else {
    throw std::invalid_argument("PhaseQuad: PhaseTable is empty");
  }
}

//----------------------------------------------------------------------------------------------
/** Load PhaseList file to a vector of HistData and a deadTimeTable.
* @param filename :: [input] phase list .inf filename
* @param deadTimeTable :: [output] table containing dead times
*/
void PhaseQuadMuon::loadPhaseList(const std::string &filename,
                                  API::ITableWorkspace_sptr deadTimeTable) {

  std::ifstream input(filename.c_str(), std::ios_base::in);

  if (input.is_open()) {

    if (input.eof()) {
      throw Exception::FileError("PhaseQuad: File is empty.", filename);
    } else {
      std::string line;

      // Header of .INF file is as follows:
      //
      // Comment on the output file
      // Top row of numbers are:
      // #histos, typ. first good bin#, typ. bin# when pulse over, mean lag.
      // Tabulated numbers are, per histogram:
      // det ok, asymmetry, phase, lag, deadtime_c, deadtime_m.
      //
      std::getline(input, line); // Skip first line in header
      std::getline(input, line); // Skip second line
      std::getline(input, line); // ...
      std::getline(input, line);
      std::getline(input, line);

      // Read first useful line
      int nHist;
      input >> nHist >> m_tValid >> m_tPulseOver >> m_meanLag;
      if (m_nHist != nHist) {
        throw std::runtime_error("PhaseQuad: Number of histograms in phase "
                                 "list does not match number of spectra in "
                                 "workspace");
      }

      // Read histogram data
      int cont = 0;
      HistData tempData;
      double lag, dead, deadm;
      while (input >> tempData.detOK >> tempData.alpha >> tempData.phi >> lag >>
             dead >> deadm) {
        m_histData.push_back(tempData);
        cont++;

        // Add dead time to deadTimeTable
        API::TableRow row = deadTimeTable->appendRow();
        row << cont << dead;
      }

      if (cont != m_nHist) {
        if (cont < m_nHist) {
          throw Exception::FileError("PhaseQuad: Lines missing in phase list",
                                     filename);
        } else {
          throw Exception::FileError("PhaseQuad: Extra lines in phase list",
                                     filename);
        }
      }
    }
  } else {
    // Throw exception if file cannot be opened
    throw std::runtime_error("PhaseQuad: Unable to open PhaseList");
  }
}

//----------------------------------------------------------------------------------------------
/** Rescale detector efficiencies to maximum value.
* @param histData :: vector of HistData containing detector efficiencies
*/
void PhaseQuadMuon::normaliseAlphas(std::vector<HistData> &histData) {
  double max = 0;
  for (int h = 0; h < m_nHist; h++) {
    if (histData[h].alpha > max) {
      max = histData[h].alpha;
    }
  }
  if (!max) {
    throw std::runtime_error("PhaseQuad: Could not rescale efficiencies");
  }

  for (int h = 0; h < m_nHist; h++) {
    histData[h].alpha /= max;
  }
}

//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param tempWs :: workspace containing the spectra to remove exponential from
*/
void PhaseQuadMuon::loseExponentialDecay(API::MatrixWorkspace_sptr tempWs) {
  for (size_t h = 0; h < tempWs->getNumberHistograms(); h++) {
    auto specIn = tempWs->getSpectrum(h);
    MantidVec outX, outY, outE;
    MantidVec inX, inY, inE;

    inX = specIn->readX();
    inY = specIn->readY();
    inE = specIn->readE();
    outX = specIn->readX();
    outY = specIn->readY();
    outE = specIn->readE();

    for (int i = 0; i < m_nData; i++) {
      double usey = specIn->readY()[i];
      double oops = ((usey <= 0) || (specIn->readE()[i] >= m_bigNumber));
      outY[i] = oops ? 0 : log(usey);
      outE[i] = oops ? m_bigNumber : specIn->readE()[i] / usey;
    }

    double s, sx, sy;
    s = sx = sy = 0;
    for (int i = 0; i < m_nData; i++) {
      double sig;
      sig = outE[i] * outE[i];
      s += 1. / sig;
      sx += outX[i] / sig;
      sy += outY[i] / sig;
    }
    double N0 = (sy + sx / m_muLife / 1000) / s;
    N0 = exp(N0);
    m_histData[h].n0 = N0;

    for (int i = 0; i < m_nData; i++) {
      specIn->dataY()[i] = inY[i] - N0 * exp(-outX[i] / m_muLife / 1000);
      if (i < m_tPulseOver) {
        specIn->dataE()[i] = m_bigNumber;
      } else {
        specIn->dataE()[i] = (inY[i] > m_poissonLim)
                                 ? inE[i]
                                 : sqrt(N0 * exp(-outX[i] / m_muLife / 1000));
      }
    }
  } // Histogram loop
}

//----------------------------------------------------------------------------------------------
/** Compute Squashograms
* @param tempWs :: input workspace containing the asymmetry in the lab frame
* @param outputWs :: output workspace to hold squashograms
*/
void PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr tempWs,
                           API::MatrixWorkspace_sptr outputWs) {

  double sxx = 0;
  double syy = 0;
  double sxy = 0;

  for (int h = 0; h < m_nHist; h++) {
    auto data = m_histData[h];
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
    sxx += X * X;
    syy += Y * Y;
    sxy += X * Y;
  }

  double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
  double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
  double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
  double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
  std::vector<double> aj, bj;
  for (int h = 0; h < m_nHist; h++) {
    auto data = m_histData[h];
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
    aj.push_back((lam1 * X + mu1 * Y) * 0.5);
    bj.push_back((lam2 * X + mu2 * Y) * 0.5);
  }

  std::vector<double> data1(m_nData, 0), data2(m_nData, 0);
  std::vector<double> sigm1(m_nData, 0), sigm2(m_nData, 0);
  for (int i = 0; i < m_nData; i++) {
    for (int h = 0; h < m_nHist; h++) {
      auto spec = tempWs->getSpectrum(h);
      data1[i] += aj[h] * spec->readY()[i];
      data2[i] += bj[h] * spec->readY()[i];
      sigm1[i] += aj[h] * aj[h] * spec->readE()[i] * spec->readE()[i];
      sigm2[i] += bj[h] * bj[h] * spec->readE()[i] * spec->readE()[i];
    }
    sigm1[i] = sqrt(sigm1[i]);
    sigm2[i] = sqrt(sigm2[i]);
  }

  outputWs->getSpectrum(0)->dataX() = tempWs->getSpectrum(0)->readX();
  outputWs->getSpectrum(0)->dataY() = data1;
  outputWs->getSpectrum(0)->dataE() = sigm1;
  outputWs->getSpectrum(1)->dataX() = tempWs->getSpectrum(1)->readX();
  outputWs->getSpectrum(1)->dataY() = data2;
  outputWs->getSpectrum(1)->dataE() = sigm2;
}

//----------------------------------------------------------------------------------------------
/** Put back in exponential decay
* @param outputWs :: output workspace with squashograms to update
*/
void PhaseQuadMuon::regainExponential(API::MatrixWorkspace_sptr outputWs) {
  auto specRe = outputWs->getSpectrum(0);
  auto specIm = outputWs->getSpectrum(1);

  for (int i = 0; i < m_nData; i++) {
    double x = outputWs->getSpectrum(0)->readX()[i];
    double e = exp(-x / m_muLife / 1000);
    specRe->dataY()[i] /= e;
    specIm->dataY()[i] /= e;
    specRe->dataE()[i] /= e;
    specIm->dataE()[i] /= e;
  }
}
}
}