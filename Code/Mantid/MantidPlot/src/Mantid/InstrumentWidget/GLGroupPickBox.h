#ifndef GLGROUPPICKBOX_H_
#define GLGROUPPICKBOX_H_

#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <vector>
#include <set>
/**
  \class  GLGroupPickBox
  \brief  class to display and pick group of detectors
  \author Srikanth Nagella
  \date   November 2008
  \version 1.0

  GLGroupPickBox class takes an image and based on mouse operations draws the box and fires the objects that are 
  with in the box.

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
class GLActorCollection;
class GLActor;
class GLGroupPickBox
{
public:
  GLGroupPickBox();  ///< Constructor
  ~GLGroupPickBox();                   ///< Destructor
  void setImages(QImage,QImage);
  void setDisplayImage(QImage);
  void setPickImage(QImage);
  void draw(QPainter*);
  void drawPickBox(QPainter* painter);
  void mousePressed (Qt::MouseButtons buttons, const QPoint & pos );
  void mouseMoveEvent ( QMouseEvent * event );
  void mouseReleased(Qt::MouseButtons buttons, const QPoint & pos);
  QRgb pickPoint(int x, int y);
  std::set<QRgb> getListOfColorsPicked();
  void hide();

  QImage mDisplayImage;  ///< This image is used for rendering in the window.
  QImage mPickImage;     ///< This image is used for picking the objects.
private:
  std::set<QRgb> mColorSet;
  int    mBoxStartPtX; ///< X-dim value of start point in Rectangular Box for the pick selection
  int    mBoxEndPtX; ///< X-dim value of end point in Rectangular Box for the pick selection
  int    mBoxStartPtY; ///< Y-dim value of start point in Rectangular Box for the pick selection
  int    mBoxEndPtY; ///< Y-dim value of end point Rectangular Box for the pick selection
  bool   mPickingActive; ///< Holds whether picking is active or not
};

#endif /*GLGROUPPICKBOX_H_*/

