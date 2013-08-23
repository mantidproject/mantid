#ifndef INSTRUMENTACTOR_H_
#define INSTRUMENTACTOR_H_

#include "GLColor.h"
#include "GLActor.h"
#include "GLActorCollection.h"
#include "GLActorVisitor.h"
#include "SampleActor.h"
#include "MantidQtAPI/MantidColorMap.h"
#include "MantidAPI/SpectraDetectorTypes.h"

#include <boost/weak_ptr.hpp>
#include <QObject>
#include <vector>
#include <map>

//------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------
namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
    class IMaskWorkspace;
  }
  namespace Geometry
  {
    class IObjComponent;
    class Instrument;
    class IDetector;
  }
}

/**
  \class  InstrumentActor
  \brief  InstrumentActor class is wrapper actor for the instrument.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

   This class has the implementation for rendering Instrument. it provides the interface for picked ObjComponent and other
   operation for selective rendering of the instrument

*/
class InstrumentActor: public QObject, public GLActor
{
  Q_OBJECT
public:
  /// Constructor
  InstrumentActor(const QString &wsName, bool autoscaling = true, double scaleMin = 0.0, double scaleMax = 0.0);
  ///< Destructor
  ~InstrumentActor();    
  ///< Type of the GL object
  virtual std::string type()const {return "InstrumentActor";} 
  /// Draw the instrument in 3D
  void draw(bool picking = false)const;
  /// Return the bounding box in 3D
  void getBoundingBox(Mantid::Kernel::V3D& minBound,Mantid::Kernel::V3D& maxBound)const{m_scene.getBoundingBox(minBound,maxBound);}
  /// Run visitors callback on each component
  bool accept(GLActorVisitor& visitor);
  /// Get the underlying instrument
  boost::shared_ptr<const Mantid::Geometry::Instrument> getInstrument() const;
  /// Get the associated data workspace
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> getWorkspace() const;
  /// Get the mask displayed but not yet applied as a MatrxWorkspace
  boost::shared_ptr<Mantid::API::MatrixWorkspace> getMaskMatrixWorkspace() const;
  /// Get the mask displayed but not yet applied as a IMaskWorkspace
  boost::shared_ptr<Mantid::API::IMaskWorkspace> getMaskWorkspace() const;
  /// Apply the mask in the attached mask workspace to the data.
  void applyMaskWorkspace();
  /// Remove the attached mask workspace without applying the mask.
  void clearMaskWorkspace();

  /// Get the color map.
  const MantidColorMap & getColorMap() const;
  /// Load a new color map from a file
  void loadColorMap(const QString& ,bool reset_colors = true);
  /// Change the colormap scale type.
  void changeScaleType(int);
  /// Get the file name of the current color map.
  QString getCurrentColorMap()const{return m_currentColorMap;}
  /// Toggle colormap scale autoscaling.
  void setAutoscaling(bool);
  /// Get colormap scale autoscaling status.
  bool autoscaling()const{return m_autoscaling;}

  /// Set the integration range.
  void setIntegrationRange(const double& xmin,const double& xmax);
  /// Get the minimum data value on the color map scale.
  double minValue()const{return m_DataMinScaleValue;}
  /// Get the maximum data value on the color map scale.
  double maxValue()const{return m_DataMaxScaleValue;}
  /// Set the minimum data value on the color map scale.
  void setMinValue(double value);
  /// Set the maximum data value on the color map scale.
  void setMaxValue(double value);
  /// Set both the minimum and the maximum data values on the color map scale.
  void setMinMaxRange(double vmin, double vmax);
  /// Get the smallest positive data value in the data. Used by the log20 scale.
  double minPositiveValue()const{return m_WkspDataPositiveMin;}
  /// Get the lower bound of the integration range.
  double minBinValue()const{return m_BinMinValue;}
  /// Get the upper bound of the integration range.
  double maxBinValue()const{return m_BinMaxValue;}
  /// Return true if the integration range covers the whole of the x-axis in the data workspace.
  bool wholeRange()const;

  /// Get the number of detectors in the instrument.
  size_t ndetectors()const{return m_detIDs.size();}
  /// Get shared pointer to a detector by a pick ID converted form a color in the pick image.
  boost::shared_ptr<const Mantid::Geometry::IDetector> getDetector(size_t pickID)const;
  /// Get a detector ID by a pick ID converted form a color in the pick image.
  Mantid::detid_t getDetID(size_t pickID)const{return m_detIDs.at(pickID);}
  /// Cache detector positions.
  void cacheDetPos() const;
  /// Get position of a detector by a pick ID converted form a color in the pick image.
  const Mantid::Kernel::V3D & getDetPos(size_t pickID)const;
  /// Get a vector of IDs of all detectors in the instrument.
  const std::vector<Mantid::detid_t>& getAllDetIDs()const{return m_detIDs;}
  /// Get displayed color of a detector by its detector ID.
  GLColor getColor(Mantid::detid_t id)const;
  /// Get the workspace index of a detector by its detector ID.
  size_t getWorkspaceIndex(Mantid::detid_t id) const;
  /// Get the integrated counts of a detector by its detector ID.
  double getIntegratedCounts(Mantid::detid_t id)const;
  /// Sum the counts in detectors
  void sumDetectors(QList<int>& dets, std::vector<double>&x, std::vector<double>&y, std::vector<double>* err = NULL) const;
  /// Calc indexes for min and max bin values
  void getBinMinMaxIndex(size_t wi,size_t& imin, size_t& imax) const;

  /// Update the detector colors to match the integrated counts within the current integration range.
  void update();
  /// Invalidate the OpenGL display lists to force full re-drawing of the instrument and creation of new lists.
  void invalidateDisplayLists()const{m_scene.invalidateDisplayList();}
  /// Toggle display of the guide and other non-detector instrument components
  void showGuides(bool);
  /// Get the guide visibility status
  bool areGuidesShown() const {return m_showGuides;}

  static void BasisRotation(const Mantid::Kernel::V3D& Xfrom,
                  const Mantid::Kernel::V3D& Yfrom,
                  const Mantid::Kernel::V3D& Zfrom,
                  const Mantid::Kernel::V3D& Xto,
                  const Mantid::Kernel::V3D& Yto,
                  const Mantid::Kernel::V3D& Zto,
                  Mantid::Kernel::Quat& R,
                  bool out = false
                  );

  static void rotateToLookAt(const Mantid::Kernel::V3D& eye, const Mantid::Kernel::V3D& up, Mantid::Kernel::Quat &R);

  /* Masking */

  void initMaskHelper() const;
  bool hasMaskWorkspace() const;
signals:
  void colorMapChanged();
protected:

  void resetColors();
  void loadSettings();
  void saveSettings();

  size_t push_back_detid(Mantid::detid_t)const;
  boost::shared_ptr<Mantid::API::IMaskWorkspace> getMaskWorkspaceIfExists() const;

  /// The workspace whose data are shown
  const boost::weak_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
  /// The helper masking workspace keeping the mask build in the mask tab but not applied to the data workspace.
  mutable boost::shared_ptr<Mantid::API::MatrixWorkspace> m_maskWorkspace;
  /// The colormap
  MantidColorMap m_colorMap;
  /// integrated spectra
  std::vector<double> m_specIntegrs;
  /// The workspace data and bin range limits
  double m_WkspDataMin, m_WkspDataMax,m_WkspDataPositiveMin; ///< min and max over whole workspace
  double m_WkspBinMin, m_WkspBinMax;                         ///< min and max over whole workspace
  /// The user requested data and bin ranges
  double m_DataMinValue, m_DataMaxValue;                     ///< min and max for current bin (x integration) range
  double m_DataMinScaleValue, m_DataMaxScaleValue;           /// min and max of the color map scale
  double m_BinMinValue, m_BinMaxValue;
  /// Flag to rescale the colormap axis automatically when the data or integration range change
  bool m_autoscaling;
  /// Flag to show the guide and other components. Loaded and saved in settings.
  bool m_showGuides;

  /// Pointer to the workspace's detector ID to workspace index map
  Mantid::detid2index_map *m_detid2index_map;

  /// All det ids in the instrument in order of pickIDs, populated by Obj..Actor constructors
  mutable std::vector<Mantid::detid_t> m_detIDs;

  /// All detector positions, in order of pickIDs, populated by Obj..Actor constructors
  mutable std::vector<Mantid::Kernel::V3D> m_detPos;

  /// Colors in order of workspace indexes
  mutable std::vector<GLColor> m_colors;
  QString m_currentColorMap;
  /// Colour of a masked detector
  GLColor m_maskedColor;
  /// Colour of a "failed" detector
  GLColor m_failedColor;
  /// The collection of actors for the instrument components
  GLActorCollection m_scene;
  /// A pointer to the sample actor
  SampleActor* m_sampleActor;

  static double m_tolerance;

  friend class ObjComponentActor;
  friend class ObjCompAssemblyActor;
  friend class RectangularDetectorActor;
};

/**
 * Sets visibility of an actor with a particular ComponentID
 * and makes all other components invisible.
 */
class SetVisibleComponentVisitor: public SetVisibilityVisitor
{
public:
  SetVisibleComponentVisitor(const Mantid::Geometry::ComponentID id):m_id(id){}
  bool visit(GLActor*);
  Mantid::Geometry::ComponentID getID()const{return m_id;}
private:
  Mantid::Geometry::ComponentID m_id;
};

/**
 * Set visibility of all actors of non-detector components.
 * Pass true to constructor to set them visible and false to make them invisible.
 */
class SetVisibleNonDetectorVisitor: public SetVisibilityVisitor
{
public:
  /// Constructor
  /// @param on :: If true then all non-detectors will be made visible or invisible if false.
  SetVisibleNonDetectorVisitor(bool on):m_on(on){}
  bool visit(GLActor*);
private:
  bool m_on;
};

/**
 * Finds an actor with a particular ComponentID
 */
class FindComponentVisitor: public GLActorVisitor
{
public:
  FindComponentVisitor(const Mantid::Geometry::ComponentID id):m_id(id),m_actor(NULL){}
  bool visit(GLActor*);
  ComponentActor* getActor()const{return m_actor;}
private:
  Mantid::Geometry::ComponentID m_id;
  mutable ComponentActor* m_actor;
};

#endif /*InstrumentActor_H_*/

