#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/SlicingAlgorithm.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDPlane.h"

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
    std::string dimChars = getDimensionChars();

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
    if (Strings::strip(str).empty())
      return;

    std::string name, id, units;
    double min, max;
    int numBins = 0;
    std::vector<std::string> strs;
    boost::split(strs, str, boost::is_any_of(","));
    if (strs.size() != this->m_inWS->getNumDims() + 4)
      throw std::invalid_argument("Wrong number of values (expected 4 + # of input dimensions) in the dimensions string: " + str);
    // Extract the arguments
    name = Strings::strip(strs[0]);
    id = name;
    units = Strings::strip(strs[1]);
    Strings::convert(strs[ strs.size()-1 ], numBins);
    max = double(numBins);
    if (name.size() == 0)
      throw std::invalid_argument("name should not be blank.");
    if (numBins < 1)
      throw std::invalid_argument("number of bins should be >= 1.");

    double length = 0.0;
    Strings::convert(strs[ strs.size()-2 ], length);
    min = 0.0;
    max = length;

    if (length <= 0)
      throw std::invalid_argument("'length' parameter should be > 0.0.");

    // Create the basis vector with the right # of dimensions
    VMD basis(this->m_inWS->getNumDims());
    for (size_t d=0; d<this->m_inWS->getNumDims(); d++)
      Strings::convert(strs[d+2], basis[d]);
    double oldBasisLength = basis.normalize();

    if (oldBasisLength <= 0)
      throw std::invalid_argument("direction should not be 0-length.");

    // Now, convert the basis vector to the coordinates of the ORIGNAL ws, if any
    if (m_originalWS)
    {
      // Turn basis vector into two points
      VMD basis0(this->m_inWS->getNumDims());
      VMD basis1 = basis * length;
      // Convert the points to the original coordinates (from inWS to originalWS)
      CoordTransform * toOrig = m_inWS->getTransformToOriginal();
      VMD origBasis0 = toOrig->applyVMD(basis0);
      VMD origBasis1 = toOrig->applyVMD(basis1);
      // New basis vector, now in the original workspace
      basis = origBasis1 - origBasis0;
      // New length of the vector (in original space).
      length = basis.normalize();
    }

    // Scaling factor, to convert from units in the inDim to the output BIN number
    double scaling = double(numBins) / length;

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
  void SlicingAlgorithm::createGeneralTransform()
  {
    // Number of input dimensions
    size_t inD = m_inWS->getNumDims();

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
    m_origin = origin;

    // Now, convert the original vector to the coordinates of the ORIGNAL ws, if any
    if (m_originalWS)
    {
      CoordTransform * toOrig = m_inWS->getTransformToOriginal();
      m_origin = toOrig->applyVMD(m_origin);
    }

    // Validate
    if (outD > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");
    if (m_scaling.size() != outD)
      throw std::runtime_error("Inconsistent number of entries in scaling vector.");

    // Create the CoordTransformAffine with these basis vectors
    CoordTransformAffine * ct = new CoordTransformAffine(inD, outD);
    ct->buildOrthogonal(m_origin, this->m_bases, VMD(this->m_scaling) ); // note the scaling makes the coordinate correspond to a bin index
    this->m_transform = ct;

    // Transformation original->binned
    std::vector<double> unitScaling(outD, 1.0);
    CoordTransformAffine * ctFrom = new CoordTransformAffine(inD, outD);
    ctFrom->buildOrthogonal(m_origin, this->m_bases, VMD(unitScaling) );
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
   * Must be called in order X,Y,Z,T.
   *
   * @param str :: name,minimum,maximum,number_of_bins
   */
  void SlicingAlgorithm::makeAlignedDimensionFromString(const std::string & str)
  {
    if (str.empty())
    {
      throw std::runtime_error("Empty string passed to one of the AlignedDimX parameters.");
    }
    else
    {
      std::string name;
      double min, max;
      int numBins = 0;
      std::vector<std::string> strs;
      boost::split(strs, str, boost::is_any_of(","));
      if (strs.size() != 4)
        throw std::invalid_argument("Wrong number of values (4 are expected) in the dimensions string: " + str);
      // Extract the arguments
      name = Strings::strip(strs[0]);
      Strings::convert(strs[1], min);
      Strings::convert(strs[2], max);
      Strings::convert(strs[3], numBins);
      if (name.size() == 0)
        throw std::invalid_argument("Name should not be blank.");
      if (min >= max)
        throw std::invalid_argument("Min should be > max.");
      if (numBins < 1)
        throw std::invalid_argument("Number of bins should be >= 1.");

      std::string units = "";
      std::string id = name;
      // Find the named axis in the input workspace
      try
      {
        size_t dim_index = m_inWS->getDimensionIndexByName(name);
        IMDDimension_const_sptr inputDim = m_inWS->getDimension(dim_index);
        units = inputDim->getUnits();
        // Add the index from which we're binning to the vector
        this->dimensionToBinFrom.push_back(dim_index);
      }
      catch (std::runtime_error &)
      {
        // The dimension was not found, so this is a problem
        throw std::runtime_error("Dimension " + name + " was not found in the MDEventWorkspace! Cannot continue.");
      }

      binDimensions.push_back( MDHistoDimension_sptr(new MDHistoDimension(name, id, units, min, max, numBins)));
    }
  }
  //----------------------------------------------------------------------------------------------
  /** Using the parameters, create a coordinate transformation
   * for aligned cuts
   */
  void SlicingAlgorithm::createAlignedTransform()
  {
    std::string dimChars = "XYZT";

    // Validate inputs
    bool previousWasEmpty = false;
    size_t numDims = 0;
    for (size_t i=0; i<4; i++)
    {
      std::string propName = "AlignedDimX"; propName[10] = dimChars[i];
      std::string prop = Strings::strip(getPropertyValue(propName));
      if (!prop.empty()) numDims++;
      if (!prop.empty() && previousWasEmpty)
        throw std::invalid_argument("Please enter the AlignedDim parameters in the order X,Y,Z,T, without skipping any entries.");
      previousWasEmpty = prop.empty();
    }

    // Number of input dimension
    size_t inD = m_inWS->getNumDims();
    // Validate
    if (numDims == 0)
      throw std::runtime_error("No output dimensions specified.");
    if (numDims > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace.");

    // Create the dimensions based on the strings from the user
    for (size_t i=0; i<numDims; i++)
    {
      std::string propName = "AlignedDimX"; propName[10] = dimChars[i];
      makeAlignedDimensionFromString( getPropertyValue(propName));
    }

    // Number of output binning dimensions found
    outD = binDimensions.size();

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

    // Transform for binning
    m_transform = new CoordTransformAligned(m_inWS->getNumDims(), outD,
        dimensionToBinFrom, origin, scaling);

    // Transformation original->binned. There is no offset or scaling!
    std::vector<double> unitScaling(outD, 1.0);
    std::vector<double> zeroOrigin(outD, 0.0);
    m_transformFromOriginal = new CoordTransformAligned(inD, outD,
        dimensionToBinFrom, zeroOrigin, unitScaling);

    // Now the reverse transformation.
    if (outD == inD)
    {
      // Make the reverse map = if you're in the output dimension "od", what INPUT dimension index is that?
      std::vector<size_t> reverseDimensionMap(inD, 0);
      for (size_t od=0; od<dimensionToBinFrom.size(); od++)
        reverseDimensionMap[ dimensionToBinFrom[od] ] = od;
      m_transformToOriginal = new CoordTransformAligned(inD, outD,
          reverseDimensionMap, zeroOrigin, unitScaling);
    }
    else
      // Changed # of dimensions - can't reverse the transform
      m_transformToOriginal = NULL;
  }



  //-----------------------------------------------------------------------------------------------
  /** Read the algorithm properties and creates the appropriate transforms
   * for slicing the MDEventWorkspace.
   *
   * NOTE: The m_inWS member must be set first.
   * If the workspace is based on another, e.g. result from BinMD,
   * m_inWS will be modified to be the original workspace and the transformations
   * will be altered to match.
   *
   * The m_transform, m_transformFromOriginal and m_transformToOriginal transforms will be set.
   */
  void SlicingAlgorithm::createTransform()
  {
    if (!m_inWS)
      throw std::runtime_error("SlicingAlgorithm::createTransform(): input MDWorkspace must be set first!");
    if (boost::dynamic_pointer_cast<MatrixWorkspace>(m_inWS))
      throw std::runtime_error(this->name() + " cannot be run on a MatrixWorkspace!");

    // Is the transformation aligned with axes?
    m_axisAligned = getProperty("AxisAligned");

    // Refer to the original workspace. Make sure that is possible
    m_originalWS = boost::dynamic_pointer_cast<IMDWorkspace>(m_inWS->getOriginalWorkspace());
    if (m_originalWS)
    {
      if (m_axisAligned)
        throw std::runtime_error("Cannot perform axis-aligned binning on a MDHistoWorkspace. Please use non-axis aligned binning.");
      if (m_originalWS->getNumDims() != m_inWS->getNumDims())
        throw std::runtime_error("SlicingAlgorithm::createTransform(): Cannot propagate a transformation if the number of dimensions has changed.");
      if (!m_inWS->getTransformToOriginal())
        throw std::runtime_error("SlicingAlgorithm::createTransform(): Cannot propagate a transformation. There is no transformation saved from "
            + m_inWS->getName() + " back to " + m_originalWS->getName() + ".");
      g_log.notice() << "Performing " << this->name() << " on the original workspace, '" << m_originalWS->getName() << "'" << std::endl;
    }

    // Create the coordinate transformation
    m_transform = NULL;
    if (m_axisAligned)
      this->createAlignedTransform();
    else
      this->createGeneralTransform();

    // Finalize, for binnign MDHistoWorkspace
    if (m_originalWS)
    {
      // Replace the input workspace
      m_inWS = m_originalWS;
    }


//
//    if (m_inWS->hasOriginalWorkspace())
//    {
//      // A was transformed to B
//      // Now we transform B to C
//      // So we come up with the A -> C transformation
//
//      IMDWorkspace_sptr origWS = m_inWS->getOriginalWorkspace();
//      g_log.notice() << "Performing " << this->name() << " on the original workspace, '" << origWS->getName() << "'" << std::endl;
//
//      if (origWS->getNumDims() != m_inWS->getNumDims())
//        throw std::runtime_error("SlicingAlgorithm::createTransform(): Cannot propagate a transformation if the number of dimensions has changed.");
//
//      // A->C transformation
//      CoordTransform * fromOrig = CoordTransformAffine::combineTransformations( m_inWS->getTransformFromOriginal(), m_transformFromOriginal );
//      // C->A transformation
//      CoordTransform * toOrig = CoordTransformAffine::combineTransformations( m_transformToOriginal, m_inWS->getTransformToOriginal() );
//      // A->C binning transformation
//      CoordTransform * binningTransform = CoordTransformAffine::combineTransformations( m_inWS->getTransformFromOriginal(), m_transform );
//
//      // Replace the transforms
//      delete m_transformFromOriginal;
//      delete m_transformToOriginal;
//      delete m_transform;
//      m_transformFromOriginal = fromOrig;
//      m_transformToOriginal = toOrig;
//      m_transform = binningTransform;
//
//      coord_t in[2] = {0,0};
//      coord_t out[2] = {0,0};
//      m_transform->apply(in, out);
//      std::cout << "0,0 gets binningTransformed to  " << VMD(2, out) << std::endl;
//      in[0] = 10; in[1] = 10;
//      m_transform->apply(in, out);
//      std::cout << "10,10 gets binningTransformed to  " << VMD(2, out) << std::endl;
//
//      // Replace the input workspace
//      m_inWS = origWS;
//    }
  }


  //----------------------------------------------------------------------------------------------
  /** Create an implicit function for picking boxes, based on the indexes in the
   * output MDHistoWorkspace.
   * This needs to be in the space of the INPUT MDEventWorkspace.
   *
   * In the most general case, this function assumes ORTHOGONAL BASIS VECTORS!
   * However, the following cases handle non-orthogonal vectors:
   *  2 bases in 2D space
   *  2 bases in 3D space
   *  3 bases in 3D space
   *  3 bases in 4D space
   *
   * @param chunkMin :: the minimum index in each dimension to consider "valid" (inclusive).
   *        NULL to use the entire range.
   * @param chunkMax :: the maximum index in each dimension to consider "valid" (exclusive)
   *        NULL to use the entire range.
   * @return MDImplicitFunction created
   */
  MDImplicitFunction * SlicingAlgorithm::getGeneralImplicitFunction(size_t * chunkMin, size_t * chunkMax)
  {
    size_t nd = m_inWS->getNumDims();

    // General implicit function
    MDImplicitFunction * func = new MDImplicitFunction;

    // First origin = 0,0,0 (offset if you specified a chunk)
    VMD o1 = m_origin;
    // Second origin = max of each basis vector
    VMD o2 = m_origin;
    // And this the list of basis vectors. Each vertex is given by o1+bases[i].
    std::vector<VMD> bases;
    VMD x,y,z,t;

    for (size_t d=0; d<m_bases.size(); d++)
    {
      double xMin = 0;
      double xMax = binDimensions[d]->getX(binDimensions[d]->getNBins());
      if (chunkMin != NULL) xMin = binDimensions[d]->getX(chunkMin[d]);
      if (chunkMax != NULL) xMax = binDimensions[d]->getX(chunkMax[d]);
      o1 += (m_bases[d] * xMin);
      o2 += (m_bases[d] * xMax);
      VMD thisBase = m_bases[d] * (xMax-xMin);
      bases.push_back(thisBase);
      if (d==0) x = thisBase;
      if (d==1) y = thisBase;
      if (d==2) z = thisBase;
      if (d==3) t = thisBase;
    }

    // Dimensionality of the box
    size_t boxDim = bases.size();

    // Create a Z vector by doing the cross-product of X and Y
    if (boxDim==2 && nd == 3)
    { z = x.cross_prod(y); z.normalize(); }

    // Point that is sure to be inside the volume of interest
    VMD insidePoint = (o1 + o2) / 2.0;      VMD normal = bases[0];

    std::vector<VMD> points;

    if (boxDim == 1)
    {
      // 2 planes defined by 1 basis vector
      // Your normal = the x vector
      func->addPlane( MDPlane(x, o1) );
      func->addPlane( MDPlane(x * -1.0, o2) );
    }
    else if (boxDim == 2 && nd == 2)
    {
      // 4 planes defined by 2 basis vectors (general to non-orthogonal basis vectors)
      std::vector<VMD> vectors;
      // X plane
      vectors.clear();
      vectors.push_back(x);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );

      // Y plane
      vectors.clear();
      vectors.push_back(y);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );
    }
    else if (boxDim == 2 && nd == 3)
    {
      // 4 planes defined by 2 basis vectors (general to non-orthogonal basis vectors)
      // The vertical = cross-product of X and Y basis vectors
      z = x.cross_prod(y);
      std::vector<VMD> vectors;

      // X plane
      vectors.clear();
      vectors.push_back(x);
      vectors.push_back(z);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );

      // Y plane
      vectors.clear();
      vectors.push_back(y);
      vectors.push_back(z);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );
    }
    else if (boxDim == 3 && nd == 3)
    {
      // 6 planes defined by 3 basis vectors (general to non-orthogonal basis vectors)
      VMD xyNormal = x.cross_prod(y);
      // Flip the normal if the input bases were NOT right-handed.
      if (!MDPlane(xyNormal, o1).isPointBounded(insidePoint)) xyNormal *= -1.0;
      func->addPlane( MDPlane(xyNormal,        o1) );
      func->addPlane( MDPlane(xyNormal * -1.0, o2) );
      VMD xzNormal = z.cross_prod(x);
      if (!MDPlane(xzNormal, o1).isPointBounded(insidePoint)) xzNormal *= -1.0;
      func->addPlane( MDPlane(xzNormal,        o1) );
      func->addPlane( MDPlane(xzNormal * -1.0, o2) );
      VMD yzNormal = y.cross_prod(z);
      if (!MDPlane(yzNormal, o1).isPointBounded(insidePoint)) yzNormal *= -1.0;
      func->addPlane( MDPlane(yzNormal,        o1) );
      func->addPlane( MDPlane(yzNormal * -1.0, o2) );
    }
    else if (boxDim == 3 && nd == 4)
    {
      // 6 planes defined by 3 basis vectors, in 4D world. (General to non-orthogonal basis vectors)

      // Get a vector (in 4D) that is normal to all 3 other bases.
      t = VMD::getNormalVector(bases);
      // All the planes will have "t" in their plane.

      std::vector<VMD> vectors;

      // XY plane
      vectors.clear();
      vectors.push_back(x);
      vectors.push_back(y);
      vectors.push_back(t);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );

      // XZ plane
      vectors.clear();
      vectors.push_back(x);
      vectors.push_back(z);
      vectors.push_back(t);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );

      // YZ plane
      vectors.clear();
      vectors.push_back(y);
      vectors.push_back(z);
      vectors.push_back(t);
      func->addPlane( MDPlane(vectors, o1, insidePoint) );
      func->addPlane( MDPlane(vectors, o2, insidePoint) );
    }
    else
    {
      // Last-resort, totally general case
      // 2*N planes defined by N basis vectors, in any dimensionality workspace. Assumes orthogonality!
      for (size_t i=0; i<bases.size(); i++)
      {
        // For each basis vector, make two planes, perpendicular to it and facing inwards
        func->addPlane( MDPlane(bases[i],        o1) );
        func->addPlane( MDPlane(bases[i] * -1.0, o2) );
      }
    }

    return func;
  }


  //----------------------------------------------------------------------------------------------
  /** Create an implicit function for picking boxes, based on the indexes in the
   * output MDHistoWorkspace.
   * This needs to be in the space of the INPUT MDEventWorkspace
   *
   * @param chunkMin :: the minimum index in each dimension to consider "valid" (inclusive).
   *        NULL to use the entire range.
   * @param chunkMax :: the maximum index in each dimension to consider "valid" (exclusive)
   *        NULL to use the entire range.
   * @return MDImplicitFunction created
   */
  MDImplicitFunction * SlicingAlgorithm::getImplicitFunctionForChunk(size_t * chunkMin, size_t * chunkMax)
  {
    size_t nd = m_inWS->getNumDims();
    if (m_axisAligned)
    {
      std::vector<coord_t> function_min(nd, -1e50); // default to all space if the dimension is not specified
      std::vector<coord_t> function_max(nd, +1e50); // default to all space if the dimension is not specified
      for (size_t bd=0; bd<outD; bd++)
      {
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        if (chunkMin)
          function_min[d] = binDimensions[bd]->getX(chunkMin[bd]);
        else
          function_min[d] = binDimensions[bd]->getX(0);
        if (chunkMax)
          function_max[d] = binDimensions[bd]->getX(chunkMax[bd]);
        else
          function_max[d] = binDimensions[bd]->getX(binDimensions[bd]->getNBins());
      }
      MDBoxImplicitFunction * function = new MDBoxImplicitFunction(function_min, function_max);
      return function;
    }
    else
    {
      // General implicit function
      return getGeneralImplicitFunction(chunkMin, chunkMax);
    }
  }


} // namespace Mantid
} // namespace MDEvents

