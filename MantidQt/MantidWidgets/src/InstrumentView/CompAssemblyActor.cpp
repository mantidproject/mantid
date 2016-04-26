#include "MantidQtMantidWidgets/InstrumentView/CompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/ObjComponentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/ObjCompAssemblyActor.h"
#include "MantidQtMantidWidgets/InstrumentView/RectangularDetectorActor.h"
#include "MantidQtMantidWidgets/InstrumentView/StructuredDetectorActor.h"
#include "MantidQtMantidWidgets/InstrumentView/OpenGLError.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h"

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidKernel/Exception.h"

#include <cfloat>

using Mantid::Geometry::Instrument;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::ObjCompAssembly;
using Mantid::Geometry::ComponentID;
using Mantid::Geometry::RectangularDetector;
using Mantid::Geometry::StructuredDetector;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::Object;

namespace MantidQt
{
	namespace MantidWidgets
	{

		/**
		* This is a constructor for CompAssembly Actor
		* @param instrActor :: the current instrument actor
		* @param compID :: the current component ID
		*/
		CompAssemblyActor::CompAssemblyActor(const InstrumentActor &instrActor, const Mantid::Geometry::ComponentID &compID)
			: ICompAssemblyActor(instrActor, compID) {
			boost::shared_ptr<const IComponent> CompPtr = getComponent();

			//bounding box of the overall instrument
			Mantid::Kernel::V3D minBound;
			Mantid::Kernel::V3D maxBound;
			//Iterate through CompAssembly children
			boost::shared_ptr<const ICompAssembly> CompAssemPtr = boost::dynamic_pointer_cast<const ICompAssembly>(CompPtr);
			if (CompAssemPtr != boost::shared_ptr<ICompAssembly>())
			{
				int nChild = CompAssemPtr->nelements();
				for (int i = 0; i<nChild; i++)
				{
					boost::shared_ptr<IComponent> ChildCompPtr = (*CompAssemPtr)[i];
					boost::shared_ptr<ICompAssembly> ChildCAPtr = boost::dynamic_pointer_cast<ICompAssembly>(ChildCompPtr);

					//If the child is a CompAssembly then create a CompAssemblyActor for the child
					if (ChildCAPtr)
					{
						boost::shared_ptr<ObjCompAssembly> ChildOCAPtr = boost::dynamic_pointer_cast<ObjCompAssembly>(ChildCompPtr);
						boost::shared_ptr<RectangularDetector> ChildRDPtr = boost::dynamic_pointer_cast<RectangularDetector>(ChildCompPtr);
						boost::shared_ptr<StructuredDetector> ChildSDPtr = boost::dynamic_pointer_cast<StructuredDetector>(ChildCompPtr);

						if (ChildSDPtr)
						{
							StructuredDetectorActor *iActor = new StructuredDetectorActor(instrActor, ChildSDPtr->getComponentID());
							iActor->getBoundingBox(minBound, maxBound);
							AppendBoundingBox(minBound, maxBound);
							mNumberOfDetectors += iActor->getNumberOfDetectors();
							mChildCompAssemActors.push_back(iActor);
						}
						else if (ChildRDPtr)
						{
							//If the child is a RectangularDetector, then create a RectangularDetectorActor for it.
							RectangularDetectorActor* iActor = new RectangularDetectorActor(instrActor, ChildCAPtr->getComponentID());
							iActor->getBoundingBox(minBound, maxBound);
							AppendBoundingBox(minBound, maxBound);
							mNumberOfDetectors += iActor->getNumberOfDetectors();
							mChildCompAssemActors.push_back(iActor);
						}
						else if (ChildOCAPtr)
						{
							ObjCompAssemblyActor* iActor =
								new ObjCompAssemblyActor(instrActor, ChildCAPtr->getComponentID());
							iActor->getBoundingBox(minBound, maxBound);
							AppendBoundingBox(minBound, maxBound);
							mNumberOfDetectors += iActor->getNumberOfDetectors();
							mChildCompAssemActors.push_back(iActor);
						}
						else
						{
							CompAssemblyActor* iActor = new CompAssemblyActor(instrActor, ChildCAPtr->getComponentID());
							iActor->getBoundingBox(minBound, maxBound);
							AppendBoundingBox(minBound, maxBound);
							mNumberOfDetectors += iActor->getNumberOfDetectors();
							mChildCompAssemActors.push_back(iActor);
						}
					}
					else //it has to be a ObjComponent child, create a ObjComponentActor for the child use the same display list attribute
					{
						boost::shared_ptr<Mantid::Geometry::IObjComponent> ChildObjPtr = boost::dynamic_pointer_cast<Mantid::Geometry::IObjComponent>(ChildCompPtr);
						ObjComponentActor* iActor = new ObjComponentActor(instrActor, ChildCompPtr->getComponentID());
						iActor->getBoundingBox(minBound, maxBound);
						AppendBoundingBox(minBound, maxBound);
						mChildObjCompActors.push_back(iActor);
						mNumberOfDetectors++;
					}
				}
			}
		}

		/**
		* Destructor which removes the actors created by this object
		*/
		CompAssemblyActor::~CompAssemblyActor()
		{
			//Remove all the child CompAssembly Actors
			for (std::vector<ICompAssemblyActor*>::iterator iAssem = mChildCompAssemActors.begin(); iAssem != mChildCompAssemActors.end(); ++iAssem)
				delete (*iAssem);
			mChildCompAssemActors.clear();
			//Remove all the child ObjComponent Actors
			for (std::vector<ObjComponentActor*>::iterator iObjComp = mChildObjCompActors.begin(); iObjComp != mChildObjCompActors.end(); ++iObjComp)
				delete (*iObjComp);
			mChildObjCompActors.clear();
		}

		/**
		* This function is concrete implementation that renders the Child ObjComponents and Child CompAssembly's
		*/
		void CompAssemblyActor::draw(bool picking)const
		{
			OpenGLError::check("CompAssemblyActor::draw(0)");
			//Only draw the CompAssembly Children only if they are visible
			if (isVisible())
			{
				//Iterate through the ObjCompActor children and draw them
				for (std::vector<ObjComponentActor*>::iterator itrObjComp = mChildObjCompActors.begin(); itrObjComp != mChildObjCompActors.end(); ++itrObjComp)
				{
					//Only draw the ObjCompActor if its visible
					if ((*itrObjComp)->isVisible())
					{
						//std::cout << (*itrObjComp)->getName() << " is gonna draw. From define()\n";
						(*itrObjComp)->draw(picking);
						OpenGLError::check("draw " + (*itrObjComp)->getComponent()->getName());
					}
					else
					{
						//std::cout << (*itrObjComp)->getName() << " is not visible\n";
					}
				}
				//Iterate through the CompAssemblyActor children and draw them
				for (std::vector<ICompAssemblyActor*>::iterator itrObjAssem = mChildCompAssemActors.begin(); itrObjAssem != mChildCompAssemActors.end(); ++itrObjAssem)
				{
					if ((*itrObjAssem)->isVisible())
					{
						//std::cout << (*itrObjAssem)->getName() << " is gonna draw. From define()\n";
						(*itrObjAssem)->draw(picking);
					}
				}
			}
			else
			{
				//std::cout << this->getName() << " is not visible\n";
			}
			OpenGLError::check("CompAssemblyActor::draw()");
		}

		bool CompAssemblyActor::accept(GLActorVisitor& visitor, VisitorAcceptRule rule)
		{
			for (std::vector<ObjComponentActor*>::iterator itrObjComp = mChildObjCompActors.begin(); itrObjComp != mChildObjCompActors.end(); ++itrObjComp)
			{
				if ((**itrObjComp).accept(visitor, rule) && rule == Finish) return true;
			}
			for (std::vector<ICompAssemblyActor*>::iterator itrObjAssem = mChildCompAssemActors.begin(); itrObjAssem != mChildCompAssemActors.end(); ++itrObjAssem)
			{
				if ((**itrObjAssem).accept(visitor, rule) && rule == Finish) return true;
			}
			return visitor.visit(this);
		}

		bool CompAssemblyActor::accept(GLActorConstVisitor &visitor, GLActor::VisitorAcceptRule rule) const
		{
			for (std::vector<ObjComponentActor*>::iterator itrObjComp = mChildObjCompActors.begin(); itrObjComp != mChildObjCompActors.end(); ++itrObjComp)
			{
				if ((**itrObjComp).accept(visitor, rule) && rule == Finish) return true;
			}
			for (std::vector<ICompAssemblyActor*>::iterator itrObjAssem = mChildCompAssemActors.begin(); itrObjAssem != mChildCompAssemActors.end(); ++itrObjAssem)
			{
				if ((**itrObjAssem).accept(visitor, rule) && rule == Finish) return true;
			}
			return visitor.visit(this);
		}

		//------------------------------------------------------------------------------------------------
		/**
		* Append the bounding box CompAssembly bounding box
		* @param minBound :: min point of the bounding box
		* @param maxBound :: max point of the bounding box
		*/
		void CompAssemblyActor::AppendBoundingBox(const Mantid::Kernel::V3D& minBound, const Mantid::Kernel::V3D& maxBound)
		{
			if (minBoundBox[0]>minBound[0]) minBoundBox[0] = minBound[0];
			if (minBoundBox[1]>minBound[1]) minBoundBox[1] = minBound[1];
			if (minBoundBox[2]>minBound[2]) minBoundBox[2] = minBound[2];
			if (maxBoundBox[0]<maxBound[0]) maxBoundBox[0] = maxBound[0];
			if (maxBoundBox[1]<maxBound[1]) maxBoundBox[1] = maxBound[1];
			if (maxBoundBox[2]<maxBound[2]) maxBoundBox[2] = maxBound[2];
		}

		void CompAssemblyActor::setColors()
		{
			for (std::vector<ICompAssemblyActor*>::iterator iAssem = mChildCompAssemActors.begin(); iAssem != mChildCompAssemActors.end(); ++iAssem)
			{
				(**iAssem).setColors();
			}
			for (std::vector<ObjComponentActor*>::iterator iObjComp = mChildObjCompActors.begin(); iObjComp != mChildObjCompActors.end(); ++iObjComp)
			{
				(**iObjComp).setColors();
			}
		}

		void CompAssemblyActor::setChildVisibility(bool on)
		{
			GLActor::setVisibility(on);
			for (std::vector<ObjComponentActor*>::iterator itrObjComp = mChildObjCompActors.begin(); itrObjComp != mChildObjCompActors.end(); ++itrObjComp)
			{
				(**itrObjComp).setVisibility(on);
			}
			for (std::vector<ICompAssemblyActor*>::iterator itrObjAssem = mChildCompAssemActors.begin(); itrObjAssem != mChildCompAssemActors.end(); ++itrObjAssem)
			{
				(**itrObjAssem).setChildVisibility(on);
			}
		}

		bool CompAssemblyActor::hasChildVisible() const
		{
			for (std::vector<ObjComponentActor*>::iterator itrObjComp = mChildObjCompActors.begin(); itrObjComp != mChildObjCompActors.end(); ++itrObjComp)
			{
				if ((**itrObjComp).isVisible()) return true;
			}
			for (std::vector<ICompAssemblyActor*>::iterator itrObjAssem = mChildCompAssemActors.begin(); itrObjAssem != mChildCompAssemActors.end(); ++itrObjAssem)
			{
				if ((**itrObjAssem).hasChildVisible()) return true;
			}
			return false;
		}

	}//MantidWidgets
}//MantidQt
