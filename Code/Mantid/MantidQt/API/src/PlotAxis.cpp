#include "MantidQtAPI/PlotAxis.h"
#include <QStringBuilder>

namespace MantidQt
{
  namespace API
  {

    /**
     * @param workspace A pointer to a MatrixWorkspace object
     * @param index Index of the axis in the workspace to inspect
     */
    PlotAxis::PlotAxis(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                       const size_t index)
      : m_title()
    {
      if (index == 0 || index == 1) titleFromIndex(workspace, index);
      else
        throw std::invalid_argument("PlotAxis() - Unknown axis index: '" + \
                                    boost::lexical_cast<std::string>(index) + "'");
    }

    /**
     * @return A new title for this axis
     */
    QString PlotAxis::title() const
    {
      return m_title;
    }

    /**
     * @param workspace A pointer to a MatrixWorkspace object
     * @param index Index of the axis in the workspace to inspect
     */
    void PlotAxis::titleFromIndex(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                  const size_t index)
    {
      // Deal with axis names
      Mantid::API::Axis* ax = workspace->getAxis(index);
      m_title = "";
      if ( ax->isSpectra() ) m_title = "Spectrum Number";
      else if (ax->unit() && ax->unit()->unitID() != "Empty" )
      {
        m_title = QString::fromStdString(ax->unit()->caption());
        const auto lbl = ax->unit()->label();
        if ( !lbl.utf8().empty() )
        {
          m_title += " (" + QString::fromStdWString(lbl.utf8()) + ")";
        }
      }
      else if (!ax->title().empty())
      {
        m_title = QString::fromStdString(ax->title());
      }
      else
      {
        m_title = (index == 0) ? "X axis" : "Y axis";
      }
    }

    /**
     * @param workspace A pointer to a MatrixWorkspace object
     */
    void PlotAxis::initYAxisTitle(const Mantid::API::MatrixWorkspace_const_sptr &workspace)
    {
      m_title = QString::fromStdString(workspace->YUnitLabel());
      Mantid::API::Axis* ax = workspace->getAxis(0);
      if (workspace->isDistribution() && ax->unit())
      {
        const std::string unitID = ax->unit()->unitID();
        if (unitID != "" || unitID != "Empty")
        {
          m_title = m_title % " / " % ax->unit()->label().ascii().c_str();
        }
      }
    }

  } // namespace API
} // namespace MantidQt
