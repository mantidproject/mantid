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
    PlotAxis::PlotAxis(const Mantid::API::IMDWorkspace &workspace,
                       const size_t index)
      : m_title()
    {
      if (index < workspace.getNumDims()) titleFromIndex(workspace, index);
      else
        throw std::invalid_argument("PlotAxis() - Unknown axis index: '" + \
                                    boost::lexical_cast<std::string>(index) + "'");
    }

    /**
     * The title will be filled with the caption & units from the given
     * dimension object
     * @param dim A dimension object, usually from a workspace
     */
    PlotAxis::PlotAxis(const Mantid::Geometry::IMDDimension &dim)
    {
      titleFromDimension(dim);
    }

    /**
     * The title will be filled with the caption & label for the Y data values
     * within the workspace, ~ the Z axis
     * @param plottingDistribution If true, the Y axis has been divided by the bin width
     * @param workspace The workspace containing the Y title information
     */
    PlotAxis::PlotAxis(const bool plottingDistribution, const Mantid::API::MatrixWorkspace &workspace)
    {
      titleFromYData(workspace, plottingDistribution);
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
    void PlotAxis::titleFromIndex(const Mantid::API::IMDWorkspace &workspace,
                                  const size_t index)
    {
      auto dim = workspace.getDimension(index);
      titleFromDimension(*dim);
      if(m_title.isEmpty())
      {
        m_title = (index == 0) ? "X axis" : "Y axis";
      }
    }

    /**
     * @param dim A dimension object that encapsulates the axis data
     */
    void PlotAxis::titleFromDimension(const Mantid::Geometry::IMDDimension &dim)
    {
      m_title = QString::fromStdString(dim.getName());
      if(!m_title.isEmpty())
      {
        auto unitLbl = dim.getUnits();
        if ( !unitLbl.utf8().empty() )
        {
          m_title += " (" + QString::fromStdWString(unitLbl.utf8()) + ")";
        }
        else if(!unitLbl.ascii().empty())
        {
          m_title += " (" + QString::fromStdString(unitLbl.ascii()) + ")";
        }
      }
    }

    /**
     * Constructs a title using the unicode methods of the UnitLabel
     * @param workspace The workspace containing the Y title information
     * @param plottingDistribution If true, the Y axis has been divided by the bin width
     */
    void PlotAxis::titleFromYData(const Mantid::API::MatrixWorkspace &workspace,
                                  const bool plottingDistribution)
    {
      // The workspace can have a custom label so we should use that as a preference.
      // The workspace.YUnitLabel does some mangling of the string if the user set no
      // custom label. See MatrixWorkspace::YUnitLabel
      
      const std::string customLabel = workspace.YUnitLabel();
      const std::string yunit = workspace.YUnit();
      if((yunit == customLabel) || (customLabel.find("per") != std::string::npos))
      {
        m_title = QString::fromStdString(yunit);
        if(plottingDistribution && workspace.axes() > 0 && workspace.getAxis(0)->unit())
        {
          const auto xunit = workspace.getAxis(0)->unit();
          const auto lbl = xunit->label();
          if(!lbl.utf8().empty())
          {
            m_title += " (" + QString::fromStdWString(lbl.utf8()) + QString::fromWCharArray(L"\u207b\u00b9)");
          }
        }
      }
      else
      {
        m_title = QString::fromStdString(customLabel);
      }
    }

  } // namespace API
} // namespace MantidQt
