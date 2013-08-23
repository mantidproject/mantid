/*WIKI*
RingProfile sums the counts against a ring.

Below, there is an example of the execution of the RingProfile to a [[Workspace2D]] where the position of the pixels are not associated 
to detector positions, but it is derived from the [[Interacting_with_Matrix_Workspaces#Axes | Axes]]. 

[[File:ExecuteRingProfile.png | 800px]]

The image below shows a visual interpretation for the inputs of the algorithm

[[File:RingProfileInputsView.png]]

The algorithm goes through each pixel and find its distance from the center. If it relies inside the defined ring, it checks the angle 
between the pixel position and the center and uses this information to define the bin where to put the count for that pixel. 


The RingProfile is also defined for Workspace2D which has the positions based on the detectors, as you can see in the picture below.

[[File:RingProfileInstrument.png | 800px ]]


In this case, the inputs of the algorithm is like the image below

[[File:Ringprofileinstrument.png]]


The algorithm does to each spectrum, get the associated detector from which it get the positions. From the positions it work out if it belongs or not to the ring and in which bin it must be placed. It finally accumulate all the spectrum values inside the target bin.
*WIKI*/

#include "MantidAlgorithms/RingProfile.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include <cmath>
#include <climits>
#include <MantidAPI/IEventWorkspace.h>
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RingProfile)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RingProfile::RingProfile()
  {
    
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RingProfile::~RingProfile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RingProfile::initDocs()
  {
    this->setWikiSummary("Calculates the sum of the counts against a circular ring.");
    this->setOptionalMessage("Calculates the sum of the counts against a circular ring.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
      It configure the algorithm to accept the following inputs: 

       - InputWorkspace
       - OutputWorkspace
       - Centre
       - MinRadius
       - MaxRadius
       - NumBins
       - StartAngle
       - Sense
   */
  void RingProfile::init()
  {
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Kernel::Direction::Input), "An input workspace.");
    declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Kernel::Direction::Output), "An output workspace.");


    auto twoOrThree = boost::make_shared<Kernel::ArrayLengthValidator<double> >(2,3);
    std::vector<double> myInput(3,0);
    declareProperty(new Kernel::ArrayProperty<double>("Centre", myInput, twoOrThree), "Coordinate of the centre of the ring");
    auto nonNegative = boost::make_shared<Kernel::BoundedValidator<double> >();
    nonNegative->setLower(0);

    declareProperty<double>("MinRadius", 0, nonNegative, "Lenght of the inner ring"); 
    declareProperty(new Kernel::PropertyWithValue<double>("MaxRadius", std::numeric_limits<double>::max(), nonNegative), 
                    "Lenght of the outer ring");
    auto nonNegativeInt = boost::make_shared<Kernel::BoundedValidator<int> >();
    nonNegativeInt->setLower(1);
    declareProperty<int>("NumBins",100, nonNegativeInt, "Number of slice bins for the output");
    auto degreesLimits = boost::make_shared<Kernel::BoundedValidator<double> >();
    degreesLimits->setLower(-360); 
    degreesLimits->setUpper(360);
    declareProperty<double>("StartAngle",0,degreesLimits, "The angle to start from."); 
    
    std::vector<std::string > op(2); 
    op[0] = "ClockWise"; 
    op[1] = "Anti-ClockWise"; 
    declareProperty("Sense", "Anti-ClockWise", boost::make_shared<Kernel::StringListValidator>(op), 
                    "The direction of the integration around the ring"); 
                    

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
      
      The algorithm is executed in the following order:
       
        - Check is performed to see if all the inputs are valid to allow an answer. 
        - Perform the ring profile algorithm 
        - Configure the output of the algorithm

     The execution of the first two steps depend on the nature of the input. 
     If the input is a workspace whose positions are held by the instrument connected to the workspace, 
     it process the two steps with the following methods: 
      - checkInputsForSpectraWorkspace
      - processInstrumentRingProfile

     If the workspace must be dealt with as a flat 2D image, these steps will be performed by: 
      - checkInputsForNumericWorkspace
      - processNumericImageRingProfile
      
   */
  void RingProfile::exec()
  {
    // get input workspace
    API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

    // the RingProfile does not support eventworkspace
    auto checkEvent = boost::dynamic_pointer_cast<API::IEventWorkspace>(inputWS); 
    if (checkEvent){
       throw std::invalid_argument("RingProfile is not defined for EventWorkspaces.");
    }
    g_log.debug() << "Get the input parameters " << std::endl; 
    // get the algorithm parameters
    std::vector<double> centre = getProperty("Centre");
    centre_x = centre[0]; 
    centre_y = centre[1]; 
    if (centre.size() == 3) 
      centre_z = centre[2]; 
    min_radius = getProperty("MinRadius");
    max_radius = getProperty("MaxRadius");
    start_angle = getProperty("StartAngle");
    num_bins = getProperty("NumBins"); 
    bin_size = 360.0/num_bins; 
    clockwise = (getPropertyValue("Sense") == "ClockWise");

    g_log.debug() << "Check the inputs of the algorithm" << std::endl; 
    // VALIDATE THE INPUTS    
    if (inputWS->getAxis(1)->isSpectra()){
      checkInputsForSpectraWorkspace(inputWS); 
    }else{
      checkInputsForNumericWorkspace(inputWS); 
    }

    // prepare the vector to hold the output
    std::vector<double> output_bins(num_bins, 0);

    g_log.debug() << "Execute the ring profile calculation" << std::endl; 
    // perform the ring profile calculation 
    if (inputWS->getAxis(1)->isSpectra()){      
      processInstrumentRingProfile(inputWS, output_bins); 
    
    }else{
      processNumericImageRingProfile(inputWS, output_bins); 
    }

    g_log.debug() << "Prepare the output" << std::endl; 
    // create the output
    API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(inputWS, 1, 
                                                                                  output_bins.size()+1, output_bins.size());
  
    // populate Y data getting the values from the output_bins
    MantidVec & refY = outputWS->dataY(0);
    if (clockwise){
      for (size_t j = 0; j< output_bins.size() ; j++)
        refY[j] = output_bins[output_bins.size()-j-1];
    }else{
      for (size_t j = 0; j< output_bins.size() ; j++)
        refY[j] = output_bins[j];
    }
    
    // prepare the output for the angles:
    // populate X data
    MantidVec & refX = outputWS->dataX(0);
    
    std::vector<double> angles(output_bins.size()+1); 
    if (clockwise){
      for (int j=0; j<(int)angles.size(); j++)
        refX[j] = start_angle - bin_size * j; 
    }else{
      for (int j=0; j<(int)angles.size(); j++)
        refX[j] = start_angle + bin_size * j;     
    }
    
    // configure the axis
    
    // the horizontal axis is configured as degrees and copy the values of X
    API::Axis * const horizontal = new API::NumericAxis(refX.size()); 
    horizontal->unit() = boost::shared_ptr<Kernel::Unit>(new Kernel::Units::Degrees);
    for (size_t j=0; j< refX.size() ; j++)
      horizontal->setValue(j, refX[j]);  
    outputWS->replaceAxis(0, horizontal); 
    
    // the vertical axis get the same unit and information from the input workspace  
    API::Axis * const verticalAxis = new API::TextAxis(1);
    verticalAxis->unit() = inputWS->getAxis(1)->unit();
    verticalAxis->title() = inputWS->getAxis(1)->title(); 
    outputWS->replaceAxis(1, verticalAxis); 
    
    // set up the output
    setProperty("OutputWorkspace",outputWS); 
  }


/**
 * Validation of the inputs of the RingProfile algorithm. 
 *
 * Inside this method, the Workspace is considered an instrument based instrument. Each spectrum
 * has a detector associated which has a position in the 3D space. 
 *
 * The main validation are: 
 *  - the centre of the ring is inside the image it self.
 *  - The minimum ring is smaller than the limits of the image to allow
 *
 * @param inputWS: the input workspace
*/
void RingProfile::checkInputsForSpectraWorkspace(const API::MatrixWorkspace_sptr inputWS){
  try{
    auto det = inputWS->readX(inputWS->getNumberHistograms()/2); 
    auto instrument = inputWS->getInstrument(); 
    Geometry::BoundingBox box; 
    instrument->getBoundingBox(box); 
    std::stringstream limits_s; 
    limits_s << "(["
             << box.xMin() << ", " << box.xMax() << "], ["
             << box.yMin() << ", " << box.yMax() << "], ["
             << box.zMin() << ", " << box.zMax() << "])"; 
    g_log.notice() << "The limits for the instrument is : " << limits_s.str() << std::endl;  
    int xOutside=0, yOutside=0, zOutside=0;
    if (centre_x < box.xMin() || centre_x > box.xMax() )
      xOutside = 1; 
    if (centre_y < box.yMin() || centre_y > box.yMax() )
      yOutside = 1; 
    if (centre_z < box.zMin() || centre_z > box.zMax() )
      zOutside = 1;
    int summed = xOutside + yOutside + zOutside;     
    // if at least 2 are outside, the centre is considered outside the box. 
    if (summed >= 2){
      std::stringstream s; 
      s << "The defined centre (" << centre_x << ", " << centre_y << ", " << centre_z 
        << ") is outside the limits of the detectors inside this instrument: " << limits_s.str(); 
      throw std::invalid_argument(s.str());       
    }
    
    xOutside = yOutside = zOutside = 0; 
    if (centre_x - min_radius > box.xMax() || centre_x + min_radius < box.xMin())
      xOutside = 1; 
    if (centre_y - min_radius > box.yMax() || centre_y + min_radius < box.yMin())
      yOutside = 1; 
    if (centre_z - min_radius > box.zMax() || centre_z + min_radius < box.zMin())
      zOutside = 1;

    if (summed >= 2){
      std::stringstream s; 
      s << "The defined minRadius make the inner ring outside the limits of the detectors inside this instrument: " 
        << limits_s.str(); 
      throw std::invalid_argument(s.str());           
    }
    

  }catch(Kernel::Exception::NotFoundError & ){
    throw std::invalid_argument("Invalid input workspace. This workspace does not has detectors to get the positions from. "); 
  }
}


/**
 * Validation of the inputs of the RingProfile algorithm. 
 *
 * Inside this method, the Workspace is considered a 2D Matrix, where each spectrum is
 * the rows of the matrix and have the variation in axis0. The columns of the matrix 
 * is the position of dataX(0)
 * 
 * The main validation are: 
 *  - the centre of the ring is inside the image it self.
 *  - The minimum ring is smaller than the limits of the image to allow 
 * @param inputWS: pointer to the input workspace
*/
void RingProfile::checkInputsForNumericWorkspace(const API::MatrixWorkspace_sptr inputWS){
  g_log.notice() << "CheckingInputs For Numeric Workspace" << std::endl; 

  // The Axis0 is defined by the values of readX inside the spectra of the workspace. 
  // The limits of this axis will be get by inspection of the readX vector taking the first 
  // and the last value. 
  
  // check that centre is inside the range available for the instrument
  const MantidVec & refX  = inputWS->readX(inputWS->getNumberHistograms()/2);  
  // get the limits of the axis 0 (X)
  double min_v_x, max_v_x;
  min_v_x = std::min(refX[0], refX[refX.size() -1]); 
  max_v_x = std::max(refX[0], refX[refX.size() -1]); 
  g_log.notice() << "Limits X = " << min_v_x << " " << max_v_x << std::endl;   
  // check centre is inside the X domain
  if ( centre_x < min_v_x || centre_x > max_v_x){
    std::stringstream s;
    s << "The input value for centre (X="<< centre_x << ") is outside the limits of the instrument ["
      << min_v_x << ", "<< max_v_x << "]" ; 
    throw std::invalid_argument(s.str());
  }        
  

  // The Axis1 is defined by the spectra inside the workspace. Its limits and values are given by the 
  // ws->getAxis(1)

  // get the limits of the axis1 (Y)
  API::NumericAxis *oldAxis2 = dynamic_cast<API::NumericAxis*>(inputWS->getAxis(1) );
  // we cannot have the positions in Y direction without a NumericAxis  
  if( !oldAxis2 ) 
    throw std::invalid_argument("Vertical axis is not a numeric axis. If it is a spectra axis try running ConvertSpectrumAxis first.");
  double min_v_y = std::min(oldAxis2->getMin(), oldAxis2->getMax()); 
  double max_v_y = std::max(oldAxis2->getMin(), oldAxis2->getMax()); 
  g_log.notice() << "Limits Y = " << min_v_y << " " << max_v_y << std::endl;   
  // check centre is inside the Y domain
  if (centre_y < min_v_y || centre_y > max_v_y){
    std::stringstream s; 
    s << "The input value for centre (Y=" << centre_y << ") is outside the limits of the instrument ["
      << min_v_y << ", " << max_v_y << "]" ;
    throw std::invalid_argument(s.str()); 
  }
  g_log.notice() << "Centre: " << centre_x << "  " << centre_y << std::endl; 
  // check minradius is inside the limits of the region of the instrument
  if (centre_x - min_radius > max_v_x || centre_x + min_radius < min_v_x || centre_y - min_radius > max_v_y || centre_y + min_radius < min_v_y)
    throw std::invalid_argument("The minimun radius is outside the region of the instrument");

}


/**
 * The main method to calculate the ring profile for workspaces based on instruments. 
 * 
 * It will iterate over all the spectrum inside the workspace. 
 * For each spectrum, it will use the ::getBinForPixel method to identify 
 * where, in the output_bins, the sum of all the spectrum values should be placed in. 
 * 
 * @param inputWS: pointer to the input workspace
 * @param output_bins: the reference to the vector to be filled with the integration values
 */
void RingProfile::processInstrumentRingProfile(const API::MatrixWorkspace_sptr inputWS, 
                                               std::vector<double> & output_bins){

  for (int i= 0; i< (int) inputWS->getNumberHistograms(); i++){
    // for the detector based, the positions will be taken from the detector itself.
    try{
      Mantid::Geometry::IDetector_const_sptr det = inputWS->getDetector(i);
      
      // skip monitors  
      if (det->isMonitor()){
        continue;
      }
      
      // this part will be executed if the instrument is attached to the workspace
      
      // get the bin position
      int bin_n = getBinForPixel(det); 
        
      if (bin_n < 0) // -1 is the agreement for an invalid bin, or outside the ring being integrated
        continue;       

      g_log.debug() << "Bin for the index " << i << " = " << bin_n << " Pos = " << det->getPos() << std::endl; 

      // get the reference to the spectrum
      auto spectrum_pt = inputWS->getSpectrum(i); 
      const MantidVec & refY = spectrum_pt->dataY(); 
      // accumulate the values of this spectrum inside this bin
      for (size_t sp_ind = 0; sp_ind < inputWS->blocksize(); sp_ind ++)
        output_bins[bin_n] += refY[sp_ind];         

    }catch(Kernel::Exception::NotFoundError & ex){
      g_log.information() << "It found that detector for " << i << " is not valid. " << ex.what() << std::endl; 
      continue;
    }
    
  }
}

/**
 * Here is the main logic to perform the transformation, to calculate the bin position in degree for each detector.
 * 
 * The first part of the method is to check if the detector is inside the ring defined as minRadio and maxRadio. 
 * 
 * To do this, it checks the projected distance between the centre and the detector position. If this projected 
 * distance is outside the defined ring, it returns -1. 
 * 
 * For those detectors that lay inside the ring, it will calculate the phi angle. And than find the slot where
 * this angle should be placed (bin)
 * @param det: pointer to the detector from which the positions will be taken
 * @return bin position
 */
int RingProfile::getBinForPixel(Mantid::Geometry::IDetector_const_sptr det ){

    using Mantid::Kernel::V3D; 
    V3D origin(centre_x, centre_y, centre_z); 
    V3D diff_vector = det->getPos()-origin;
    double radio, theta, phi; 
    // get the spherical values of the vector from center to detector position
    diff_vector.getSpherical(radio, theta, phi);

    // the distance from the centre to the ring will be calculated as radio * sin(theta). 
    double effect_distance = radio * sin(theta*M_PI/180);

    //    g_log.debug() << "effect Distance = " << effect_distance << std::endl; 
    
    // check if it is inside the ring defined by min_radius, max_radius
    if ( effect_distance < min_radius || effect_distance > max_radius || effect_distance == 0)
      return -1; 

    // get the angle 
    // g_log.debug() << "The real angle is " << phi << std::endl; 
    return fromAngleToBin(phi); 
}

/**
 * The main method to calculate the ring profile for 2d image based workspace. 
 * 
 * It will iterate over all the spectrum inside the workspace. 
 * For each spectrum, it will use the ::getBinForPixel method to identify 
 * where, in the output_bins, the elements of the spectrum should be placed in. 
 * 
 * @param inputWS: pointer to the input workspace
 * @param output_bins: the reference to the vector to be filled with the integration values
 */
void RingProfile::processNumericImageRingProfile(const API::MatrixWorkspace_sptr inputWS, 
                                                 std::vector<double> & output_bins){
  // allocate the bin positions vector
  std::vector<int> bin_n(inputWS->dataY(0).size(), -1);

  // consider that each spectrum is a row in the image 
  for (int i= 0; i< (int) inputWS->getNumberHistograms(); i++){
    
    // get bin for the pixels inside this spectrum
    // for each column of the image
    getBinForPixel(inputWS, i, bin_n);
    
    // accumulate the values from the spectrum to the target bin
    // each column has it correspondend bin_position inside bin_n
    const MantidVec & refY = inputWS->dataY(i);
    for (size_t j = 0; j< bin_n.size(); j++){
      
      // is valid bin? No -> skip        
      if (bin_n[j] < 0)
        continue; 
      
      // accumulate the values of this spectrum inside this bin          
      output_bins[bin_n[j]] += refY[j];
      
    }
  }
       
}

/**
 * Here is the main logic to perform the transformation, to calculate the bin position in degree for each spectrum.
 * 
 * The first part of the method is to check if the pixel position is inside the ring defined as minRadio and maxRadio. 
 * 
 * To do this, it deducts the pixel position. This deduction follows the followin assumption: 
 * 
 *  - the spectrum_index == row number 
 *  - the position in the 'Y' direction is given by getAxis(1)[spectrum_index]
 *  - the position in the 'X' direction is the central point of the bin (dataX[column] + dataX[column+1])/2
 *
 * Having the position of the pixel, as defined above, if the distance is outside the ring defined by minRadio, maxRadio, 
 * it defines the bin position as -1.
 * 
 * If the pixel is inside the ring, it calculates the angle of the pixel and calls fromAngleToBin to define the bin 
 * position. 
 * @param ws: pointer to the workspace
 * @param spectrum_index: index of the spectrum
 * @param vector<int> bin positions (for each column inside the spectrum, the correspondent bin_pos)
 */
void RingProfile::getBinForPixel(const API::MatrixWorkspace_sptr ws, 
                                 int spectrum_index, std::vector<int> & bins_pos ){
  
  if (bins_pos.size() != ws->dataY(spectrum_index).size())
    throw std::runtime_error("Invalid bin positions vector"); 
  
  API::NumericAxis *oldAxis2 = dynamic_cast<API::NumericAxis*>(ws->getAxis(1));
  // assumption y position is the ws->getAxis(1)(spectrum_index)

  // calculate ypos, the difference of y - centre and  the square of this difference
  double ypos = (*oldAxis2)(spectrum_index);
  double diffy = ypos-centre_y;
  double diffy_quad = pow(diffy, 2.0);

  // the reference to X bins (the limits for each pixel in the horizontal direction)
  auto xvec = ws->dataX(spectrum_index);
  double xpos; 
  double diffx;
  double distance;
  double angle; 
  // g_log.debug() << "minR=" << min_radius << ", maxR=" << max_radius << ", start_angle = " << start_angle
  //               << ", num_bins" << num_bins << std::endl; 
  
  // for each pixel inside this row
  for (size_t i = 0; i< xvec.size()-1; i++){

    xpos = (xvec[i] + xvec[i+1])/2.0; // the x position is the centre of the bins boundaries
    diffx = xpos - centre_x; 
    // calculate the distance => norm of pixel position - centre
    distance = sqrt(pow(diffx, 2.0) + diffy_quad);
    
    //    g_log.debug() << "Distance found for v[" << i << "] = " << distance << "|("<< xpos << ", " 
    //               <<ypos << ")|" << std::endl; 
    
    // check if the distance is inside the ring
    if (distance < min_radius || distance > max_radius || distance == 0){
      bins_pos[i] = -1; 
      continue;
    }
    
    angle = atan2(diffy, diffx);

    // call fromAngleToBin (radians)
    bins_pos[i] = fromAngleToBin(angle, false); 

  } 
  
  }; 

  /* Return the bin position for a given angle.
   * 
   * The whole ring has 360 degree which is divided in NumBins bins. 
   * Hence, defining the bin_size as 360/NumBins, you have the dimension
   * of each bin. This means, that the bins will follow the following rule: 
   * 
   * Bins(n) = [startAngle + n * bin_size, startAngle + (n+1) * bin_size]
   * 
   * Having a given angle x, we need to find n so that: 
   * 
   *  startAngle + n bin_size < x < startAngle + (n+1) bin_size
   *  n bin_size < x-startAngle < (n+1) bin_size
   *  n < (x-startAngle)/bin_size < n+1
   *  
   * So, n = truncate (x-startAngle)/bin_size
   * 
   * @param angle: the input angle
   * @param degree: flag that indicates that the input angle is in degree (default = true)
   * @return n
   */
int RingProfile::fromAngleToBin(double angle, bool degree){
    if (!degree)
      angle *= (180/M_PI);
    
    angle -= start_angle;

    if (angle<0){
      while(angle<0)
        angle+= 360; 
    }
    
    angle /= bin_size; 
    return (int)angle;
}; 



 
} // namespace Algorithms
} // namespace Mantid
