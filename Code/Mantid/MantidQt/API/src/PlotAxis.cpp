#include "MantidQtAPI/PlotAxis.h"

namespace MantidQt
{
  namespace API
  {

    /**
     * The title will be filled with the caption & units from the given Axis object
     * of the workspace
     * @param workspace The workspace containing the axis information
     * @param index Index of the axis in the workspace to inspect
     */
    PlotAxis::PlotAxis(const Mantid::API::MatrixWorkspace &workspace,
                       const size_t index)
      : m_title()
    {
      if (index == 0 || index == 1) titleFromIndex(workspace, index);
      else
        throw std::invalid_argument("PlotAxis() - Unknown axis index: '" + \
                                    boost::lexical_cast<std::string>(index) + "'");
    }

    /**
     * The title will be filled with the caption & label for the Y data values
     * within the workspace, ~ the Z axis
     * @param workspace  The workspace containing the Y title information
     */
    PlotAxis::PlotAxis(const Mantid::API::MatrixWorkspace &workspace)
    {
      titleFromYData(workspace);
    }

    /**
     * @return A new title for this axis
     */
    QString PlotAxis::title() const
    {
      return m_title;
    }

    /**
     * @param workspace  The workspace containing the axis information
     * @param index Index of the axis in the workspace to inspect
     */
    void PlotAxis::titleFromIndex(const Mantid::API::MatrixWorkspace &workspace,
                                  const size_t index)
    {
      // Deal with axis names
      Mantid::API::Axis* ax = workspace.getAxis(index);
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
        else if(!lbl.ascii().empty())
        {
          m_title += " (" + QString::fromStdString(lbl.ascii()) + ")";
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
     * Constructs a title using the unicode methods of the UnitLabel
     * @param workspace The workspace containing the Y title information
     */
    void PlotAxis::titleFromYData(const Mantid::API::MatrixWorkspace &workspace)
    {
      m_title = QString::fromStdString(workspace.YUnit());
      if(workspace.isDistribution() && workspace.axes() > 0 && workspace.getAxis(0)->unit())
      {
        const auto xunit = workspace.getAxis(0)->unit();
        const auto lbl = xunit->label();
        if(!lbl.utf8().empty())
        {
          m_title += " (" + QString::fromStdWString(lbl.utf8()) + QString::fromWCharArray(L"\u207b\u00b9)");
        }
      }
    }

  } // namespace API
} // namespace MantidQt
