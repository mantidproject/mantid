//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GeneralisedSecondDifference.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <numeric>
#include <sstream>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GeneralisedSecondDifference)

using namespace Kernel;
using namespace API;

/// Constructor
GeneralisedSecondDifference::GeneralisedSecondDifference()
    : Algorithm(), m_Cij(0), m_Cij2(0), m_z(0), m_m(0) {}

/// Destructor
GeneralisedSecondDifference::~GeneralisedSecondDifference() = default;

/// Initialisation method.
void GeneralisedSecondDifference::init() {

  // Input and output workspaces
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                      Direction::Output),
      "The name of the workspace to be created as the output of the algorithm");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "M", 0, mustBePositive,
      "The number of points for averaging, i.e. summing will be done in the\n"
      "range [y(i-m),y(i+m)]");
  declareProperty("Z", 0, mustBePositive,
                  "The number of iteration steps in the averaging procedure");
  declareProperty("WorkspaceIndexMin", 0, mustBePositive,
                  "Lower bound of the spectrum range (default 0)");
  declareProperty("WorkspaceIndexMax", 0, mustBePositive,
                  "Upper bound of the spectrum range (default workspace max)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void GeneralisedSecondDifference::exec() {
  // Message stream used for logger
  std::ostringstream message;

  // Get some properties
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int spec_min = getProperty("WorkspaceIndexMin");
  int spec_max = getProperty("WorkspaceIndexMax");
  int n_hists = static_cast<int>(inputWS->getNumberHistograms());

  if (spec_min == 0 && spec_max == 0) // Values per default, take all spectra
    spec_max = n_hists - 1;

  if (spec_min > spec_max)
    std::swap(spec_min, spec_max);

  if (spec_max > n_hists) {
    message << "WorkspaceIndexMax " << spec_max
            << " > number of histograms, WorkspaceIndexMax reset to "
            << (n_hists - 1);
    g_log.information(message.str());
    message.str("");
    spec_max = n_hists - 1;
  }

  // Get some more input fields
  m_z = getProperty("Z");
  m_m = getProperty("M");
  const int n_av = m_z * m_m + 1;

  // Calculate the Cij and Cij^2 coefficients
  computePrefactors();

  const int n_specs = spec_max - spec_min + 1;
  const int n_points = static_cast<int>(inputWS->dataY(0).size()) - 2 * n_av;
  if (n_points < 1) {
    throw std::invalid_argument("Invalid (M,Z) values");
  }
  // Create OuputWorkspace
  MatrixWorkspace_sptr out = WorkspaceFactory::Instance().create(
      inputWS, n_specs, n_points + 1, n_points);

  const int nsteps = 2 * n_av + 1;

  boost::shared_ptr<API::Progress> progress =
      boost::make_shared<API::Progress>(this, 0.0, 1.0, (spec_max - spec_min));
  for (int i = spec_min; i <= spec_max; i++) {
    int out_index = i - spec_min;
    out->getSpectrum(out_index)
        ->setSpectrumNo(inputWS->getSpectrum(i)->getSpectrumNo());
    const MantidVec &refX = inputWS->readX(i);
    const MantidVec &refY = inputWS->readY(i);
    const MantidVec &refE = inputWS->readE(i);
    MantidVec &outX = out->dataX(out_index);
    MantidVec &outY = out->dataY(out_index);
    MantidVec &outE = out->dataE(out_index);

    std::copy(refX.begin() + n_av, refX.end() - n_av, outX.begin());
    auto itInY = refY.cbegin();
    auto itOutY = outY.begin();
    auto itInE = refE.cbegin();
    auto itOutE = outE.begin();
    for (; itOutY != outY.end(); ++itOutY, ++itInY, ++itOutE, ++itInE) {
      // Calculate \sum_{j}Cij.Y(j)
      (*itOutY) = std::inner_product(itInY, itInY + nsteps, m_Cij.begin(), 0.0);
      // Calculate the error bars sqrt(\sum_{j}Cij^2.E^2)
      double err2 =
          std::inner_product(itInE, itInE + nsteps, m_Cij2.cbegin(), 0.0);
      (*itOutE) = sqrt(err2);
    }
    progress->report();
  }
  setProperty("OutputWorkspace", out);

  return;
}
/** Compute the Cij
 *
 */
void GeneralisedSecondDifference::computePrefactors() {
  int zz = 0;
  int max_index_prev = 1;
  int n_el_prev = 3;
  std::vector<double> previous(n_el_prev);
  previous[0] = 1;
  previous[1] = -2;
  previous[2] = 1;

  if (m_z == 0) //
  {
    m_Cij.resize(3);
    std::copy(previous.begin(), previous.end(), m_Cij.begin());
    m_Cij2.resize(3);
    std::transform(m_Cij.begin(), m_Cij.end(), m_Cij2.begin(),
                   VectorHelper::Squares<double>());
    return;
  }
  std::vector<double> next;
  // Calculate the Cij iteratively.
  do {
    zz++;
    int max_index = zz * m_m + 1;
    int n_el = 2 * max_index + 1;
    next.resize(n_el);
    std::fill(next.begin(), next.end(), 0.0);
    for (int i = 0; i < n_el; ++i) {
      int delta = -max_index + i;
      for (int l = delta - m_m; l <= delta + m_m; l++) {
        int index = l + max_index_prev;
        if (index >= 0 && index < n_el_prev)
          next[i] += previous[index];
      }
    }
    previous.resize(n_el);
    std::copy(next.begin(), next.end(), previous.begin());
    max_index_prev = max_index;
    n_el_prev = n_el;
  } while (zz != m_z);

  m_Cij.resize(2 * m_z * m_m + 3);
  std::copy(previous.begin(), previous.end(), m_Cij.begin());
  m_Cij2.resize(2 * m_z * m_m + 3);
  std::transform(m_Cij.begin(), m_Cij.end(), m_Cij2.begin(),
                 VectorHelper::Squares<double>());
  return;
}

} // namespace Algorithm
} // namespace Mantid
