#ifndef DETECTOR_H_
#define DETECTOR_H_
#include "System.h"
#include "ObjComponent.h"
#include <string>
namespace Mantid
{
namespace Geometry
{
class DLLExport Detector : public ObjComponent
{
public:
	virtual std::string type() const {return "DetectorComponent";}
	Detector();
	Detector(const std::string&, Component* reference=0);
	virtual ~Detector();
	virtual Component* clone() const {return new Detector(*this);}
	void setID(int);
	int getID() const;
private:
	// The detector id
	int id;
};

}
}
#endif /*DETECTOR_H_*/
