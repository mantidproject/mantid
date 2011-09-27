#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/SlicingAlgorithm.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidMDEvents/CoordTransformAligned.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SlicingAlgorithm::SlicingAlgorithm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SlicingAlgorithm::~SlicingAlgorithm()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SlicingAlgorithm::initSlicingProps()
  {
    std::string dimChars = "XYZT";

    // --------------- Axis-aligned properties ---------------------------------------
    declareProperty("AxisAligned", true, "Perform binning aligned with the axes of the input MDEventWorkspace?");
    setPropertyGroup("AxisAligned", "Axis-Aligned Binning");
    for (size_t i=0; i<4; i++)
    {
      std::string dim(" "); dim[0] = dimChars[i];
      std::string propName = "AlignedDim" + dim;
      declareProperty(new PropertyWithValue<std::string>(propName,"",Direction::Input),
          "Binning parameters for the " + dim + " dimension.\n"
          "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
      setPropertySettings(propName, new EnabledWhenProperty(this, "AxisAligned", IS_EQUAL_TO, "1") );
      setPropertyGroup(propName, "Axis-Aligned Binning");
    }

    // --------------- NON-Axis-aligned properties ---------------------------------------
//    declareProperty(new PropertyWithValue<std::string>("TransformationXML","",Direction::Input),
//            "XML string describing the coordinate transformation that converts from the MDEventWorkspace dimensions to the output dimensions.\n");
//    setPropertyGroup("TransformationXML", "Non-Aligned Binning");

    IPropertySettings * ps = new EnabledWhenProperty(this, "AxisAligned", IS_EQUAL_TO, "0") ;
    for (size_t i=0; i<4; i++)
    {
      std::string dim(" "); dim[0] = dimChars[i];
      std::string propName = "BasisVector" + dim;
      declareProperty(new PropertyWithValue<std::string>(propName,"",Direction::Input),
          "Description of the basis vector of the output dimension " + dim + "."
          "Format: 'name, units, x,y,z,.., length, number_of_bins'.\n"
          "  x,y,z,...: vector definining the basis in the input dimensions space.\n"
          "  length: length of this dimension in the input space.\n"
          "  number_of_bins: separate 'length' into this many bins\n"
          "Leave blank for NONE." );
      setPropertySettings(propName, ps->clone() );
      setPropertyGroup(propName, "Non-Aligned Binning");
    }
    declareProperty(new PropertyWithValue<std::string>("Origin","",Direction::Input),
        "Origin (in the input workspace) that corresponds to (0,0,0) in the output MDHistoWorkspace.\n"
        "Enter as a comma-separated string." );
    declareProperty(new PropertyWithValue<bool>("ForceOrthogonal", false, Direction::Input),
        "Force the input basis vectors to form an orthogonal coordinate system. Only works in 3 dimension!" );
    // For GUI niceness
    setPropertyGroup("Origin", "Non-Aligned Binning");
    setPropertyGroup("ForceOrthogonal", "Non-Aligned Binning");
    setPropertySettings("Origin", ps->clone() );
    setPropertySettings("ForceOrthogonal", ps->clone() );

  }




  //----------------------------------------------------------------------------------------------
  /** Generate the MDHistoDimension and basis vector for a given string from BasisVectorX etc.
   *
   *  "Format: 'name, units, x,y,z,.., length, number_of_bins'.\n"
   *
   * @param str :: name,number_of_bins
   * @return a vector describing the basis vector in the input dimensions.
   */
  void SlicingAlgorithm::makeBasisVectorFromString(const std::string & str)
  {
    if (str.empty())
      return;

    std::string name, id, units;
    double min, max;
    int numBins = 0;
    std::vector<std::string> strs;
    boost::split(strs, str, boost::is_any_of(","));
    if (strs.size() != this->in_ws->getNumDims() + 4)
      throw std::invalid_argument("Wrong number of values (expected 4 + # of input dimensions) in the dimensions string: " + str);
    // Extract the arguments
    name = Strings::strip(strs[0]);
    id = name;
    units = Strings::strip(strs[1]);
    Strings::convert(strs[ strs.size()-1 ], numBins);
    max = double(numBins);
    if (name.size() == 0)
      throw std::invalid_argument("Name should not be blank.");
    if (numBins < 1)
      throw std::invalid_argument("Number of bins should be >= 1.");

    double length = 0.0;
    Strings::convert(strs[ strs.size()-2 ], length);
    min = 0.0;
    max = length;
    // Scaling factor, to convert from units in the inDim to the output BIN number
    double scaling = double(numBins) / length;

    // Create the basis vector with the right # of dimensions
    VMD basis(this->in_ws->getNumDims());
    for (size_t d=0; d<this->in_ws->getNumDims(); d++)
      Strings::convert(strs[d+2], basis[d]);

    // Create the output dimension
    MDHistoDimension_sptr out(new MDHistoDimension(name, id, units, min, max, numBins));

    // Put both in the algo for future use
    m_bases.push_back(basis);
    binDimensions.push_back(out);
    m_scaling.push_back(scaling);
  }


  //----------------------------------------------------------------------------------------------
  /** Loads the dimensions and create the coordinate transform, using the inputs.
   * This is for the general (i.e. non-aligned) case
   */
  void SlicingAlgorithm::createTransform()
  {
    // Number of input dimensions
    size_t inD = in_ws->getNumDims();

    // Create the dimensions based on the strings from the user
    std::string dimChars = "XYZT";
    for (size_t i=0; i<4; i++)
    {
      std::string propName = "BasisVectorX"; propName[11] = dimChars[i];
      try
      { makeBasisVectorFromString( getPropertyValue(propName) ); }
      catch (std::exception & e)
      { throw std::invalid_argument("Error parsing the " + propName + " parameter: " + std::string(e.what()) ); }
    }

    // Number of output binning dimensions found
    outD = binDimensions.size();
    if (outD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");

    bool ForceOrthogonal = getProperty("ForceOrthogonal");
    if (ForceOrthogonal && m_bases[0].getNumDims() == 3 && m_bases.size() >= 2)
    {
      std::vector<VMD> firstTwo = m_bases;
      firstTwo.resize(2, VMD(3));
      std::vector<VMD> ortho = VMD::makeVectorsOrthogonal(firstTwo);
      // Set the bases back
      ortho.resize(m_bases.size(), VMD(3));
      m_bases = ortho;
      g_log.information() << "Basis vectors forced to be orthogonal: ";
      for (size_t i=0; i<m_bases.size(); i++)
        g_log.information() << m_bases[i].toString(",") << "; ";
      g_log.information() << std::endl;

    }

    // Get the origin
    VMD origin;
    try
    { origin = VMD( getPropertyValue("Origin") ); }
    catch (std::exception & e)
    { throw std::invalid_argument("Error parsing the Origin parameter: " + std::string(e.what()) ); }
    if (origin.getNumDims() != inD)
      throw std::invalid_argument("The number of dimensions in the Origin parameter is not consistent with the number of dimensions in the input workspace.");

    // Validate
    if (outD > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");
    if (m_scaling.size() != outD)
      throw std::runtime_error("Inconsistent number of entries in scaling vector.");

    // Create the CoordTransformAffine with these basis vectors
    CoordTransformAffine * ct = new CoordTransformAffine(inD, outD);
    ct->buildOrthogonal(origin, this->m_bases, VMD(this->m_scaling) ); // note the scaling makes the coordinate correspond to a bin index
    this->m_transform = ct;

    // Transformation original->binned
    std::vector<double> unitScaling(outD, 1.0);
    CoordTransformAffine * ctFrom = new CoordTransformAffine(inD, outD);
    ctFrom->buildOrthogonal(origin, this->m_bases, VMD(unitScaling) );
    m_transformFromOriginal = ctFrom;

    // Validate
    if (m_transform->getInD() != inD)
      throw std::invalid_argument("The number of input dimensions in the CoordinateTransform object is not consistent with the number of dimensions in the input workspace.");
    if (m_transform->getOutD() != outD)
      throw std::invalid_argument("The number of output dimensions in the CoordinateTransform object is not consistent with the number of dimensions specified in the OutDimX, etc. properties.");

    // Now the reverse transformation
    m_transformToOriginal = NULL;
    if (outD == inD)
    {
      // Can't reverse transform if you lost dimensions.
      CoordTransformAffine * ctTo = new CoordTransformAffine(inD, outD);
      Matrix<coord_t> fromMatrix = ctFrom->getMatrix();
      Matrix<coord_t> toMatrix = fromMatrix;
      // Invert the affine matrix to get the reverse transformation
      toMatrix.Invert();
      ctTo->setMatrix( toMatrix );
      m_transformToOriginal = ctTo;
    }

  }






  //----------------------------------------------------------------------------------------------
  /** Generate a MDHistoDimension_sptr from a comma-sep string (for AlignedDimX, etc.)
   *
   * @param str :: name,minimum,maximum,number_of_bins
   * @return
   */
  MDHistoDimension_sptr makeMDHistoDimensionFromString(const std::string & str)
  {
    if (str.empty())
    {
      // Make a blank dimension
      MDHistoDimension_sptr out;
      return out;
    }
    else
    {
      std::string name, id;
      double min, max;
      int numBins = 0;
      std::vector<std::string> strs;
      boost::split(strs, str, boost::is_any_of(","));
      if (strs.size() != 4)
        throw std::invalid_argument("Wrong number of values (4 are expected) in the dimensions string: " + str);
      // Extract the arguments
      name = Strings::strip(strs[0]);
      id = name;
      Strings::convert(strs[1], min);
      Strings::convert(strs[2], max);
      Strings::convert(strs[3], numBins);
      if (name.size() == 0)
        throw std::invalid_argument("Name should not be blank.");
      if (min >= max)
        throw std::invalid_argument("Min should be > max.");
      if (numBins < 1)
        throw std::invalid_argument("Number of bins should be >= 1.");

      MDHistoDimension_sptr out(new MDHistoDimension(name, id, "", min, max, numBins));
      return out;
    }
  }
  //----------------------------------------------------------------------------------------------
  /** Using the parameters, create a coordinate transformation
   * for aligned cuts
   */
  void SlicingAlgorithm::createAlignedTransform()
  {
    // Create the dimensions based on the strings from the user
    std::string dimChars = "XYZT";
    for (size_t i=0; i<4; i++)
    {
      std::string propName = "AlignedDimX"; propName[10] = dimChars[i];
      MDHistoDimension_sptr binDim = makeMDHistoDimensionFromString( getPropertyValue(propName));
      if (binDim)
      {
        // (valid pointer?)
        if (binDim->getNBins() == 0)
          throw std::runtime_error("Dimension " + binDim->getName() + " was set to have 0 bins. Cannot continue.");

        // We have to find the dimension in the INPUT workspace to set to the OUTPUT workspace
        try {
          size_t dim_index = in_ws->getDimensionIndexByName(binDim->getName());
          dimensionToBinFrom.push_back(dim_index);
        }
        catch (std::runtime_error &)
        {
          // The dimension was not found, so we are not binning across it.
          if (binDim->getNBins() > 1)
            throw std::runtime_error("Dimension " + binDim->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
        }
        // This is a dimension we'll use in the output
        binDimensions.push_back(binDim);
      }
    }
    // Number of output binning dimensions found
    outD = binDimensions.size();
    // Number of input dimension
    size_t inD = in_ws->getNumDims();

    // Validate
    if (outD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
    if (outD > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");

    // Now we build the coordinate transformation object
    m_origin = VMD(inD);
    m_bases.clear();
    std::vector<coord_t> origin(outD), scaling(outD);
    for (size_t d=0; d<outD; d++)
    {
      origin[d] = binDimensions[d]->getMinimum();
      scaling[d] = coord_t(1.0) / binDimensions[d]->getBinWidth();
      // Origin in the input
      m_origin[ dimensionToBinFrom[d] ] = origin[d];
      // Create a unit basis vector that corresponds to this
      VMD basis(inD);
      basis[ dimensionToBinFrom[d] ] = 1.0;
      m_bases.push_back(basis);
    }

    m_transform = new CoordTransformAligned(in_ws->getNumDims(), outD,
        dimensionToBinFrom, origin, scaling);

    // Transformation original->binned
    std::vector<double> unitScaling(outD, 1.0);
    m_transformFromOriginal = new CoordTransformAligned(in_ws->getNumDims(), outD,
        dimensionToBinFrom, origin, unitScaling);

    // Now the reverse transformation. This is not possible in general.
    m_transformToOriginal = NULL;
  }







} // namespace Mantid
} // namespace MDEvents

