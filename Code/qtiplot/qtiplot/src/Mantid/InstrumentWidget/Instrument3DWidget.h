#ifndef INSTRUMENT3DWIDGET_H_
#define INSTRUMENT3DWIDGET_H_

#include <QGLWidget> 
#include "GL3DWidget.h"
#include "GLColorMapQwt.h"
#include "boost/shared_ptr.hpp"
#include <vector>
/*!
  \class  GL3DWidget
  \brief  OpenGL Qt Widget which renders Instrument
  \author Chapon Laurent & Srikanth Nagella
  \date   August 2008
  \version 1.0

  This Class takes input a Instrument and renders them with in the Qt widget. 

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
namespace Mantid{
	namespace API{
		class Instrument;
	}
}

class Instrument3DWidget : public GL3DWidget
{
	Q_OBJECT
	enum DataMappingType { SINGLE_BIN, INTEGRAL };

public:
	Instrument3DWidget(QWidget* parent=0); ///< Constructor
	virtual ~Instrument3DWidget();         ///< Destructor
	void setWorkspace(std::string name);
	std::string getWorkspaceName();
	void setColorMapName(std::string name);
	GLColorMapQwt getColorMap()const;
	double getDataMinValue();
	double getDataMaxValue();
	void setDataMappingType(DataMappingType);
public slots:
	void fireDetectorsPicked(std::vector<GLActor*> );
	void fireDetectorHighligted(GLActor* pickedActor);
	void setTimeBin(int value);
	void setColorMapMinValue(double minValue);
	void setColorMapMaxValue(double maxValue);
	void setDataMappingIntegral(int minValue,int maxValue);
	void setDataMappingSingleBin(int binNumber);
	void setViewDirectionXPositive();
	void setViewDirectionYPositive();
	void setViewDirectionZPositive();
	void setViewDirectionXNegative();
	void setViewDirectionYNegative();
	void setViewDirectionZNegative();
	void pickedID(int id)
	{
		std::cout<<"ID of the detector selected "<<id<<std::endl;
	}

	void pickedSpectra(int id)
	{
		std::cout<<"Spectra Index selected "<<id<<std::endl;
	}
signals:
	void actionDetectorSelected(int);
	void actionSpectraSelected(int);
	void actionDetectorHighlighted(int,int,int);
	void actionDetectorSelectedList(std::vector<int>);
	void actionSpectraSelectedList(std::vector<int>);
private:
	int iTimeBin;
	DataMappingType mDataMapping;
	GLColorMapQwt mColorMap;
	double DataMinValue;
	double DataMaxValue;
	double BinMinValue;
	double BinMaxValue;
	void AssignColors();
	void ParseInstrumentGeometry(boost::shared_ptr<Mantid::API::Instrument>);
	std::vector<int> getDetectorIDList();
	std::vector<int> getSpectraIndexList(std::vector<int> idDecVec);
	void setColorForDetectors(double minval,double maxval,std::vector<double> values,GLColorMap colMap);
	void CollectTimebinValues(int timebin, std::vector<int> histogramIndexList, double& minval,double& maxval, std::vector<double>& valuesList);
	void CollectIntegralValues(std::vector<int> historgramIndexList, int startbin,int endbin,double& minval,double& maxval, std::vector<double>& valuesList);
	std::string strWorkspaceName;

};

#endif /*GL3DWIDGET_H_*/

