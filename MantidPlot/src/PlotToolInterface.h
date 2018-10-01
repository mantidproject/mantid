/***************************************************************************
    File                 : PlotToolInterface.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Knut Franke, Ion Vasilief
    Email (use @ for *)  : knut.franke*gmx.de, ion_vasilief*yahoo.fr
    Description          : Interface for tools operating on a Graph

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PLOT_TOOL_INTERFACE_H
#define PLOT_TOOL_INTERFACE_H

class Graph;

/** Interface for tools operating on a Graph.
 *
 * The basic idea is to have one PlotToolInterface subclass per user-visible
 *tool operating on a Graph,
 * although there may be cases where exceptions to this rule are appropriate.
 * %Note that tools can be presented to the user via icons (like DataPickerTool)
 *or via menu entries
 * (like TranslateCurveTool); the basic concept is quite similar.
 * The main point in managing plot tools as subclasses of this class (as opposed
 *to using void pointers)
 * is the virtual destructor that allows tools to clean up after themselves.
 * Additionally, a pointer to the parent Graph (#d_graph) is managed.
 * In the future, this class may provide other generic tool functionality.
 *
 * %Note that zooming and range selection are somewhat special in that they can
 *be active in addition
 * to other tools. These are handled as special cases, while all other tools are
 *rendered mutually exclusive
 * by having Graph manage a pointer to the currently active tool
 *(Graph::d_active_tool).
 *
 * It would be nice for some of the plot tools (like TranslateCurveTool or
 *MultiPeakFitTool) to send a signal
 * when they are finished and to generalize the statusText signal provided by
 *most tools, but having
 * PlotToolInterface inherit from QObject would make it impossible for
 * plot tools to also inherit from other QObject descendants (such as
 *QwtPlotPicker).
 * As a workaround, plot tools can call Graph::setActiveTool(), carefully noting
 *that they are deleted
 * during this call.
 *
 * Currently, plot tools are instantiated by ApplicationWindow and handed to the
 *Graph in question;
 * this scheme will have to be revised for dynamically adding new tools via
 *plugins.
 */
class PlotToolInterface {
public:
  enum RttiValues {
    Rtti_PlotTool = 0,

    Rtti_RangeSelector,
    Rtti_DataPicker,
    Rtti_TranslateCurveTool,
    Rtti_MultiPeakFitTool,
    Rtti_LineProfileTool,
    Rtti_SelectPeakTool,
    Rtti_LabelTool,

    Rtti_PlotUserTool = 1000
  };

  explicit PlotToolInterface(Graph *graph) { d_graph = graph; }
  virtual ~PlotToolInterface(){};

  virtual int rtti() const { return Rtti_PlotTool; };
  Graph *graph() const { return d_graph; }

protected:
  Graph *d_graph;
};

#endif // ifndef PLOT_TOOL_INTERFACE_H
