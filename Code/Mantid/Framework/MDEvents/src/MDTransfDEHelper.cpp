#include "MantidMDEvents/MDTransfDEHelper.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Exception.h"


namespace Mantid
{
namespace MDEvents
{

MDTransfDEHelper::MDTransfDEHelper():
EmodesList(CnvrtToMD::No_DE,"")
{
   EmodesList[CnvrtToMD::Elastic]="Elastic";
   EmodesList[CnvrtToMD::Direct] ="Direct";
   EmodesList[CnvrtToMD::Indir]  ="Indirect";

}

CnvrtToMD::EModes MDTransfDEHelper::getEmode(const std::string &Mode)const
{
    int nMode = Kernel::Strings::isMember(EmodesList,Mode);
    if(nMode<0){
        std::string ERR= "MDTransfDEHelper::getEmode: Unknown energy conversion mode "+ Mode+" requested\n";
        throw(std::invalid_argument(ERR));
    }
    return CnvrtToMD::EModes(nMode);
}
/**return string representation of correspondend enum mode */
std::string MDTransfDEHelper::getEmode(CnvrtToMD::EModes Mode)const
{
    if(Mode>CnvrtToMD::No_DE) throw(std::invalid_argument(" Can not convert into string undefined emode"));
  
    return EmodesList[Mode];
}

} // endnamespace MDEvents
} // endnamespace Mantid
