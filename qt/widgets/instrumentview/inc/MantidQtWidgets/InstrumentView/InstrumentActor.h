#ifndef INSTRUMENTACTOR_H_
#define INSTRUMENTACTOR_H_

#include "DllOption.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "GLColor.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"
#include "MaskBinsData.h"
#include "MantidGeometry/IComponent.h"
#include <boost/weak_ptr.hpp>
#include <vector>

//------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------
namespace Mantid {
namespace API {
class MatrixWorkspace;
class IMaskWorkspace;
}
namespace Geometry {
class IObjComponent;
class Instrument;
class IDetector;
class ComponentInfo;
class DetectorInfo;
}
}

namespace MantidQt {
namespace MantidWidgets {

/**
\class  InstrumentActor
\brief  InstrumentActor class is wrapper actor for the instrument.
\author Srikanth Nagella
\date   March 2009
\version 1.0

This class has the implementation for rendering Instrument. it provides the
interface for picked ObjComponent and other
operation for selective rendering of the instrument

*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentActor : public QObject {
  Q_OBJECT
public:
  /// Invalid workspace index in detector index to workspace index lookup
  static const size_t INVALID_INDEX;
  /// Constructor
  InstrumentActor(const QString &wsName, bool autoscaling = true,
                  double scaleMin = 0.0, double scaleMax = 0.0);
  ///< Destructor
  ~InstrumentActor();
  /// Draw the instrument in 3D
  void draw(bool picking = false) const;
  /// Return the bounding box in 3D
  void getBoundingBox(Mantid::Kernel::V3D &minBound,
                      Mantid::Kernel::V3D &maxBound) const;
  /// Set a component (and all its children) visible.
  void setComponentVisible(size_t componentIndex);
  /// Toggle the visibility of the child actors (if exist).
  void setChildVisibility(bool);
  /// Check if any child is visible
  bool hasChildVisible() const;
  /// Get the underlying instrument
  std::vector<size_t> getMonitors() const;
  boost::shared_ptr<const Mantid::Geometry::Instrument> getInstrument() const;
  /// Get the associated data workspace
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> getWorkspace() const;
  const Mantid::Geometry::ComponentInfo &componentInfo() const;
  const Mantid::Geometry::DetectorInfo &detectorInfo() const;
  /// Get the mask displayed but not yet applied as a MatrxWorkspace
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  getMaskMatrixWorkspace() const;
  /// set the mask workspace
  void setMaskMatrixWorkspace(Mantid::API::MatrixWorkspace_sptr wsMask) const;
  /// inverts the internal mask workspace
  void invertMaskWorkspace() const;
  /// Get the mask displayed but not yet applied as a IMaskWorkspace
  boost::shared_ptr<Mantid::API::IMaskWorkspace> getMaskWorkspace() const;
  /// Apply the mask in the attached mask workspace to the data.
  void applyMaskWorkspace();
  /// Add a range of bins for masking
  void addMaskBinsData(const std::vector<size_t> &indices);
  /// Remove the attached mask workspace without applying the mask.
  /// Remove the bin masking data.
  void clearMasks();

  /// Get the color map.
  const MantidColorMap &getColorMap() const;
  /// Load a new color map from a file
  void loadColorMap(const QString &, bool reset_colors = true);
  /// Change the colormap scale type.
  void changeScaleType(int);
  /// Change the colormap power scale exponent.
  void changeNthPower(double);
  /// Get the file name of the current color map.
  QString getCurrentColorMap() const { return m_currentColorMapFilename; }
  /// Toggle colormap scale autoscaling.
  void setAutoscaling(bool);
  /// extracts a mask workspace from the visualised workspace
  Mantid::API::MatrixWorkspace_sptr extractCurrentMask() const;

  /// Get colormap scale autoscaling status.
  bool autoscaling() const { return m_autoscaling; }

  /// Set the integration range.
  void setIntegrationRange(const double &xmin, const double &xmax);
  /// Get the minimum data value on the color map scale.
  double minValue() const { return m_DataMinScaleValue; }
  /// Get the maximum data value on the color map scale.
  double maxValue() const { return m_DataMaxScaleValue; }
  /// Set the minimum data value on the color map scale.
  void setMinValue(double value);
  /// Set the maximum data value on the color map scale.
  void setMaxValue(double value);
  /// Set both the minimum and the maximum data values on the color map scale.
  void setMinMaxRange(double vmin, double vmax);
  /// Get the smallest positive data value in the data. Used by the log20 scale.
  double minPositiveValue() const { return m_DataPositiveMinValue; }
  /// Get the lower bound of the integration range.
  double minBinValue() const { return m_BinMinValue; }
  /// Get the upper bound of the integration range.
  double maxBinValue() const { return m_BinMaxValue; }
  /// Return true if the integration range covers the whole of the x-axis in the
  /// data workspace.
  bool wholeRange() const;

  /// Get the number of detectors in the instrument.
  size_t ndetectors() const; // { return m_detIDs.size(); }
  /// Get a detector index by a detector ID.
  size_t getDetectorByDetID(Mantid::detid_t detID) const;
  /// Get a detector ID by a pick ID converted form a color in the pick image.
  Mantid::detid_t getDetID(size_t pickID) const;
  QList<Mantid::detid_t> getDetIDs(const std::vector<size_t> &dets) const;
  /// Get a component ID for a non-detector.
  Mantid::Geometry::ComponentID getComponentID(size_t pickID) const;
  /// Get position of a detector by a pick ID converted form a color in the pick
  /// image.
  const Mantid::Kernel::V3D getDetPos(size_t pickID) const;
  /// Get a vector of IDs of all detectors in the instrument.
  const std::vector<Mantid::detid_t> &getAllDetIDs() const;
  /// Get displayed color of a detector by its index.
  GLColor getColor(size_t index) const;
  /// Get the workspace index of a detector by its detector Index.
  size_t getWorkspaceIndex(size_t index) const;
  /// Get the integrated counts of a detector by its detector Index.
  double getIntegratedCounts(size_t index) const;
  /// Sum the counts in detectors
  void sumDetectors(const std::vector<size_t> &dets, std::vector<double> &x,
                    std::vector<double> &y, size_t size = 0) const;
  /// Calc indexes for min and max bin values
  void getBinMinMaxIndex(size_t wi, size_t &imin, size_t &imax) const;

  /// Update the detector colors to match the integrated counts within the
  /// current integration range.
  void updateColors();
  /// Invalidate the OpenGL display lists to force full re-drawing of the
  /// instrument and creation of new lists.
  void invalidateDisplayLists() const; //{ m_scene.invalidateDisplayList(); }
  /// Toggle display of the guide and other non-detector instrument components
  void showGuides(bool);
  /// Get the guide visibility status
  bool areGuidesShown() const { return m_showGuides; }

  static void BasisRotation(const Mantid::Kernel::V3D &Xfrom,
                            const Mantid::Kernel::V3D &Yfrom,
                            const Mantid::Kernel::V3D &Zfrom,
                            const Mantid::Kernel::V3D &Xto,
                            const Mantid::Kernel::V3D &Yto,
                            const Mantid::Kernel::V3D &Zto,
                            Mantid::Kernel::Quat &R, bool out = false);

  static void rotateToLookAt(const Mantid::Kernel::V3D &eye,
                             const Mantid::Kernel::V3D &up,
                             Mantid::Kernel::Quat &R);

  /// Convert a "pick ID" to a colour to put into the pick image.
  static GLColor makePickColor(size_t pickID);
  /// Decode a pick colour and return corresponding "pick ID"
  static size_t decodePickColor(const QRgb &c);
  /* Masking */

  void initMaskHelper() const;
  bool hasMaskWorkspace() const;
  bool hasBinMask() const;
  QString getParameterInfo(size_t index) const;
  std::string getDefaultAxis() const;
  std::string getDefaultView() const;
  std::string getInstrumentName() const;
  std::vector<std::string> getStringParameter(const std::string &name,
                                              bool recursive = true) const;
  /// Load the state of the actor from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the actor to a Mantid project file.
  std::string saveToProject() const;

signals:
  void colorMapChanged();

private:
  void doDraw(bool picking) const;
  void setUpWorkspace(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> sharedWorkspace,
      double scaleMin, double scaleMax);
  void setupPhysicalInstrumentIfExists();
  void resetColors();
  void loadSettings();
  void saveSettings();
  void setDataMinMaxRange(double vmin, double vmax);
  void setDataIntegrationRange(const double &xmin, const double &xmax);
  void
  calculateIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace);
  /// Sum the counts in detectors if the workspace has equal bins for all
  /// spectra
  void sumDetectorsUniform(const std::vector<size_t> &dets,
                           std::vector<double> &x,
                           std::vector<double> &y) const;
  /// Sum the counts in detectors if the workspace is ragged
  void sumDetectorsRagged(const std::vector<size_t> &dets,
                          std::vector<double> &x, std::vector<double> &y,
                          size_t size) const;

  void setupPickColors();

  boost::shared_ptr<Mantid::API::IMaskWorkspace>
  getMaskWorkspaceIfExists() const;

  /// The workspace whose data are shown
  const boost::weak_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
  /// The helper masking workspace keeping the mask build in the mask tab but
  /// not applied to the data workspace.
  mutable boost::shared_ptr<Mantid::API::MatrixWorkspace> m_maskWorkspace;
  /// A helper object that keeps bin masking data.
  mutable MaskBinsData m_maskBinsData;
  /// The colormap
  MantidColorMap m_colorMap;
  QString m_currentColorMapFilename;
  /// integrated spectra
  std::vector<double> m_specIntegrs;
  /// The workspace data and bin range limits
  double m_WkspBinMinValue, m_WkspBinMaxValue;
  // The user requested data and bin ranges
  /// y-values min and max for current bin (x integration) range
  double m_DataMinValue, m_DataMaxValue, m_DataPositiveMinValue;
  /// min and max of the color map scale
  double m_DataMinScaleValue, m_DataMaxScaleValue;
  /// x integration range
  double m_BinMinValue, m_BinMaxValue;
  /// Hint on whether the workspace is ragged or not
  bool m_ragged;
  /// Flag to rescale the colormap axis automatically when the data or
  /// integration range change
  bool m_autoscaling;
  /// Flag to show the guide and other components. Loaded and saved in settings.
  bool m_showGuides;
  /// Color map scale type: linear or log
  GraphOptions::ScaleType m_scaleType;
  /// Position to refer to when detector not found
  const Mantid::Kernel::V3D m_defaultPos;

  /// Colors in order of component info
  std::vector<GLColor> m_colors;
  std::vector<size_t> m_monitors;
  std::vector<size_t> m_components;
  /// Colour of a masked detector
  GLColor m_maskedColor;
  /// Colour of a "failed" detector
  GLColor m_failedColor;

  static double m_tolerance;

  std::vector<GLColor> m_pickColors;
  std::vector<bool> m_isCompVisible;
  std::vector<size_t> m_detIndex2WsIndex;
  // Two display lists for normal rendering and picking
  mutable GLuint m_displayListId[2];
  mutable bool m_useDisplayList[2];
  bool m_isPhysicalInstrument;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_physicalComponentInfo;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_physicalDetectorInfo;
};

} // MantidWidgets
} // MantidQt

#endif /*InstrumentActor_H_*/
