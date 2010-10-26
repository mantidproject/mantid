#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MDDataObjects/point3D.h"
#include <cmath>
#include <vector>

namespace Mantid
{
    namespace MDAlgorithms
    {

        PlaneImplicitFunction::PlaneImplicitFunction(NormalParameter normal, OriginParameter origin): m_normal(normal), m_origin(origin)
        {

        }

        bool PlaneImplicitFunction::evaluate(const MDDataObjects::point3D* pPoint) const
        {
            std::vector<double> num; 
            dotProduct<double>(pPoint->GetX() - m_origin.getX(), pPoint->GetY() - m_origin.getY(), pPoint->GetZ() - m_origin.getZ(), m_normal.getX(), m_normal.getY(), m_normal.getZ(), num);
            //return num.at(0) + num.at(1) + num.at(2) / absolute(normalX, normalY, normalZ) <= 0; //Calculates distance, but magnituted of normal not important in this algorithm
            return num.at(0) + num.at(1) + num.at(2)  <= 0;
        }

        bool PlaneImplicitFunction::operator==(const PlaneImplicitFunction &other) const
        {
            return this->m_normal == other.m_normal && this->m_origin == other.m_origin;
        }
        
        bool PlaneImplicitFunction::operator!=(const PlaneImplicitFunction &other) const
        {
            return !(*this == other);
        }

        std::string PlaneImplicitFunction::getName() const
        {
            return functionName();
        }

        double PlaneImplicitFunction::getOriginX() const
        {
            return this->m_origin.getX();
        }

        double PlaneImplicitFunction::getOriginY() const
        {
            return this->m_origin.getY();
        }

        double PlaneImplicitFunction::getOriginZ() const
        {
            return this->m_origin.getZ();
        }

        double PlaneImplicitFunction::getNormalX() const
        {
            return this->m_normal.getX();
        }

        double PlaneImplicitFunction::getNormalY() const
        {
            return this->m_normal.getY();
        }

        double PlaneImplicitFunction::getNormalZ() const
        {
            return this->m_normal.getZ();
        }

        std::string PlaneImplicitFunction::toXMLString() const 
        {
            using namespace Poco::XML;

            AutoPtr<Document> pDoc = new Document;
            AutoPtr<Element> functionElement = pDoc->createElement("Function");
            pDoc->appendChild(functionElement);
            AutoPtr<Element> typeElement = pDoc->createElement("Type");
            AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
            typeElement->appendChild(typeText);
            functionElement->appendChild(typeElement);

            AutoPtr<Element> paramListElement = pDoc->createElement("ParameterList");

            AutoPtr<Text> formatText = pDoc->createTextNode("%s%s");
            paramListElement->appendChild(formatText);
            functionElement->appendChild(paramListElement);

            std::stringstream xmlstream;

            DOMWriter writer;
            writer.writeNode(xmlstream, pDoc);

            std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str()) % m_normal.toXMLString().c_str() % m_origin.toXMLString().c_str());
            return formattedXMLString;
        }

        PlaneImplicitFunction::~PlaneImplicitFunction()
        {
        }
    }


}