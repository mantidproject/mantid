// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <QDir>
#include <QStringList>
#include <fstream>
#include <vector>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MdPlottingCmapsProvider.h"

#ifdef MAKE_VATES
#include "vtkNew.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtk_jsoncpp.h"
#endif

namespace {
/// Static logger
Mantid::Kernel::Logger g_log("MdViewerWidget");
} // namespace

MdPlottingCmapsProvider::MdPlottingCmapsProvider() {}

MdPlottingCmapsProvider::~MdPlottingCmapsProvider() {}

void MdPlottingCmapsProvider::getColorMapsForMdPlotting(
    QStringList &colorMapNames, QStringList &colorMapFiles) {
  // Get the installed color maps directory.
  QString colorMapDirectory = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "colormaps.directory"));
  if (colorMapDirectory.isEmpty()) {
    return;
  }

  // We show only those color maps as options which can be found in the .map
  // files and in the .xml files of the VSI
  QStringList colorMapNamesSliceViewer;
  QStringList colorMapFilesSliceViewer;
  appendAllFileNamesForFileType(colorMapNamesSliceViewer,
                                colorMapFilesSliceViewer, colorMapDirectory,
                                "map");

  QStringList colorMapNamesVsi;
  getColorMapsForVSI(colorMapNamesVsi);

  std::vector<int> indexList = getSliceViewerIndicesForCommonColorMaps(
      colorMapNamesSliceViewer, colorMapNamesVsi);

  for (int &it : indexList) {
    colorMapNames.append(colorMapNamesSliceViewer[it]);
    colorMapFiles.append(colorMapFilesSliceViewer[it]);
  }
}

void MdPlottingCmapsProvider::getColorMapsForVSI(QStringList &colorMapNames) {
#ifdef MAKE_VATES
  vtkNew<vtkSMTransferFunctionPresets> presets;

  // Check for colormap "hot". If preset, assume custom colormaps have
  // already been loaded.
  auto viridisColormap = presets->GetFirstPresetWithName("hot");
  if (viridisColormap.empty()) {
    const std::string filenames[3] = {"All_slice_viewer_cmaps_for_vsi.json",
                                      "All_idl_cmaps.json",
                                      "All_mpl_cmaps.json"};
    const std::string colorMapDirectory =
        Mantid::Kernel::ConfigService::Instance().getString(
            "colormaps.directory");
    for (const auto &baseName : filenames) {
      std::string colorMap = colorMapDirectory + baseName;
      presets->ImportPresets(colorMap.c_str());
    }
  }

  unsigned int numberOfPresets = presets->GetNumberOfPresets();
  for (unsigned int i = 0; i < numberOfPresets; ++i) {
    colorMapNames.append(QString::fromStdString(presets->GetPresetName(i)));
  }
#else
  (void)colorMapNames;
#endif
}

void MdPlottingCmapsProvider::appendAllFileNamesForFileType(
    QStringList &colorMapNames, QStringList &colorMapFiles,
    QString colorMapDirectory, QString fileType) {
  QDir directory(colorMapDirectory);

  QStringList filter(QString("*.%1").arg(fileType));

  QFileInfoList info = directory.entryInfoList(filter, QDir::Files);

  for (const auto &fileInfo : info) {
    colorMapNames.append(fileInfo.baseName());
    colorMapFiles.append(fileInfo.absoluteFilePath());
  }
}

std::vector<int>
MdPlottingCmapsProvider::getSliceViewerIndicesForCommonColorMaps(
    QStringList colorMapNamesSliceViewer, QStringList colorMapNamesVsi) {
  int index = 0;

  std::vector<int> indexVector;

  for (const auto &colorMapName : colorMapNamesSliceViewer) {
    if (colorMapNamesVsi.indexOf(colorMapName) != -1) {
      indexVector.push_back(index);
    }

    index++;
  }

  return indexVector;
}
