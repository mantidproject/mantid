#include "MantidCurveFitting/FunctionDomain1DSpectrumCreator.h"

namespace Mantid
{
namespace CurveFitting
{

using namespace API;

FunctionDomain1DSpectrumCreator::FunctionDomain1DSpectrumCreator() :
    IDomainCreator(NULL, std::vector<std::string>(), FunctionDomain1DSpectrumCreator::Simple),
    m_matrixWorkspace(),
    m_workspaceIndex(0),
    m_workspaceIndexIsSet(false)
{
}

void FunctionDomain1DSpectrumCreator::setMatrixWorkspace(MatrixWorkspace_sptr matrixWorkspace)
{
    m_matrixWorkspace = matrixWorkspace;
}

void FunctionDomain1DSpectrumCreator::setWorkspaceIndex(size_t workspaceIndex)
{
    m_workspaceIndex = workspaceIndex;

    m_workspaceIndexIsSet = true;
}

void FunctionDomain1DSpectrumCreator::createDomain(boost::shared_ptr<FunctionDomain> &domain, boost::shared_ptr<FunctionValues> &values, size_t i0)
{
    throwIfWorkspaceInvalid();

    if(m_matrixWorkspace->isHistogramData()) {
        domain.reset(new FunctionDomain1DSpectrum(m_workspaceIndex, getVectorHistogram()));
    } else {
        domain.reset(new FunctionDomain1DSpectrum(m_workspaceIndex, getVectorNonHistogram()));
    }

    if(!values) {
        values.reset(new FunctionValues(*domain));
    } else {
        values->expand(i0 + domain->size());
    }
}

size_t FunctionDomain1DSpectrumCreator::getDomainSize() const
{
    throwIfWorkspaceInvalid();

    size_t numberOfXValues = m_matrixWorkspace->readX(m_workspaceIndex).size();

    if(m_matrixWorkspace->isHistogramData()) {
        return numberOfXValues - 1;
    }

    return numberOfXValues;
}

void FunctionDomain1DSpectrumCreator::throwIfWorkspaceInvalid() const
{
    if(!m_matrixWorkspace) {
        throw std::invalid_argument("No matrix workspace assigned or does not contain histogram data - cannot create domain.");
    }

    if(!m_workspaceIndexIsSet || m_workspaceIndex > m_matrixWorkspace->getNumberHistograms()) {
        throw std::invalid_argument("Workspace index has not been set or is invalid.");
    }
}

MantidVec FunctionDomain1DSpectrumCreator::getVectorHistogram() const
{
    const MantidVec wsXData = m_matrixWorkspace->readX(m_workspaceIndex);
    size_t wsXSize = wsXData.size();

    if(wsXSize < 2) {
        throw std::invalid_argument("Histogram Workspace2D with less than two x-values cannot be processed.");
    }

    MantidVec x(wsXSize - 1);

    for(size_t i = 0; i < x.size(); ++i) {
        x[i] = (wsXData[i] + wsXData[i + 1]) / 2.0;
    }

    return x;
}

MantidVec FunctionDomain1DSpectrumCreator::getVectorNonHistogram() const
{
    const MantidVec wsXData = m_matrixWorkspace->readX(m_workspaceIndex);
    size_t wsXSize = wsXData.size();

    if(wsXSize < 1) {
        throw std::invalid_argument("Workspace2D with less than one x-value cannot be processed.");
    }

    return MantidVec(wsXData);
}

} // namespace CurveFitting
} // namespace Mantid
