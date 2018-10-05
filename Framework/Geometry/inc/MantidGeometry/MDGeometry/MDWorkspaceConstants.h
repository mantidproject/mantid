// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef H_MD_WORKSPACE_CONSTANTS
#define H_MD_WORKSPACE_CONSTANTS
//
#include <string>

/** The is the collection of the constants and enums, which are used in
Multidimensional Workspace, usually in more then one file
*
*  Collected here to simplify references and modifications, the dimensionsID
will be replaced by proper class

    @author Alex Buts, RAL ISIS
    @date 01/10/2010
*/

/** We define maximal number of dimensionsMAX_MD_DIMS_POSSIBLE  which a MD
   dataset can have because
   \li a) every additional dimension will be expensive to process
   \li b) minimal size of the visualisation dataset is 2^MAX_NDIMS_POSSIBLE, so
   this number has to be reasonable and any bigger number is probably because of
   an  error;
   \li c) if we need to increase the number of dimensions, it can be easy done
   here
*/
#define MAX_MD_DIMS_POSSIBLE 11

/// we are not going to rebin data on more than some number of bins. Lets set
/// this constant as the limit for checks
/// (in case of wrong word padding or -int casted to unsigned int)
#define MAX_REASONABLE_BIN_NUMBER 1000000
//***********************************************************************************************************************************

#endif
