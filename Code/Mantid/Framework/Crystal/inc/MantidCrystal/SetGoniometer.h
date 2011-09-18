#ifndef MANTID_CRYSTAL_SETGONIOMETER_H_
#define MANTID_CRYSTAL_SETGONIOMETER_H_
/*WIKI* 

Use this algorithm to define your goniometer. Enter each axis in the order of rotation, starting with the one closest to the sample. 

You may enter up to 6 axes, for which you must define (separated by commas): 
* The name of the axis, which MUST match the name in your sample logs.
* The X, Y, Z components of the vector of the axis of rotation. Right-handed coordinates with +Z=beam direction; +Y=Vertically up (against gravity); +X to the left.
* The sense of rotation as 1 or -1: 1 for counter-clockwise, -1 for clockwise rotation.

The run's sample logs will be used in order to determine the actual angles of rotation: for example, if you have an axis called 'phi', then the first value of the log called 'phi' will be used as the rotation angle. Units are assumed to be degrees.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** Define the goniometer used in an experiment by giving the axes and directions of rotations.
   * 
   * @author Janik Zikovsky
   * @date 2011-05-27
   */
  class DLLExport SetGoniometer  : public API::Algorithm
  {
  public:
    SetGoniometer();
    ~SetGoniometer();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SetGoniometer";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_SETGONIOMETER_H_ */
