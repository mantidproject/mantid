// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "MantidQtWidgets/InstrumentView/ColorMap.h"
#include "MantidQtWidgets/InstrumentView/DllOption.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MaskBinsData.h"

#include <QObject>
#include <QString>

#include <limits>
#include <memory>
#include <string>
#include <vector>

//------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------
namespace Mantid {
namespace API {
class MatrixWorkspace;
class IMaskWorkspace;
} // namespace API
namespace Geometry {
class Instrument;
class ComponentInfo;
class DetectorInfo;
} // namespace Geometry
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class InstrumentRenderer;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IInstrumentActor : public QObject {
  Q_OBJECT
public:
  virtual void draw(bool picking = false) const = 0;
  virtual void getBoundingBox(Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound,
                              const bool excludeMonitors) const = 0;
  virtual std::shared_ptr<const Mantid::Geometry::Instrument> getInstrument() const = 0;
  virtual std::shared_ptr<const Mantid::API::MatrixWorkspace> getWorkspace() const = 0;
  virtual const Mantid::Geometry::ComponentInfo &componentInfo() const = 0;
  virtual const Mantid::Geometry::DetectorInfo &detectorInfo() const = 0;

  virtual GLColor getColor(size_t index) const = 0;
  virtual double minBinValue() const = 0;
  virtual double maxBinValue() const = 0;

  virtual size_t ndetectors() const = 0;
  virtual Mantid::detid_t getDetID(size_t pickID) const = 0;
  virtual const Mantid::Kernel::V3D getDetPos(size_t pickID) const = 0;
  virtual double getIntegratedCounts(size_t index) const = 0;

  virtual size_t getWorkspaceIndex(size_t index) const = 0;
  virtual void getBinMinMaxIndex(size_t wi, size_t &imin, size_t &imax) const = 0;

  virtual const std::vector<size_t> &components() const = 0;
  virtual const InstrumentRenderer &getInstrumentRenderer() const = 0;
signals:
  void colorMapChanged() const;
  void refreshView() const;
  void initWidget(bool resetGeometry, bool setDefaultView) const;
};

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
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentActor : public IInstrumentActor {
  Q_OBJECT
public:
  /// Invalid workspace index in detector index to workspace index lookup
  static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();
  /// Value that indicates this pixel data is invalid
  static constexpr double INVALID_VALUE = std::numeric_limits<double>::lowest();

  /// Constructor
  InstrumentActor(const std::string &wsName, MantidWidgets::IMessageHandler &messageHandler, bool autoscaling = true,
                  double scaleMin = 0.0, double scaleMax = 0.0, QString settingsGroup = "Mantid/InstrumentWidget");
  InstrumentActor(Mantid::API::MatrixWorkspace_sptr workspace, MantidWidgets::IMessageHandler &messageHandler,
                  bool autoscaling = true, double scaleMin = 0.0, double scaleMax = 0.0,
                  QString settingsGroup = "Mantid/InstrumentWidget");
  ~InstrumentActor();

  /// Draw the instrument in 3D
  void draw(bool picking = false) const override;
  /// Return the bounding box in 3D
  void getBoundingBox(Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound,
                      const bool excludeMonitors) const override;
  /// Set a component (and all its children) visible.
  void setComponentVisible(size_t componentIndex);
  /// Set visibilit of all components.
  void setAllComponentsVisibility(bool /*on*/);
  /// Check if any child is visible
  bool hasChildVisible() const;
  /// Get the underlying instrument
  std::shared_ptr<const Mantid::Geometry::Instrument> getInstrument() const override;
  /// Get the associated data workspace
  std::shared_ptr<const Mantid::API::MatrixWorkspace> getWorkspace() const override;
  const Mantid::Geometry::ComponentInfo &componentInfo() const override;
  const Mantid::Geometry::DetectorInfo &detectorInfo() const override;
  /// Get the mask displayed but not yet applied as a MatrxWorkspace
  std::shared_ptr<Mantid::API::MatrixWorkspace> getMaskMatrixWorkspace() const;
  /// set the mask workspace
  void setMaskMatrixWorkspace(Mantid::API::MatrixWorkspace_sptr wsMask) const;
  /// inverts the internal mask workspace
  void invertMaskWorkspace() const;
  /// Get the mask displayed but not yet applied as a IMaskWorkspace
  std::shared_ptr<Mantid::API::IMaskWorkspace> getMaskWorkspace() const;
  std::shared_ptr<Mantid::API::IMaskWorkspace> getMaskWorkspaceIfExists() const;
  /// Apply the mask in the attached mask workspace to the data.
  void applyMaskWorkspace();
  /// Add a range of bins for masking
  void addMaskBinsData(const std::vector<size_t> &indices);
  /// Remove the attached mask workspace without applying the mask.
  /// Remove the bin masking data.
  void clearMasks();

  bool isInitialized() const { return m_initialized; }

  /// Get the color map.
  const ColorMap &getColorMap() const;
  /// Load a new color map from a file
  void loadColorMap(const std::pair<QString, bool> & /*cmap*/, bool reset_colors = true);
  /// Change the colormap scale type.
  void changeScaleType(int /*type*/);
  /// Change the colormap power scale exponent.
  void changeNthPower(double /*nth_power*/);
  /// Get the file name of the current color map.
  std::pair<QString, bool> getCurrentColorMap() const { return m_currentCMap; }
  /// Toggle colormap scale autoscaling.
  void setAutoscaling(bool /*on*/);
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
  double minBinValue() const override { return m_BinMinValue; }
  /// Get the upper bound of the integration range.
  double maxBinValue() const override { return m_BinMaxValue; }
  /// Get the workspace min bin
  double minWkspBinValue() const { return m_WkspBinMinValue; }
  /// Get the workspace max bin
  double maxWkspBinValue() const { return m_WkspBinMaxValue; }
  /// Return true if the integration range covers the whole of the x-axis in the
  /// data workspace.
  bool wholeRange() const;

  /// Get the number of detectors in the instrument.
  size_t ndetectors() const override;
  /// Get a detector index by a detector ID.
  size_t getDetectorByDetID(Mantid::detid_t detID) const;
  /// Get a detector ID by a pick ID converted form a color in the pick image.
  Mantid::detid_t getDetID(size_t pickID) const override;
  std::vector<Mantid::detid_t> getDetIDs(const std::vector<size_t> &dets) const;
  /// Get a component ID for a non-detector.
  Mantid::Geometry::ComponentID getComponentID(size_t pickID) const;
  /// Get position of a detector by a pick ID converted form a color in the pick
  /// image.
  const Mantid::Kernel::V3D getDetPos(size_t pickID) const override;
  /// Get a vector of IDs of all detectors in the instrument.
  const std::vector<Mantid::detid_t> &getAllDetIDs() const;
  /// Get displayed color of a detector by its index.
  GLColor getColor(size_t index) const override;
  /// Get the workspace index of a detector by its detector Index.
  size_t getWorkspaceIndex(size_t index) const override;
  /// Get the workspace indices of a list of detectors by their detector Index
  std::vector<size_t> getWorkspaceIndices(const std::vector<size_t> &dets) const;
  /// Get the integrated counts of a detector by its detector Index.
  double getIntegratedCounts(size_t index) const override;
  /// Sum the counts in detectors
  void sumDetectors(const std::vector<size_t> &dets, std::vector<double> &x, std::vector<double> &y, size_t size) const;
  /// Sum the counts in detectors.
  void sumDetectors(const std::vector<size_t> &dets, std::vector<double> &x, std::vector<double> &y) const;
  /// Calc indexes for min and max bin values
  void getBinMinMaxIndex(size_t wi, size_t &imin, size_t &imax) const override;

  /// Update the detector colors to match the integrated counts within the
  /// current integration range.
  void updateColors();
  /// Toggle display of the guide and other non-detector instrument components
  void showGuides(bool /*on*/);
  /// Get the guide visibility status
  bool areGuidesShown() const { return m_showGuides; }

  static void BasisRotation(const Mantid::Kernel::V3D &Xfrom, const Mantid::Kernel::V3D &Yfrom,
                            const Mantid::Kernel::V3D &Zfrom, const Mantid::Kernel::V3D &Xto,
                            const Mantid::Kernel::V3D &Yto, const Mantid::Kernel::V3D &Zto, Mantid::Kernel::Quat &R,
                            bool out = false);

  static void rotateToLookAt(const Mantid::Kernel::V3D &eye, const Mantid::Kernel::V3D &up, Mantid::Kernel::Quat &R);

  /* Masking */

  void initMaskHelper() const;
  bool hasMaskWorkspace() const;
  bool hasBinMask() const;
  QString getParameterInfo(size_t index) const;
  std::string getDefaultAxis() const;
  std::string getDefaultView() const;
  std::string getInstrumentName() const;
  std::vector<std::string> getStringParameter(const std::string &name, bool recursive = true) const;
  /// Load the state of the actor from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the actor to a Mantid project file.
  std::string saveToProject() const;
  /// Returns indices of all non-detector components in Instrument.
  const std::vector<size_t> &components() const override { return m_components; }

  bool hasGridBank() const;
  size_t getNumberOfGridLayers() const;
  void setGridLayer(bool isUsingLayer, int layer) const;
  const InstrumentRenderer &getInstrumentRenderer() const override;

  void saveSettings() const;

public slots:
  void initialize(bool resetGeometry, bool setDefaultView);
  void cancel();

private:
  static constexpr double TOLERANCE = 0.00001;

  void setUpWorkspace(const std::shared_ptr<const Mantid::API::MatrixWorkspace> &sharedWorkspace, double scaleMin,
                      double scaleMax);
  void setupPhysicalInstrumentIfExists();
  void loadSettings();
  void resetColors();
  void setDataMinMaxRange(double vmin, double vmax);
  void setDataIntegrationRange(const double &xmin, const double &xmax);
  void calculateIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace);
  /// Sum the counts in detectors if the workspace has equal bins for all
  /// spectra
  void sumDetectorsUniform(const std::vector<size_t> &dets, std::vector<double> &x, std::vector<double> &y) const;
  /// Sum the counts in detectors if the workspace is ragged
  void sumDetectorsRagged(const std::vector<size_t> &dets, std::vector<double> &x, std::vector<double> &y,
                          size_t size) const;
  void resetInstrumentRenderer();

  /// The workspace whose data are shown
  std::shared_ptr<Mantid::API::MatrixWorkspace> m_workspace;
  /// The name of the settings group to store settings in
  QString m_settingsGroup;
  /// The helper masking workspace keeping the mask build in the mask tab but
  /// not applied to the data workspace.
  mutable std::shared_ptr<Mantid::API::MatrixWorkspace> m_maskWorkspace;
  /// A helper object that keeps bin masking data.
  mutable MaskBinsData m_maskBinsData;
  std::pair<QString, bool> m_currentCMap;
  /// integrated spectra
  std::vector<double> m_integratedSignal;
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
  ColorMap::ScaleType m_scaleType;
  /// Position to refer to when detector not found
  const Mantid::Kernel::V3D m_defaultPos;
  /// Flag which stores whether or not a 3D GridBank is present
  bool m_hasGrid;
  /// Stores the number of grid Layers
  size_t m_numGridLayers;

  bool m_initialized;
  double m_scaleMin;
  double m_scaleMax;

  /// Colors in order of component info
  std::vector<size_t> m_monitors;
  std::vector<size_t> m_components;
  std::vector<bool> m_isCompVisible;
  std::vector<size_t> m_detIndex2WsIndex;

  bool m_isPhysicalInstrument;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_physicalComponentInfo;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_physicalDetectorInfo;
  std::unique_ptr<InstrumentRenderer> m_renderer;
  MantidWidgets::IMessageHandler &m_messageHandler;

  mutable Mantid::API::AlgorithmID m_algID;

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
  friend class InstrumentWidgetRenderTab;
};

} // namespace MantidWidgets
} // namespace MantidQt
