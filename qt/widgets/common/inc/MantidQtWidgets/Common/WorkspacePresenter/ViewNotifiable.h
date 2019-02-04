// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_VIEWNOTIFIABLE_H_
#define MANTID_MANTIDWIDGETS_VIEWNOTIFIABLE_H_
/**
\class  ViewNotifiable
\brief  Mixin class allows view notifications
\author Lamar Moore
\date   24-08-2016
\version 1.0
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