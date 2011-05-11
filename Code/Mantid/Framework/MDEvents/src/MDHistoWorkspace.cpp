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
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
    if (dimX) dimensions.push_back(dimX);
    if (dimY) dimensions.push_back(dimY);
    if (dimZ) dimensions.push_back(dimZ);
    if (dimT) dimensions.push_back(dimT);
    this->init(dimensions);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor given a vector of dimensions
   * @param dimensions :: vector of MDHistoDimension; no limit to how many.
   */
  MDHistoWorkspace::MDHistoWorkspace(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions)
  : numDimensions(0)
  {
    this->init(dimensions);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor helper method
   * @param dimensions :: vector of MDHistoDimension; no limit to how many.
   */
  void MDHistoWorkspace::init(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions)
  {
    if (dimensions.size() == 0)
      throw std::invalid_argument("0 valid dimensions were given to the MDHistoWorkspace constructor!");

    // Copy the dimensions array
    m_dimensions = dimensions;
    numDimensions = m_dimensions.size();

    // For indexing.
    if (numDimensions < 4)
      indexMultiplier = new size_t[4];
    else
      indexMultiplier = new size_t[numDimensions];

    // For quick indexing, accumulate these values
    // First multiplier
    indexMultiplier[0] = m_dimensions[0]->getNBins();
    for (size_t d=1; d<numDimensions; d++)
      indexMultiplier[d] = indexMultiplier[d-1] * m_dimensions[d]->getNBins();

    // This is how many dense data points
    m_length = indexMultiplier[numDimensions-1];

    // Now fix things for < 4 dimensions. Indices > the number of dimensions will be ignored (*0)
    for (size_t d=numDimensions-1; d<4; d++)
      indexMultiplier[d] = 0;

    // Allocate the linear arrays
    m_signals = new double[m_length];
    m_errors = new double[m_length];

    // Initialize them to NAN (quickly)
    double nan = std::numeric_limits<double>::quiet_NaN();
    for (size_t i=0; i < m_length; i++)
    {
      m_signals[i] = nan;
      m_errors[i] = nan;
    }

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
    delete [] indexMultiplier;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Return the memory used, in bytes */
  size_t MDHistoWorkspace::getMemorySize() const
  {
    return m_length * 2 * sizeof(double);
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


  //---------------------------------------------------------------------------------------------------
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

