#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view)
    : m_view(view), m_data(), m_sections(), m_correctedData()
  {}

  void ALCBaselineModellingPresenter::initialize()
  {
    m_view->initialize();

    connect(m_view, SIGNAL(fit()), this, SLOT(fit()));
    connect(m_view, SIGNAL(addSection(Section)), this, SLOT(addSection(Section)));
    connect(m_view, SIGNAL(modifySection(SectionIndex, Section)),
              this, SLOT(modifySection(SectionIndex, Section)));
  }

  void ALCBaselineModellingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    m_data = data;
    m_view->setData(data);
  }

  void ALCBaselineModellingPresenter::fit()
  {
    IFunction_sptr funcToFit =
        FunctionFactory::Instance().createInitialized( m_view->function()->asString() );

    MatrixWorkspace_sptr wsToFit = filteredData();

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", funcToFit);
    fit->setProperty("InputWorkspace", wsToFit);
    fit->setProperty("CreateOutput", true);
    fit->execute();

    MatrixWorkspace_sptr fitOutput = fit->getProperty("OutputWorkspace");

    IAlgorithm_sptr extract = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
    extract->setChild(true);
    extract->setProperty("InputWorkspace", fitOutput);
    extract->setProperty("WorkspaceIndex", 2);
    extract->setProperty("OutputWorkspace", "__NotUsed__");
    extract->execute();

    m_correctedData = extract->getProperty("OutputWorkspace");

    m_view->setFunction(funcToFit);
    m_view->setCorrectedData(m_correctedData);
  }

  void ALCBaselineModellingPresenter::addSection(Section newSection)
  {
    m_sections.push_back(newSection);
    m_view->setSections(m_sections);
  }

  void ALCBaselineModellingPresenter::modifySection(SectionIndex index, Section modified)
  {
    if (index >= m_sections.size())
    {
      throw std::out_of_range("Section index");
    }

    m_sections[index] = modified;
    m_view->setSections(m_sections);
  }

  MatrixWorkspace_sptr ALCBaselineModellingPresenter::filteredData() const
  {
    // Assumptions about data
    assert(m_data);
    assert(m_data->getNumberHistograms() == 1);
    assert(!m_data->isHistogramData()); // Point data expected

    // Whether point with particular index should be disabled
    std::vector<bool> toDisable(m_data->blocksize(), true);

    // Find points which are in at least one section, and exclude them from disable list
    for (size_t i = 0; i < m_data->blocksize(); ++i)
    {
      for (auto it = m_sections.begin(); it != m_sections.end(); ++it)
      {
        if ( m_data->dataX(0)[i] >= it->first && m_data->dataX(0)[i] <= it->second )
        {
          toDisable[i] = false;
          break; // No need to check other sections
        }
      }
    }

    // Create a copy of the data
    IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
    clone->setChild(true);
    clone->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    clone->setProperty("OutputWorkspace", "__NotUsed__");
    clone->execute();

    Workspace_sptr cloned = clone->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(cloned);
    assert(ws); // CloneWorkspace should return the same type of workspace

    // XXX: Points are disabled by settings their errors to very high value. This makes those
    //      points to have very low weights during the fitting, effectively disabling them.

    const double DISABLED_ERR = std::numeric_limits<double>::max();

    // Disable chosen points
    for (size_t i = 0; i < ws->blocksize(); ++i)
    {
      if (toDisable[i])
      {
        ws->dataE(0)[i] = DISABLED_ERR;
      }
    }

    return ws;
  }

} // namespace CustomInterfaces
} // namespace MantidQt
