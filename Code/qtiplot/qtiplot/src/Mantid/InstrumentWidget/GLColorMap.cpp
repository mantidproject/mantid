#include "GLColorMap.h"
#include "GLColor.h"
#include <iostream>
#include <fstream>
#include <sstream>

GLColorMap::GLColorMap()
{
	for(int i=0;i<256;i++){
		color[i]=boost::shared_ptr<GLColor>(new GLColor());
	}
	defaultColormap();
}

void GLColorMap::setColorMapFile(std::string name)
{
	std::string line;
	std::ifstream cmapfile(name.c_str(),std::ios::in);
	if (cmapfile.is_open())
	{
		int count=0;
		double r,g,b;
		while (! cmapfile.eof() )
		{
			std::getline (cmapfile,line);
			std::stringstream sline(line);
			if(count==256||line=="")break;
			sline>>r>>g>>b;
			color[count]->set(r/255.0,g/255.0,b/255.0,1.0);
			count++;
		}
		cmapfile.close();
	}else{ // restore to default colormap
		defaultColormap();
	}

}

boost::shared_ptr<GLColor> GLColorMap::getColor(int id)
{
	return color[id];
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
}