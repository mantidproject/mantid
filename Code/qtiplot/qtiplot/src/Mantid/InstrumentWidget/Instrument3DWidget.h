#ifndef INSTRUMENT3DWIDGET_H_
#define INSTRUMENT3DWIDGET_H_

#include "GL3DWidget.h"
#include "MantidColorMap.h"
#include "boost/shared_ptr.hpp"
#include <vector>
#include "MantidGeometry/V3D.h"

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
  namespace API
  {
    class MatrixWorkspace;
  }
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
  void setTimeBin(int value);
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setDataMappingIntegral(double minValue,double maxValue);
  void setDataMappingSingleBin(int binNumber);
  void setViewDirectionXPositive();
  void setViewDirectionYPositive();
  void setViewDirectionZPositive();
  void setViewDirectionXNegative();
  void setViewDirectionYNegative();
  void setViewDirectionZNegative();

signals:
  void detectorsSelected();
  void actionDetectorHighlighted(int,int,int);

private:
  void ParseInstrumentGeometry(boost::shared_ptr<Mantid::Geometry::IInstrument>);
  void calculateBinRange(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace);
  void calculateColorCounts(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace);
  double integrateSingleSpectra(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, const int wks_index);


  void drawSceneUsingColorID();
  void setSceneLowResolution();
  void setSceneHighResolution();
  void getBoundingBox(Mantid::Geometry::V3D& minBound, Mantid::Geometry::V3D& maxBound);

private:
  /// Convert the list of detector ids to a list of workspace indices and store them
  void createWorkspaceIndexList(const std::vector<int> & idlist);
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

  // The user requested data and bin ranges
  double mDataMinValue, mDataMaxValue;
  double mBinMinValue, mBinMaxValue;

  bool mDataMinEdited, mDataMaxEdited;

  // The workspace data and bin range limits
  double mWkspDataMin, mWkspDataMax;
  double mWkspBinMin, mWkspBinMax;

  QString mWorkspaceName;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> mWorkspace;

  /// Store a value between 0->255 for each of the integrated spectra.
  std::vector<unsigned char> mScaledValues;

  std::vector<int> m_detector_ids;
  std::vector<int> m_workspace_indices;

};

#endif /*GL3DWIDGET_H_*/

