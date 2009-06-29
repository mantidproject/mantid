#include "GLColorMap.h"
#include "GLColor.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Define the static constant
short GLColorMap::mMaxPossibleColors = 256;

GLColorMap::GLColorMap() : mNumberOfColors(0)
{
  for(int i = 0; i < mMaxPossibleColors; i++)
  {
    color[i] = boost::shared_ptr<GLColor>(new GLColor());
  }
  defaultColormap();
}

void GLColorMap::setColorMapFile(const std::string& name)
{
	std::ifstream cmapfile(name.c_str(),std::ios::in);
	if (cmapfile.is_open())
	{
		int count = 0;
		double red(0.0), green(0.0), blue(0.0);
		std::string line;
		while ( std::getline(cmapfile, line) )
		{
		  if(count == mMaxPossibleColors || line.empty() ) break;
		  std::stringstream sline(line);
		  sline >> red >> green >> blue;
		  color[count]->set((float)red/255.0,(float)green/255.0,(float)blue/255.0,1.0);
		  ++count;
		}
		cmapfile.close();
		mNumberOfColors = count;
	}else{ // restore to default colormap
		defaultColormap();
	}

}

boost::shared_ptr<GLColor> GLColorMap::getColor(short id) const
{
	if(mNumberOfColors == 0)
	{
	  return boost::shared_ptr<GLColor>(new GLColor(1.0,0.0,0.0));
	}
	if(id >= 0 && id < mNumberOfColors)
	{
	  return color[id];
	}
	return boost::shared_ptr<GLColor>(new GLColor(0.0,0.0,0.0));
}

void GLColorMap::defaultColormap()
{
	//set default hsv colormap
	int i;
	//First y coordinate
	for(i=0;i<43;i++){
		color[i]->set(1,6*i/256.0,0,1.0);
	}
	//Second decrment x
	for(i=0;i<43;i++){
		color[43+i]->set((254-6*i)/256.0,1,0,1.0);
	}
	//third increment z
	for(i=0;i<43;i++){
		color[86+i]->set(0.0,1.0,(4+i*6)/256.0,1.0);
	}
	//fourth decrement y
	for(i=0;i<43;i++){
		color[129+i]->set(0.0,(250-6*i)/256.0,1.0,1.0);
	}
	//fifth increment x
	for(i=0;i<43;i++){
		color[172+i]->set((2+6*i)/256.0,0.0,1.0,1.0);
	}
	//sixth decrement z
	for(i=0;i<41;i++){
		color[215+i]->set(1.0,0.0,(252-6*i)/256.0,1.0);
	}
	mNumberOfColors=256;
}

short GLColorMap::getNumberOfColors() const
{
  return mNumberOfColors;
}

short GLColorMap::getMaxNumberOfColors() const
{
  return mMaxPossibleColors;
}
