#include "GLColorMap.h"
#include "GLColor.h"
#include <iostream>
#include <fstream>
#include <sstream>

GLColorMap::GLColorMap()
{
	defaultColormap();
}

void GLColorMap::setColorMapFile(std::string name)
{
	std::string line;
	std::ifstream cmapfile(name.c_str(),std::ios::in);
	if (cmapfile.is_open())
	{
		int count=0;
		while (! cmapfile.eof() )
		{
			std::getline (cmapfile,line);
			std::stringstream sline(line);
			sline>>color[count][0]>>color[count][1]>>color[count][2];
			count++;
		}
		cmapfile.close();
	}else{ // restore to default colormap
		defaultColormap();
	}

}

boost::shared_ptr<GLColor> GLColorMap::getColor(int id)
{
	boost::shared_ptr<GLColor> col(new GLColor(color[id][0]/(float)256.0,color[id][1]/(float)256.0,color[id][2]/(float)256.0,1.0));
	return col;
}

void GLColorMap::defaultColormap()
{
	//set default hsv colormap
	int i;
	//First y coordinate
	for(i=0;i<43;i++){
		color[i][0]=256;
		color[i][1]=6*i;
		color[i][2]=0;
	}
	//Second decrment x
	for(i=0;i<43;i++){
		color[43+i][0]=254-6*i;
		color[43+i][1]=256;
		color[43+i][2]=0;
	}
	//third increment z
	for(i=0;i<43;i++){
		color[86+i][0]=0;
		color[86+i][1]=256;
		color[86+i][2]=4+i*6;
	}
	//fourth decrement y
	for(i=0;i<43;i++){
		color[129+i][0]=0;
		color[129+i][1]=250-6*i;
		color[129+i][2]=256;
	}
	//fifth increment x
	for(i=0;i<43;i++){
		color[172+i][0]=2+6*i;
		color[172+i][1]=0;
		color[172+i][2]=256;
	}
	//sixth decrement z
	for(i=0;i<41;i++){
		color[215+i][0]=256;
		color[215+i][1]=0;
		color[215+i][2]=252-6*i;
	}
}