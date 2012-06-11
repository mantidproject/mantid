#include "MantidMDEvents/MDTransfDEHelper.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Exception.h"


namespace Mantid
{
namespace MDEvents
{

MDTransfDEHelper::MDTransfDEHelper():
EmodesList(ConvertToMD::No_DE,"")
{
   EmodesList[ConvertToMD::Elastic]="Elastic";
   EmodesList[ConvertToMD::Direct] ="Direct";
   EmodesList[ConvertToMD::Indir]  ="Indirect";

}

ConvertToMD::EModes MDTransfDEHelper::getEmode(const std::string &Mode)const
{
    int nMode = Kernel::Strings::isMember(EmodesList,Mode);
    if(nMode<0){
        std::string ERR= "MDTransfDEHelper::getEmode: Unknown energy conversion mode "+ Mode+" requested\n";
        throw(std::invalid_argument(ERR));
    }
    return ConvertToMD::EModes(nMode);
}
/**return string representation of correspondend enum mode */
std::string MDTransfDEHelper::getEmode(ConvertToMD::EModes Mode)const
{
    if(Mode>ConvertToMD::No_DE) throw(std::invalid_argument(" Can not convert into string undefined emode"));
  
    return EmodesList[Mode];
}

} // endnamespace MDEvents
} // endnamespace Mantid
