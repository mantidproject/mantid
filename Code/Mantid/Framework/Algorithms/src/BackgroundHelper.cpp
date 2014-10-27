#include "MantidAlgorithms/BackgroundHelper.h"


namespace Mantid
{
namespace Algorithms
{


void BackgroundHelper::initialize(const API::MatrixWorkspace_const_sptr &bkgWS,const API::MatrixWorkspace_sptr &sourceWS,int emode)
{
}

void BackgroundHelper::removeBackground(int hist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data)
{
}

} // end API
} // end Mantid
