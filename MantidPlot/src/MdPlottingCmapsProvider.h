// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDPLOTTINGCMAPSPROVIDER_H_
#define MDPLOTTINGCMAPSPROVIDER_H_

#include <QString>
#include <vector>

class QStringList;

/**
  *
  This helper class allows for reading and processing the names of the available
  MD color map files

  @date 7/1/2015
  */

class MdPlottingCmapsProvider {
public:
  MdPlottingCmapsProvider();

  ~MdPlottingCmapsProvider();

  /**
   * Get the name of all available color maps for general MD plotting.
   * @param colorMapNames Reference to a list with the names of the color maps.
   * @param colorMapFiles Reference to a corresponding list with the file paths
   * for the color maps.
   */
  void getColorMapsForMdPlotting(QStringList &colorMapNames,
                                 QStringList &colorMapFiles);

  /**
   * Get the name of all available color maps for the VSI (at least the ones
   * stored in files).
   * @param colorMapNames Reference to a list with the names of the color maps.
   */
  void getColorMapsForVSI(QStringList &colorMapNames);

private:
  /**
   * Gets all files from directory of a given file type.
   * @param colorMapNames Reference to a list of color map names.
   * @param colorMapFiles Reference to a list of file paths for the
   * corresponding color maps.
   * @param colorMapDirectory Directory where the color maps are stored.
   * @param fileType suffix of the desired files.
   */
  void appendAllFileNamesForFileType(QStringList &colorMapNames,
                                     QStringList &colorMapFiles,
                                     QString colorMapDirectory,
                                     QString fileType);

  /**
   * Compare the colormap names of the Slice Viewer and the VSI and extract all
   * indices of the list of Slice Viewer color maps
   * which also exist in the list of the VSI color maps.
   * @param colorMapNamesSliceViewer A list of color maps of the Slice Viewer.
   * @param colorMapNamesVsi A list of color maps for the VSI.
   * @returns A vector of indices for the slice viewer list.
   */
  std::vector<int>
  getSliceViewerIndicesForCommonColorMaps(QStringList colorMapNamesSliceViewer,
                                          QStringList colorMapNamesVsi);
};

#endif
