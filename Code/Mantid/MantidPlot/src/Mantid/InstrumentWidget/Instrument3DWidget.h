#ifndef INSTRUMENT3DWIDGET_H_
#define INSTRUMENT3DWIDGET_H_

#include "GL3DWidget.h"
#include "MantidColorMap.h"
#include "boost/shared_ptr.hpp"
#include <vector>
#include "MantidGeometry/V3D.h"
#include "MantidAPI/MatrixWorkspace.h"

class QAction;

/*!
  \class  GL3DWidget
  \brief  OpenGL Qt Widget which renders Instrument
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This Class takes input a Instrument and renders them with in the Qt widget.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
namespace Mantid
{
  namespace Geometry
  {
    class IInstrument;
  }
}

class InstrumentActor;

class Instrument3DWidget : public GL3DWidget
{ 
  Q_OBJECT
  enum DataMappingType { SINGLE_BIN, INTEGRAL };
  enum Handedness { LEFT, RIGHT };

public:

  /// Contains summary information about detectors for display to users
  class DetInfo : public std::unary_function<int, int>
  {
  public:
    /** Returns the index number of the spectrum for the given detector
    *  @param someDetID id of detector to be queried
    */
    int operator()(const int someDetID) const{return getIndexOf(someDetID);}
    /// Get pointers to the workspace that contain information about detectors
    DetInfo(Mantid::API::MatrixWorkspace_const_sptr workspace=Mantid::API::MatrixWorkspace_const_sptr(), const std::vector<double> * const counts=NULL);

    void setDet(const int detID);                                 ///< set up the object to contain data for only one detector
    void setEndRange(const int detID);                            ///< for a detector with at least one detector set the detector for the end of the range
    QString display() const;                                      ///< Creates a string with the object's data in a human readable form
  private:
    static const int NO_INDEX = -1;                               ///< Value used to indicate missing data, detectors, spectra, etc...
    static const int ERROR_FLAG = -2;                             ///< flags missing values but indicates an inconsistency in the data state
    Mantid::API::MatrixWorkspace_const_sptr m_workspace;          ///< All data refer to this workspace
    const std::vector<double> * m_integrals;                      ///< the integral of each spectra
    boost::shared_ptr<const Mantid::API::IndexToIndexMap> m_detID_to_wi_map;///< used to get workspace indices from detector IDs
    int m_firstDet;                                               ///< id number of the detector that was selected first
    int m_lastDet;                                                ///< if more than one detector is selected this is the id number of the one selected last otherwise the NO_DATA value

    int getIndexOf(const int someDetID) const;                    ///< Gets the index number of the spectrum for the given detector
    void printSpectrum(const int index, std::ostringstream & output) const;///< Retrieves information about the spectrum whose index was passed
    void printLocation(std::ostringstream & output) const;///< Writes the location of the detectors
    void printV(Mantid::Geometry::V3D pos, std::ostringstream & out) const;///< Writes a position vector in a nice way
    /** Returns true only if the value passed can't be missing data
    *  @param testVal the data value to check
    *  @return if there are no error flags returns true
    */
    bool is_good(const int testVal) const { return testVal > NO_INDEX; }
  };

  Instrument3DWidget(QWidget* parent=0); ///< Constructor
  virtual ~Instrument3DWidget();         ///< Destructor
  void setWorkspace(const QString& name);
  QString getWorkspaceName() const;
  const MantidColorMap & getColorMap() const;
  MantidColorMap & mutableColorMap();
  double getDataMinValue() const;
  double getDataMaxValue() const;
  double getBinMinValue() const;
  double getBinMaxValue() const;
  bool getBinEntireRange() const;
  void setDataMappingType(DataMappingType);
  void setView(const Mantid::Geometry::V3D&,double,double,double,double,double,double);
  void setAxis(const Mantid::Geometry::V3D &direction);
  void resetWidget();
  void setFastRendering();
  void setSlowRendering();
  void updateColorsForNewMap();
  void setMinData(const double);
  void setMaxData(const double);
  void setDataMinEdited(bool state);
  void setDataMaxEdited(bool state);
  bool dataMinValueEdited() const { return mDataMinEdited; }
  bool dataMaxValueEdited() const { return mDataMaxEdited; }
  
  void recount();

  const std::vector<int> & getSelectedDetectorIDs() const { return m_detector_ids; }
  const std::vector<int> & getSelectedWorkspaceIndices() const { return m_workspace_indices; }

public slots:
  void fireDetectorsPicked(const std::set<QRgb>& );
  void fireDetectorHighligted(QRgb);
  void detectorsHighligted(QRgb);
  void setTimeBin(int value);
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setDataMappingIntegral(double minValue,double maxValue, bool entireRange);
  void setDataMappingSingleBin(int binNumber);
  void setViewDirectionXPositive();
  void setViewDirectionYPositive();
  void setViewDirectionZPositive();
  void setViewDirectionXNegative();
  void setViewDirectionYNegative();
  void setViewDirectionZNegative();
  void showInfo();
  void extractDetsToWorkspace();
  void sumDetsToWorkspace();
  void createIncludeGroupingFile();
  void createExcludeGroupingFile();


signals:
  void detectorsSelected();
  void actionDetectorHighlighted(const Instrument3DWidget::DetInfo &);

public:
  void calculateBinRange();

private:
  void ParseInstrumentGeometry(boost::shared_ptr<Mantid::Geometry::IInstrument>);
  void calculateColorCounts(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, bool firstCalculation);

  void drawSceneUsingColorID();
  void setSceneLowResolution();
  void setSceneHighResolution();
  void getBoundingBox(Mantid::Geometry::V3D& minBound, Mantid::Geometry::V3D& maxBound);
  void showUnwrappedContextMenu();

  /// Convert the list of detector ids to a list of workspace indices and store them
  void createWorkspaceIndexList(const std::vector<int> & idlist, bool forceNew);
  boost::shared_ptr<Mantid::API::MatrixWorkspace> getMatrixWorkspace(QString ) const;

  bool mFastRendering;
  int iTimeBin;
  DataMappingType mDataMapping;
  MantidColorMap mColorMap;
  
  /// List of detector IDs used by the color mapping algorithms.
  std::vector<int> detector_list;

  InstrumentActor* mInstrumentActor;
  Mantid::Geometry::V3D mAxisDirection;
  Mantid::Geometry::V3D mAxisUpVector;

  /// The user requested data and bin ranges
  double mDataMinValue, mDataMaxValue;
  double mBinMinValue, mBinMaxValue;

  /// Bool set to true if you keep all the bins, no matter what. Defaults to true.
  bool mBinEntireRange;

  /// If true the data min or maxvalue has been set by the user
  bool mDataMinEdited, mDataMaxEdited;

  /// The workspace data and bin range limits
  double mWkspDataMin, mWkspDataMax;
  double mWkspBinMin, mWkspBinMax;

  QString mWorkspaceName;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> mWorkspace;

  /// Store a value between 0->255 for each of the integrated spectra.
  std::vector<unsigned char> mScaledValues;

  std::vector<int> m_detector_ids;
  std::vector<int> m_workspace_indices;
  /// integrated number of counts in each spectrum
  std::vector<double> m_specIntegrs;

  /// Contains information about selected detectors, when there is a selection
  DetInfo m_detInfo;

  QAction *m_ExtractDetsToWorkspaceAction;  ///< Extract selected detector ids to a new workspace
  QAction *m_SumDetsToWorkspaceAction;      ///< Sum selected detectors to a new workspace
  QAction *m_createIncludeGroupingFileAction; ///< Create grouping xml file which includes selected detectors
  QAction *m_createExcludeGroupingFileAction; ///< Create grouping xml file which excludes selected detectors
};

#endif /*GL3DWIDGET_H_*/

