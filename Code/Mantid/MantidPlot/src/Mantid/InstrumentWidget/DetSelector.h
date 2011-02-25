#ifndef DETSELECTOR_H_
#define DETSELECTOR_H_

#include <QColor>
/**
  \class  DetSelector
  \brief  class to pick group of detectors
  \author Roman Tolchenov
  \date   10 Feb 2011

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

class QPainter;

enum DetSelectionType {Single,BoxType,Tube};

class DetSelector
{
public:
  DetSelector();            ///< Constructor
  virtual ~DetSelector(){}  ///< Destructor
  virtual void draw(QPainter& painter);
  virtual void start(int x, int y);
  virtual void move(int , int ){}
  virtual void stop();
  static DetSelector* create(DetSelectionType type);
protected:
  int m_xStart;
  int m_yStart;
  bool m_inProgress;
  QColor m_color;
};

class BoxDetSelector: public DetSelector
{
public:
  void draw(QPainter& painter);
  void move(int x, int y);
protected:
  int m_xEnd;
  int m_yEnd;
};

#endif /*DETSELECTOR_H_*/

