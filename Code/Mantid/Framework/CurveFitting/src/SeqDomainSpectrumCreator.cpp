#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidAPI/Workspace.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/FunctionDomain1DSpectrumCreator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid
{
namespace CurveFitting
{

using namespace API;

SeqDomainSpectrumCreator::SeqDomainSpectrumCreator(Kernel::IPropertyManager *manager, const std::string &workspacePropertyName) :
    IDomainCreator(manager, std::vector<std::string>(1, workspacePropertyName), SeqDomainSpectrumCreator::Sequential),
    m_matrixWorkspace()
{
    m_workspacePropertyName = m_workspacePropertyNames.front();
}

void SeqDomainSpectrumCreator::createDomain(boost::shared_ptr<FunctionDomain> &domain, boost::shared_ptr<FunctionValues> &values, size_t i0)
{
    setParametersFromPropertyManager();

    if(!m_matrixWorkspace) {
        throw std::invalid_argument("No matrix workspace assigned - can not create domain.");
    }

    SeqDomain *seqDomain = SeqDomain::create(m_domainType);

    size_t numberOfHistograms = m_matrixWorkspace->getNumberHistograms();
    for(size_t i = 0; i < numberOfHistograms; ++i) {
        FunctionDomain1DSpectrumCreator *spectrumDomain = new FunctionDomain1DSpectrumCreator;
        spectrumDomain->setMatrixWorkspace(m_matrixWorkspace);
        spectrumDomain->setWorkspaceIndex(i);

        seqDomain->addCreator(IDomainCreator_sptr(spectrumDomain));
    }

    domain.reset(seqDomain);

    if(!values) {
        values.reset(new FunctionValues(*domain));
    } else {
        values->expand(i0 + domain->size());
    }

}

Workspace_sptr SeqDomainSpectrumCreator::createOutputWorkspace(const std::string &baseName, IFunction_sptr function, boost::shared_ptr<FunctionDomain> domain, boost::shared_ptr<FunctionValues> values, const std::string &outputWorkspacePropertyName)
{
    boost::shared_ptr<SeqDomain> seqDomain = boost::dynamic_pointer_cast<SeqDomain>(domain);

    if(!seqDomain) {
        throw std::invalid_argument("CreateOutputWorkspace requires SeqDomain.");
    }

    MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create(m_matrixWorkspace));

    for(size_t i = 0; i < seqDomain->getNDomains(); ++i) {
        FunctionDomain_sptr localDomain;
        FunctionValues_sptr localValues;

        seqDomain->getDomainAndValues(i, localDomain, localValues);
        function->function(*localDomain, *localValues);

        const MantidVec& originalXValue = m_matrixWorkspace->readX(i);
        MantidVec& xValues = outputWs->dataX(i);
        MantidVec& yValues = outputWs->dataY(i);

        for(size_t j = 0; j < yValues.size(); ++j) {
            xValues[j] = originalXValue[j];
            yValues[j] = localValues->getCalculated(j);
        }
    }

    if(m_manager && !outputWorkspacePropertyName.empty()) {
        declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputWorkspacePropertyName, "", Kernel::Direction::Output),
                        "Result workspace");

        m_manager->setPropertyValue(outputWorkspacePropertyName, baseName + "Workspace");
        m_manager->setProperty(outputWorkspacePropertyName, outputWs);
    }

    return outputWs;
}

size_t SeqDomainSpectrumCreator::getDomainSize() const
{
    if(!m_matrixWorkspace) {
        throw std::invalid_argument("No matrix workspace assigned.");
    }

    size_t nHist = m_matrixWorkspace->getNumberHistograms();
    size_t totalSize = 0;

    for(size_t i = 0; i < nHist; ++i) {
        totalSize += m_matrixWorkspace->dataY(i).size();
    }

    return totalSize;
}

void SeqDomainSpectrumCreator::setParametersFromPropertyManager()
{
    if(m_manager) {
        Workspace_sptr workspace = m_manager->getProperty(m_workspacePropertyName);

        setMatrixWorkspace(boost::dynamic_pointer_cast<MatrixWorkspace>(workspace));
    }
}

void SeqDomainSpectrumCreator::setMatrixWorkspace(MatrixWorkspace_sptr matrixWorkspace)
{
    if(!matrixWorkspace) {
        throw std::invalid_argument("InputWorkspace must be a valid MatrixWorkspace.");
    }

    m_matrixWorkspace = matrixWorkspace;
}



} // namespace CurveFitting
} // namespace Mantid
