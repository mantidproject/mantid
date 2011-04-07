#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/System.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h> 
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor given the 4 dimensions
   * @param dimX :: X dimension binning parameters
   * @param dimY :: Y dimension binning parameters
   * @param dimZ :: Z dimension binning parameters
   * @param dimT :: T (time) dimension binning parameters
   */
  MDHistoWorkspace::MDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY,
      Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::Geometry::MDHistoDimension_sptr dimT)
  : numDimensions(0)
  {
    // For quick indexing, accumulate these values
    indexMultiplier[0] = dimX->getNBins();
    indexMultiplier[1] = dimY->getNBins() * indexMultiplier[0];
    indexMultiplier[2] = dimZ->getNBins() * indexMultiplier[1];
    m_length = dimT->getNBins() * indexMultiplier[2];
    // Allocate the linear arrays
    m_signals = new double[m_length];
    m_errors = new double[m_length];
    // Initialize them to zero (quickly)
    memset(m_signals, 0, sizeof(double)*m_length);
    memset(m_errors, 0, sizeof(double)*m_length);
    // Add all the dimensions
    m_dimensions.clear();
    addDimension(dimX);
    addDimension(dimY);
    addDimension(dimZ);
    addDimension(dimT);
    // Compute the volume of each cell.
    double volume = 1.0;
    for (size_t i=0; i < m_dimensions.size(); ++i)
      volume *= m_dimensions[i]->getBinWidth();
    m_inverseVolume = 1.0 / volume;
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspace::~MDHistoWorkspace()
  {
    delete [] m_signals;
    delete [] m_errors;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Return the memory used, in bytes */
  size_t MDHistoWorkspace::getMemorySize() const
  {
    return m_length * 2 * sizeof(double);
  }


  //----------------------------------------------------------------------------------------------
  /** Add a dimension to the workspace. These must be added in order of X, Y, Z, time
   *
   * @param dim :: sptr to the dimension object.
   */
  void MDHistoWorkspace::addDimension(Mantid::Geometry::MDHistoDimension_sptr dim)
  {
    if (numDimensions >= 4)
      throw std::runtime_error("MDHistoWorkspace can only have a maximum of 4 dimensions.");
    this->m_dimensions.push_back(dim);
    numDimensions = m_dimensions.size();
  }

  //----------------------------------------------------------------------------------------------
  /// Return a vector containing a copy of the signal data in the workspace.
  std::vector<double> MDHistoWorkspace::getSignalDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<double> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
        out[i] = m_signals[i];
    // This copies again! :(
    return out;
  }

  /// Return a vector containing a copy of the error data in the workspace.
  std::vector<double> MDHistoWorkspace::getErrorDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<double> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
        out[i] = m_errors[i];
    // This copies again! :(
    return out;
  }

  std::string MDHistoWorkspace::getGeometryXML() const
  {
    using namespace Poco::XML;

    //Create the root element for this fragment.
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> dimensionSetElement = pDoc->createElement("DimensionSet");
    pDoc->appendChild(dimensionSetElement);

    //Loop through dimensions and generate xml for each.
    std::string dimensionXMLString;
    for(size_t i = 0; i <m_dimensions.size(); i++)
    {
      dimensionXMLString += m_dimensions[i]->toXMLString();
    }

    //Pass dimensions to dimension set.
    dimensionSetElement->appendChild(pDoc->createTextNode("%s"));

    //x-dimension mapping.
    AutoPtr<Element> xDimensionElement = pDoc->createElement("XDimension");
    AutoPtr<Element> xDimensionIdElement = pDoc->createElement("RefDimensionId");
    std::string xDimensionId = this->getXDimension()->getDimensionId();
    AutoPtr<Text> idXText = pDoc->createTextNode(xDimensionId);
    xDimensionIdElement->appendChild(idXText);
    xDimensionElement->appendChild(xDimensionIdElement);
    dimensionSetElement->appendChild(xDimensionElement);

    //y-dimension mapping.
    AutoPtr<Element> yDimensionElement = pDoc->createElement("YDimension");
    AutoPtr<Element> yDimensionIdElement = pDoc->createElement("RefDimensionId");
    std::string yDimensionId = this->getYDimension()->getDimensionId();
    AutoPtr<Text> idYText = pDoc->createTextNode(yDimensionId);
    yDimensionIdElement->appendChild(idYText);
    yDimensionElement->appendChild(yDimensionIdElement);
    dimensionSetElement->appendChild(yDimensionElement);

    //z-dimension mapping.
    AutoPtr<Element> zDimensionElement = pDoc->createElement("ZDimension");
    AutoPtr<Element> zDimensionIdElement = pDoc->createElement("RefDimensionId");
    std::string zDimensionId = this->getZDimension()->getDimensionId();
    AutoPtr<Text> idZText = pDoc->createTextNode(zDimensionId);
    zDimensionIdElement->appendChild(idZText);
    zDimensionElement->appendChild(zDimensionIdElement);
    dimensionSetElement->appendChild(zDimensionElement);

    //t-dimension mapping.
    AutoPtr<Element> tDimensionElement = pDoc->createElement("TDimension");
    AutoPtr<Element> tDimensionIdElement = pDoc->createElement("RefDimensionId");
    std::string tDimensionId = this->getTDimension()->getDimensionId();
    AutoPtr<Text> idTText = pDoc->createTextNode(tDimensionId);
    tDimensionIdElement->appendChild(idTText);
    tDimensionElement->appendChild(tDimensionIdElement);
    dimensionSetElement->appendChild(tDimensionElement);

    std::stringstream xmlstream;
    DOMWriter writer;
    writer.writeNode(xmlstream, pDoc);

    std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str()) % dimensionXMLString.c_str());
    return formattedXMLString;
  }



} // namespace Mantid
} // namespace MDEvents

