#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/Exception.h"
#include <iostream>


using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Document;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using Poco::XML::Node;
using Poco::XML::AutoPtr;
using Poco::Exception;

#include "MantidGeometry/Object.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/vtkGeometryCacheReader.h"


namespace Mantid
{
	namespace Geometry
	{

		/**
		* Constructor
		*/
		vtkGeometryCacheReader::vtkGeometryCacheReader(std::string filename)
		{
			//mFileName=filename;
			//mDoc=new Document();
			//Init();
		}

		/**
		* Destructor
		*/
		vtkGeometryCacheReader::~vtkGeometryCacheReader()
		{
		}

	}
}
