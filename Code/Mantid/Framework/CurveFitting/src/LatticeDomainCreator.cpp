#include "MantidCurveFitting/LatticeDomainCreator.h"

#include "MantidAPI/IPeak.h"
#include "MantidAPI/LatticeDomain.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidDataObjects/Peak.h"
#include "MantidCurveFitting/PawleyFit.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Kernel;
using namespace DataObjects;

LatticeDomainCreator::LatticeDomainCreator(
    Kernel::IPropertyManager *manager, const std::string &workspacePropertyName,
    API::IDomainCreator::DomainType domainType)
    : IDomainCreator(manager,
                     std::vector<std::string>(1, workspacePropertyName),
                     domainType),
      m_workspace() {
  m_workspacePropertyName = m_workspacePropertyNames.front();
}

void LatticeDomainCreator::createDomain(
    boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> &values, size_t i0) {
  setWorkspaceFromPropertyManager();

  API::IPeaksWorkspace_sptr peaksWorkspace =
      boost::dynamic_pointer_cast<IPeaksWorkspace>(m_workspace);
  if (peaksWorkspace) {
    createDomainFromPeaksWorkspace(peaksWorkspace, domain, values, i0);
  } else {
    API::ITableWorkspace_sptr tableWorkspace =
        boost::dynamic_pointer_cast<ITableWorkspace>(m_workspace);
    if (tableWorkspace) {
      createDomainFromPeakTable(tableWorkspace, domain, values, i0);
    }
  }
}

Workspace_sptr LatticeDomainCreator::createOutputWorkspace(
    const std::string &baseName, IFunction_sptr function,
    boost::shared_ptr<FunctionDomain> domain,
    boost::shared_ptr<FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  boost::shared_ptr<LatticeDomain> latticeDomain =
      boost::dynamic_pointer_cast<LatticeDomain>(domain);

  if (!latticeDomain) {
    throw std::invalid_argument("LatticeDomain is required.");
  }

  ITableWorkspace_sptr tableWorkspace =
      WorkspaceFactory::Instance().createTable();

  if (tableWorkspace) {
    tableWorkspace->addColumn("V3D", "HKL");
    tableWorkspace->addColumn("double", "d(obs)");
    tableWorkspace->addColumn("double", "d(calc)");
    tableWorkspace->addColumn("double", "d(obs) - d(calc)");

    for (size_t i = 0; i < values->size(); ++i) {
      double dObs = values->getFitData(i);
      double dCalc = values->getCalculated(i);

      TableRow newRow = tableWorkspace->appendRow();
      newRow << (*latticeDomain)[i] << dObs << dCalc << dObs - dCalc;
    }
  }

  if (m_manager && !outputWorkspacePropertyName.empty()) {
    declareProperty(
        new WorkspaceProperty<ITableWorkspace>(outputWorkspacePropertyName, "",
                                               Kernel::Direction::Output),
        "Result workspace");

    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, tableWorkspace);
  }

  return tableWorkspace;
}

size_t LatticeDomainCreator::getDomainSize() const {
  API::IPeaksWorkspace_sptr peaksWorkspace =
      boost::dynamic_pointer_cast<IPeaksWorkspace>(m_workspace);
  if (peaksWorkspace) {
    return peaksWorkspace->getNumberPeaks();
  }

  API::ITableWorkspace_sptr tableWorkspace =
      boost::dynamic_pointer_cast<ITableWorkspace>(m_workspace);
  if (tableWorkspace) {
    return tableWorkspace->rowCount();
  }

  return 0;
}

void LatticeDomainCreator::setWorkspaceFromPropertyManager() {
  if (!m_manager) {
    throw std::invalid_argument(
        "PropertyManager not set in LatticeDomainCreator.");
  }

  try {
    m_workspace = m_manager->getProperty(m_workspacePropertyName);
  }
  catch (Kernel::Exception::NotFoundError) {
    throw std::invalid_argument(
        "Could not extract workspace from PropertyManager.");
  }
}

void LatticeDomainCreator::createDomainFromPeaksWorkspace(
    const API::IPeaksWorkspace_sptr &workspace,
    boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> &values, size_t i0) {
  if (!workspace) {
    throw std::invalid_argument(
        "This function only works on an IPeaksWorkspace-object.");
  }

  size_t peakCount = workspace->getNumberPeaks();

  if (peakCount < 1) {
    throw std::range_error("Cannot create a domain for 0 peaks.");
  }

  std::vector<V3D> hkls;
  hkls.reserve(peakCount);

  std::vector<double> dSpacings;
  dSpacings.reserve(peakCount);

  for (size_t i = 0; i < peakCount; ++i) {
    IPeak *currentPeak = workspace->getPeakPtr(static_cast<int>(i));
    hkls.push_back(currentPeak->getHKL());
    dSpacings.push_back(currentPeak->getDSpacing());
  }

  LatticeDomain *latticeDomain = new LatticeDomain(hkls);
  domain.reset(latticeDomain);

  if (!values) {
    FunctionValues *functionValues = new FunctionValues(*domain);
    values.reset(functionValues);
  } else {
    values->expand(i0 + latticeDomain->size());
  }

  values->setFitData(dSpacings);

  // Set unit weights.
  values->setFitWeights(1.0);
}

void LatticeDomainCreator::createDomainFromPeakTable(
    const ITableWorkspace_sptr &workspace,
    boost::shared_ptr<FunctionDomain> &domain,
    boost::shared_ptr<FunctionValues> &values, size_t i0) {

  size_t peakCount = workspace->rowCount();

  if (peakCount < 1) {
    throw std::range_error("Cannot create a domain for 0 peaks.");
  }

  try {
    Column_const_sptr hklColumn = workspace->getColumn("HKL");
    Column_const_sptr dColumn = workspace->getColumn("d");

    std::vector<V3D> hkls;
    hkls.reserve(peakCount);

    std::vector<double> dSpacings;
    dSpacings.reserve(peakCount);

    V3DFromHKLColumnExtractor extractor;

    for (size_t i = 0; i < peakCount; ++i) {
      try {
        hkls.push_back(extractor(hklColumn, i));

        double d = (*dColumn)[i];
        dSpacings.push_back(d);
      }
      catch (std::bad_alloc) {
        // do nothing.
      }
    }

    LatticeDomain *latticeDomain = new LatticeDomain(hkls);
    domain.reset(latticeDomain);

    if (!values) {
      FunctionValues *functionValues = new FunctionValues(*domain);
      values.reset(functionValues);
    } else {
      values->expand(i0 + latticeDomain->size());
    }

    values->setFitData(dSpacings);

    values->setFitWeights(1.0);
  }
  catch (std::runtime_error) {
    // Column does not exist
    throw std::runtime_error("Can not process table, the following columns are "
                             "required: HKL, d.");
  }
}

} // namespace CurveFitting
} // namespace Mantid
