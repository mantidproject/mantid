#ifndef MANTID_MANTIDWIDGETS_VIEWNOTIFIABLE_H_
#define MANTID_MANTIDWIDGETS_VIEWNOTIFIABLE_H_
/**
\class  ViewNotifiable
\brief  Mixin class allows view notifications
\author Lamar Moore
\date   24-08-2016
\version 1.0


Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
namespace MantidQt {
namespace MantidWidgets {

class ViewNotifiable {
public:
  virtual ~ViewNotifiable() {}

  enum class Flag {
    LoadWorkspace,
    LoadLiveDataWorkspace,
    RenameWorkspace,
    DeleteWorkspaces,
    GroupWorkspaces,
    UngroupWorkspaces,
    SortWorkspaces,
    SaveSingleWorkspace,
    SaveWorkspaceCollection,
    FilterWorkspaces,
    PopulateAndShowWorkspaceContextMenu,
    SaveToProgram,
    ShowWorkspaceData,
    ShowInstrumentView,
    PlotSpectrum,
    PlotSpectrumWithErrors,
    PlotSpectrumAdvanced,
    ShowColourFillPlot,
    ShowDetectorsTable,
    ShowBoxDataTable,
    ShowVatesGUI,
    ShowMDPlot,
    ShowListData,
    ShowSpectrumViewer,
    ShowSliceViewer,
    ShowLogs,
    ShowSampleMaterialWindow,
    ShowAlgorithmHistory,
    ShowTransposed,
    ConvertToMatrixWorkspace,
    ConvertMDHistoToMatrixWorkspace,
    ClearUBMatrix,
    RefreshWorkspaces
  };

  virtual void notifyFromView(Flag flag) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTID_MANTIDWIDGETS_VIEWNOTIFIABLE_H_