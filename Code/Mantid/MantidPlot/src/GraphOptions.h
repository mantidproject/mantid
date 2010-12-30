#ifndef MANTIDGRAPHOPTIONS_H_
#define MANTIDGRAPHOPTIONS_H_

/**
 * This file contains declarations of options such as scale types that are 
 * shared between the colormaps and instrument window
 */
namespace GraphOptions
{

/**
 * Scale type enumeration
 */
  enum ScaleType { Linear = 0, Log10 };

  /**
   * Axis choice
   */
  enum Axis { Left, Right, Bottom, Top };
}

#endif //MANTIDSCALETYPE_H_
