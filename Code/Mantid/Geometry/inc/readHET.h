#ifndef READHET_
#define READHET_
#include <fstream>
#include "CompAssembly.h"
#include "Detector.h"
using namespace Mantid::Geometry;

// This is reading the HET definition fron a file 
// containing the detector ID number, followed by
// the position of the detectors in spherical coordinates
// I choose  a convention based on the Busing-Levy convention
// (crystallography). The y-axis is along the beam and z up
// The coordinate system is right-handed.
struct readHET
{
	readHET(){}
	~readHET(){}
	void createFromFile(const char* filename)
	{
		std::fstream in;
		in.open(filename,std::ios::in);
		double R, theta, phi;
		if (in.is_open())
		{
			instrument.setName("HET");
			source.setName("Source");
			samplepos.setName("samplepos");
			samplepos.setPos(V3D(0.0,10.0,0.0));
			instrument.add(&source);
			instrument.add(&samplepos);
			tube.setName("PSD");
			bank.setName("bank");
			bank.setParent(&samplepos);
			int i=0;
			int det_id=400;
			V3D pos;
			double c1,c2,c3;
			do
			{
			det_id++;
			in >>  R >> theta >> phi >> c1 >> c2 >> c3;
			pos.spherical(R,theta,phi);
			bank.addCopy(&tube);
			bank[i]->setPos(pos);
			Detector* temp=dynamic_cast<Detector*>(bank[i]);
			if (temp) temp->setID(det_id);
			i++;
			}while(!in.eof()); 
		}
		else
		{
			std::cout << "Error opening file" << filename << std::endl;
		}
	}
	CompAssembly instrument;
	Component source;
	Component samplepos;
	Detector tube;
	CompAssembly bank;
};
#endif /*READHET_*/
