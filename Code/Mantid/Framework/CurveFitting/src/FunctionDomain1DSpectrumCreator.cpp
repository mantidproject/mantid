#include "MantidCurveFitting/FunctionDomain1DSpectrumCreator.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;

/**
 * Default Constructor.
 */
FunctionDomain1DSpectrumCreator::FunctionDomain1DSpectrumCreator()
    : IDomainCreator(NULL, std::vector<std::string>(),
                     FunctionDomain1DSpectrumCreator::Simple),
      m_matrixWorkspace(), m_workspaceIndex(0), m_workspaceIndexIsSet(false) {}

/**
 * Sets the matrix workspace this creator is working with.
 * @param matrixWorkspace :: Pointer to matrix workspace the domain is based on.
 */
void FunctionDomain1DSpectrumCreator::setMatrixWorkspace(
    MatrixWorkspace_sptr matrixWorkspace) {
  m_matrixWorkspace = matrixWorkspace;
}

/**
 * Sets the workspace index for the created domain. Does not perform any
 * validity checks.
 * @param workspaceIndex :: Index of spectrum for which the domain is created.
 */
void FunctionDomain1DSpectrumCreator::setWorkspaceIndex(size_t workspaceIndex) {
  m_workspaceIndex = workspaceIndex;

  m_workspaceIndexIsSet = true;
}

/**
 * Creates the domain according to the stored MatrixWorkspace and workspace
 *index.
 *
 * This method constructs a FunctionDomain1DSpectrum instance and set the domain
 *and values parameters.
 * If no workspace is present, the index is invalid or there are too few bins
 *(0), the method throws std::invalid_argument.
 *
 * @param domain :: Pointer that holds the domain instance after the call.
 * @param values :: Pointer that holds the function values instance after the
 *call.
 * @param i0 :: Size offset in case the FunctionValues object already contains
 *data.
 */
void FunctionDomain1DSpectrumCreator::createDomain(
    boost::shared_ptr<FunctionDomain> &domain,
    boost::shared_ptr<FunctionValues> &values, size_t i0) {
  throwIfWorkspaceInvalid();

  if (m_matrixWorkspace->isHistogramData()) {
    domain.reset(
        new FunctionDomain1DSpectrum(m_workspaceIndex, getVectorHistogram()));
  } else {
    domain.reset(new FunctionDomain1DSpectrum(m_workspaceIndex,
                                              getVectorNonHistogram()));
  }

  if (!values) {
    values.reset(new FunctionValues(*domain));
  } else {
    values->expand(i0 + domain->size());
  }

  const MantidVec &yData = m_matrixWorkspace->readY(m_workspaceIndex);
  const MantidVec &eData = m_matrixWorkspace->readE(m_workspaceIndex);

  for (size_t i = 0; i < yData.size(); ++i) {
    values->setFitData(i, yData[i]);
    values->setFitWeight(i, 1.0 / (eData[i] != 0 ? eData[i] : 1.0));
  }
}

/**
 * Returns the domain sizes.
 *
 * The function throws std::invalid_argument if the workspace, or the index are
 *not valid.
 *
 * @return Domain size, depending on whether the MatrixWorkspace contains
 *histogram data or not.
 */
size_t FunctionDomain1DSpectrumCreator::getDomainSize() const {
  throwIfWorkspaceInvalid();

  size_t numberOfXValues = m_matrixWorkspace->readX(m_workspaceIndex).size();

  if (m_matrixWorkspace->isHistogramData()) {
    return numberOfXValues - 1;
  }

  return numberOfXValues;
}

/// Checks the assigned MatrixWorkspace and workspace index.
void FunctionDomain1DSpectrumCreator::throwIfWorkspaceInvalid() const {
  if (!m_matrixWorkspace) {
    throw std::invalid_argument("No matrix workspace assigned or does not "
                                "contain histogram data - cannot create "
                                "domain.");
  }

  if (!m_workspaceIndexIsSet ||
      m_workspaceIndex >= m_matrixWorkspace->getNumberHistograms()) {
    throw std::invalid_argument(
        "Workspace index has not been set or is invalid.");
  }
}

/// Returns vector with x-values of spectrum if MatrixWorkspace contains
/// histogram data.
MantidVec FunctionDomain1DSpectrumCreator::getVectorHistogram() const {
  const MantidVec wsXData = m_matrixWorkspace->readX(m_workspaceIndex);
  size_t wsXSize = wsXData.size();

  if (wsXSize < 2) {
    throw std::invalid_argument("Histogram Workspace2D with less than two "
                                "x-values cannot be processed.");
  }

  MantidVec x(wsXSize - 1);

  for (size_t i = 0; i < x.size(); ++i) {
    x[i] = (wsXData[i] + wsXData[i + 1]) / 2.0;
  }

  return x;
}

/// Returns vector with x-values of spectrum if MatrixWorkspace does not contain
/// histogram data.
MantidVec FunctionDomain1DSpectrumCreator::getVectorNonHistogram() const {
  const MantidVec wsXData = m_matrixWorkspace->readX(m_workspaceIndex);
  size_t wsXSize = wsXData.size();

  if (wsXSize < 1) {
    throw std::invalid_argument(
        "Workspace2D with less than one x-value cannot be processed.");
  }

  return MantidVec(wsXData);
}

} // namespace CurveFitting
} // namespace Mantid
