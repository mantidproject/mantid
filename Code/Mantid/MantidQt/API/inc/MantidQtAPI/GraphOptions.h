#ifndef MANTIDGRAPHOPTIONS_H_
#define MANTIDGRAPHOPTIONS_H_
#include "DllOption.h"

/**
 * This file contains declarations of options such as scale types that are 
 * shared between the colormaps and instrument window
 */
namespace GraphOptions
{

/**
 * Scale type enumeration
 */
  EXPORT_OPT_MANTIDQT_API enum ScaleType { Linear = 0, Log10, Power };

  /**
   * Axis choice
   */
  EXPORT_OPT_MANTIDQT_API enum Axis { Left, Right, Bottom, Top };
}

#endif //MANTIDSCALETYPE_H_
