#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/Exception.h"
#include "Poco/DOM/Element.h"
#include <iostream>


using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Document;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using Poco::XML::Node;
using Poco::XML::AutoPtr;
using Poco::Exception;
using Poco::XML::Element;

#include "MantidKernel/Exception.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"


namespace Mantid
{
	namespace Geometry
	{
		Kernel::Logger& vtkGeometryCacheReader::PLog(Kernel::Logger::get("Object"));

		/**
		* Constructor
		*/
		vtkGeometryCacheReader::vtkGeometryCacheReader(std::string filename)
		{
			mFileName=filename;
			mDoc=NULL;
			Init();
		}

		/**
		* Destructor
		*/
		vtkGeometryCacheReader::~vtkGeometryCacheReader()
		{
		  mDoc->release();
			delete pParser;
		}

		/**
		 * Initialise Reading of the cached file
		 */
		void vtkGeometryCacheReader::Init()
		{
			// Set up the DOM parser and parse xml file
			pParser=new DOMParser();
			try
			{
				mDoc = pParser->parse(mFileName);
			}
			catch(...)
			{
				PLog.error("Unable to parse file " + mFileName);
				throw Kernel::Exception::FileError("Unable to parse File:" , mFileName);
			}
		}

		/**
		 * Set the geometry for the object
		 */
		void vtkGeometryCacheReader::readCacheForObject(Object *obj)
		{
			//Get the element corresponding to the name of the object
			std::stringstream objName;
			objName<<obj->getName();
			Poco::XML::Element *pEle=getElementByObjectName(objName.str());
			if(pEle==NULL) //Element not found
			{
				PLog.debug("Cache not found for Object with name " + objName.str());
				return;
			}
			// Read the cache from the element
			int noOfTriangles=0
				,noOfPoints=0;
			double *Points;
			int    *Faces;
			std::stringstream buff;
			// Read number of points
			buff<<pEle->getAttribute("NumberOfPoints");
			buff>>noOfPoints;
			buff.clear();
			//Read number of triangles
			buff<<pEle->getAttribute("NumberOfPolys");
			buff>>noOfTriangles;
			buff.clear();

			// Read Points
			Element* pPts = pEle->getChildElement("Points")->getChildElement("DataArray");
			readPoints(pPts,&noOfPoints,&Points);

			// Read Triangles
			Element* pTris = pEle->getChildElement("Polys")->getChildElement("DataArray");
			readTriangles(pTris,&noOfTriangles,&Faces);
			
			//First check whether Object can be written to the file
			boost::shared_ptr<GeometryHandler> handle=obj->getGeometryHandler();
			handle->setGeometryCache(noOfPoints,noOfTriangles,Points,Faces);	
		}

		/**
		 * Get the Element by using the object name
		 */
		Poco::XML::Element* vtkGeometryCacheReader::getElementByObjectName(std::string name)
		{			
			Element* pRoot = mDoc->documentElement();
			if(pRoot==NULL||pRoot->nodeName().compare("VTKFile")!=0)
				return NULL;
			Element* pPolyData = pRoot->getChildElement("PolyData");
			if(pPolyData==NULL)
				return NULL;
			return pPolyData->getElementById(name,"name");
		}

		/**
		 * Read the points from the element
		 */
		void vtkGeometryCacheReader::readPoints(Poco::XML::Element* pEle,int *noOfPoints,double** points)
		{
			if(pEle==NULL)
			{
				*noOfPoints=0;
				return;
			}
			//Allocate memory
			*points=new double[(*noOfPoints)*3];
			if(*points==NULL) // Out of memory
			{
				PLog.error("Cannot allocate memory for triangle cache of Object ");
				return;
			}
			if(pEle->getAttribute("format").compare("ascii")==0) 
			{ //Read from Ascii
				std::stringstream buf;
				buf<<pEle->innerText();
				for(int i=0;i<(*noOfPoints)*3;i++)
				{
					buf>>(*points)[i];
				}
			}
			else
			{ //Read from binary
			}

		}

		/**
		 * Read triangle face indexs
		 */
		void vtkGeometryCacheReader::readTriangles(Poco::XML::Element* pEle,int *noOfTriangles,int** faces)
		{
			if(pEle==NULL)
			{
				*noOfTriangles=0;
				return;
			}
			//Allocate memory
			*faces=new int[(*noOfTriangles)*3];
			if(*faces==NULL) // Out of memory
			{
				PLog.error("Cannot allocate memory for triangle cache of Object ");
				return;
			}
			if(pEle->getAttribute("format").compare("ascii")==0) 
			{ //Read from Ascii
				std::stringstream buf;
				buf<<pEle->innerText();
				for(int i=0;i<(*noOfTriangles)*3;i++)
				{
					buf>>(*faces)[i];
				}
			}
			else
			{ //Read from binary
			}
		}
	}
}
