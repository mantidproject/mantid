#ifndef OBJCOMPONENT_H_
#define OBJCOMPONENT_H_
#include "Component.h"

namespace Mantid
{
namespace Geometry
{
	class GeomObj;
class ObjComponent : public Component
{
public:
	virtual std::string type() {return "PhysicalComponent";}
	ObjComponent();
	ObjComponent(const std::string&, Component* reference=0);
	virtual ~ObjComponent();
	virtual Component* clone() const {return new ObjComponent(*this);}
	void setObject(GeomObj*);
	const GeomObj* getObject() const;
	int& getLevel() { return level; }
	const int& getLevel() const {return level;}
private:
	int level;
	GeomObj* obj;
};

} // Namespace Geometry
} // Namespace Mantid

#endif /*OBJCOMPONENT_H_*/
