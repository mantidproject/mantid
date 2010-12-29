#include <algorithm>
#include <iostream> // REMOVE
#include <limits>
#include <sstream>
#include <stdexcept>
#include "MantidKernel/Atom.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace PhysicalConstants
{

using std::string;

/// constant to use for garbage numbers
static const double NAN = std::numeric_limits<double>::quiet_NaN();

const NeutronAtom getNeutronNoExceptions(const uint16_t z, const uint16_t a)
{
  try {
    return getNeutronAtom(z, a);
  } catch (std::runtime_error& e) {
    return NeutronAtom(z, a,
		       NAN, NAN, NAN, NAN,
		       NAN, NAN, NAN, NAN); // set to junk value
  }
}

Atom::Atom(const string& symbol, const uint16_t z, const uint16_t a,
           const double abundance, const double mass, const double density) :
           symbol(symbol), z_number(z), a_number(a), abundance(abundance),
           mass(mass), mass_density(density),
	   number_density(density * N_A / mass),
	   neutron(getNeutronNoExceptions(z, a))
{
}

/// Copy constructor.
Atom::Atom(const Atom& other):
    symbol(other.symbol), z_number(other.z_number), a_number(other.a_number),
    abundance(other.abundance), mass(other.mass), mass_density(other.mass_density),
    number_density(other.number_density), neutron(other.neutron)
  {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// ---------- START DO NOT EDIT AREA----------
static const Atom H("H", 1, 0, 0.000000, 1.007940, 0.0708);
static const Atom H1("H", 1, 1, 99.988500, 1.007825, 0.070791924393);
static const Atom H2("H", 1, 2, 0.011500, 2.014102, 0.141475093639);
static const Atom H3("H", 1, 3, 0.000000, 3.016049, 0.21185416606);
static const Atom H4("H", 1, 4, 0.000000, 4.027830, 0.282923947854);
static const Atom H5("H", 1, 5, 0.000000, 5.039540, 0.353988761236);
static const Atom H6("H", 1, 6, 0.000000, 6.044940, 0.424610345854);
static const Atom He("He", 2, 0, 0.000000, 4.002602, 0.122);
static const Atom He3("He", 2, 3, 0.000137, 3.016029, 0.0919290940702);
static const Atom He4("He", 2, 4, 99.999863, 4.002603, 0.122000038091);
static const Atom He5("He", 2, 5, 0.000000, 5.012220, 0.152773330948);
static const Atom He6("He", 2, 6, 0.000000, 6.018888, 0.183456748435);
static const Atom He7("He", 2, 7, 0.000000, 7.028030, 0.214215567773);
static const Atom He8("He", 2, 8, 0.000000, 8.033922, 0.244875329598);
static const Atom He9("He", 2, 9, 0.000000, 9.043820, 0.275657194995);
static const Atom He10("He", 2, 10, 0.000000, 10.052400, 0.306398887524);
static const Atom Li("Li", 3, 0, 0.000000, 6.941000, 0.534);
static const Atom Li4("Li", 3, 4, 0.000000, 4.027180, 0.309827707823);
static const Atom Li5("Li", 3, 5, 0.000000, 5.012540, 0.385635551073);
static const Atom Li6("Li", 3, 6, 7.590000, 6.015122, 0.462768377496);
static const Atom Li7("Li", 3, 7, 92.410000, 7.016004, 0.539770369687);
static const Atom Li8("Li", 3, 8, 0.000000, 8.022487, 0.617203270105);
static const Atom Li9("Li", 3, 9, 0.000000, 9.026789, 0.694468430975);
static const Atom Li10("Li", 3, 10, 0.000000, 10.035481, 0.772071294338);
static const Atom Li11("Li", 3, 11, 0.000000, 11.043796, 0.849645161216);
static const Atom Li12("Li", 3, 12, 0.000000, 12.053780, 0.927347431206);
static const Atom Be("Be", 4, 0, 0.000000, 9.012182, 1.848);
static const Atom Be5("Be", 4, 5, 0.000000, 5.040790, 1.03364311995);
static const Atom Be6("Be", 4, 6, 0.000000, 6.019726, 1.23437960396);
static const Atom Be7("Be", 4, 7, 0.000000, 7.016929, 1.4388618829);
static const Atom Be8("Be", 4, 8, 0.000000, 8.005305, 1.64153407092);
static const Atom Be9("Be", 4, 9, 100.000000, 9.012182, 1.84800002051);
static const Atom Be10("Be", 4, 10, 0.000000, 10.013534, 2.05333295284);
static const Atom Be11("Be", 4, 11, 0.000000, 11.021658, 2.26005466645);
static const Atom Be12("Be", 4, 12, 0.000000, 12.026921, 2.46618965396);
static const Atom Be13("Be", 4, 13, 0.000000, 13.036130, 2.67313379157);
static const Atom Be14("Be", 4, 14, 0.000000, 14.042820, 2.87956139368);
static const Atom B("B", 5, 0, 0.000000, 10.811000, 2.34);
static const Atom B7("B", 5, 7, 0.000000, 7.029920, 1.52159955601);
static const Atom B8("B", 5, 8, 0.000000, 8.024607, 1.73689572454);
static const Atom B9("B", 5, 9, 0.000000, 9.013329, 1.95090087799);
static const Atom B10("B", 5, 10, 19.900000, 10.012937, 2.16726228656);
static const Atom B11("B", 5, 11, 80.100000, 11.009305, 2.38292247433);
static const Atom B12("B", 5, 12, 0.000000, 12.014352, 2.60046100398);
static const Atom B13("B", 5, 13, 0.000000, 13.017780, 2.81764923707);
static const Atom B14("B", 5, 14, 0.000000, 14.025404, 3.03574557025);
static const Atom B15("B", 5, 15, 0.000000, 15.031097, 3.25342401073);
static const Atom B16("B", 5, 16, 0.000000, 16.039810, 3.47175611877);
static const Atom B17("B", 5, 17, 0.000000, 17.046930, 3.68974342799);
static const Atom B18("B", 5, 18, 0.000000, 18.056170, 3.90818960318);
static const Atom B19("B", 5, 19, 0.000000, 19.063730, 4.12627214874);
static const Atom C("C", 6, 0, 0.000000, 12.010700, 2.1);
static const Atom C8("C", 6, 8, 0.000000, 8.037675, 1.40534003014);
static const Atom C9("C", 6, 9, 0.000000, 9.031040, 1.57902405438);
static const Atom C10("C", 6, 10, 0.000000, 10.016853, 1.75138763852);
static const Atom C11("C", 6, 11, 0.000000, 11.011434, 1.92528420325);
static const Atom C12("C", 6, 12, 98.930000, 12.000000, 2.09812916816);
static const Atom C13("C", 6, 13, 1.070000, 13.003355, 2.27355983909);
static const Atom C14("C", 6, 14, 0.000000, 14.003242, 2.44838420532);
static const Atom C15("C", 6, 15, 0.000000, 15.010599, 2.62451468524);
static const Atom C16("C", 6, 16, 0.000000, 16.014701, 2.80007594062);
static const Atom C17("C", 6, 17, 0.000000, 17.022584, 2.97629833399);
static const Atom C18("C", 6, 18, 0.000000, 18.026760, 3.15187258028);
static const Atom C19("C", 6, 19, 0.000000, 19.035250, 3.32820110402);
static const Atom C20("C", 6, 20, 0.000000, 20.040320, 3.50393166094);
static const Atom C21("C", 6, 21, 0.000000, 21.049340, 3.68035285204);
static const Atom C22("C", 6, 22, 0.000000, 22.056450, 3.85644009092);
static const Atom N("N", 7, 0, 0.000000, 14.006700, 0.808);
static const Atom N10("N", 7, 10, 0.000000, 10.042620, 0.57932539142);
static const Atom N11("N", 7, 11, 0.000000, 11.026800, 0.636099466684);
static const Atom N12("N", 7, 12, 0.000000, 12.018613, 0.693313875902);
static const Atom N13("N", 7, 13, 0.000000, 13.005739, 0.750257860355);
static const Atom N14("N", 7, 14, 99.632000, 14.003074, 0.807790828404);
static const Atom N15("N", 7, 15, 0.368000, 15.000109, 0.865306459759);
static const Atom N16("N", 7, 16, 0.000000, 16.006101, 0.923338825791);
static const Atom N17("N", 7, 17, 0.000000, 17.008450, 0.981160987242);
static const Atom N18("N", 7, 18, 0.000000, 18.014082, 1.03917255713);
static const Atom N19("N", 7, 19, 0.000000, 19.017027, 1.09702912292);
static const Atom N20("N", 7, 20, 0.000000, 20.023370, 1.15508170804);
static const Atom N21("N", 7, 21, 0.000000, 21.027090, 1.212982981);
static const Atom N22("N", 7, 22, 0.000000, 22.034440, 1.27109365661);
static const Atom N23("N", 7, 23, 0.000000, 23.040510, 1.32913049326);
static const Atom N24("N", 7, 24, 0.000000, 24.050500, 1.3873934617);
static const Atom O("O", 8, 0, 0.000000, 15.999400, 1.14);
static const Atom O12("O", 8, 12, 0.000000, 12.034405, 0.857483511882);
static const Atom O13("O", 8, 13, 0.000000, 13.024810, 0.928052514469);
static const Atom O14("O", 8, 14, 0.000000, 14.008595, 0.998149845032);
static const Atom O15("O", 8, 15, 0.000000, 15.003065, 1.06900849757);
static const Atom O16("O", 8, 16, 99.757000, 15.994915, 1.13968040484);
static const Atom O17("O", 8, 17, 0.038000, 16.999132, 1.21123354063);
static const Atom O18("O", 8, 18, 0.205000, 17.999160, 1.28248827181);
static const Atom O19("O", 8, 19, 0.000000, 19.003579, 1.35405578084);
static const Atom O20("O", 8, 20, 0.000000, 20.004076, 1.42534387965);
static const Atom O21("O", 8, 21, 0.000000, 21.008655, 1.49692280336);
static const Atom O22("O", 8, 22, 0.000000, 22.009970, 1.56826917259);
static const Atom O23("O", 8, 23, 0.000000, 23.015690, 1.63992940985);
static const Atom O24("O", 8, 24, 0.000000, 24.020370, 1.71151554433);
static const Atom O25("O", 8, 25, 0.000000, 25.029140, 1.78339310224);
static const Atom O26("O", 8, 26, 0.000000, 26.037750, 1.85525925972);
static const Atom F("F", 9, 0, 0.000000, 18.998403, 1.5);
static const Atom F14("F", 9, 14, 0.000000, 14.036080, 1.1082047148);
static const Atom F15("F", 9, 15, 0.000000, 15.018010, 1.18573201984);
static const Atom F16("F", 9, 16, 0.000000, 16.011466, 1.26416934872);
static const Atom F17("F", 9, 17, 0.000000, 17.002095, 1.34238349358);
static const Atom F18("F", 9, 18, 0.000000, 18.000938, 1.42124610504);
static const Atom F19("F", 9, 19, 100.000000, 18.998403, 1.5);
static const Atom F20("F", 9, 20, 0.000000, 19.999981, 1.57907860277);
static const Atom F21("F", 9, 21, 0.000000, 20.999949, 1.65803004697);
static const Atom F22("F", 9, 22, 0.000000, 22.002999, 1.73722486846);
static const Atom F23("F", 9, 23, 0.000000, 23.003570, 1.81622395507);
static const Atom F24("F", 9, 24, 0.000000, 24.008100, 1.89553562059);
static const Atom F25("F", 9, 25, 0.000000, 25.012090, 1.97480465095);
static const Atom F26("F", 9, 26, 0.000000, 26.019630, 2.05435396802);
static const Atom F27("F", 9, 27, 0.000000, 27.026890, 2.13388117797);
static const Atom F28("F", 9, 28, 0.000000, 28.035670, 2.21352839801);
static const Atom F29("F", 9, 29, 0.000000, 29.043260, 2.29308166278);
static const Atom Ne("Ne", 10, 0, 0.000000, 20.179700, 1.207);
static const Atom Ne32("Ne", 10, 32, 0.000000, 32.039910, 1.91638980609);
static const Atom Ne16("Ne", 10, 16, 0.000000, 16.025757, 0.958541935658);
static const Atom Ne17("Ne", 10, 17, 0.000000, 17.017700, 1.0178726096);
static const Atom Ne18("Ne", 10, 18, 0.000000, 18.005697, 1.07696726907);
static const Atom Ne19("Ne", 10, 19, 0.000000, 19.001880, 1.13655153043);
static const Atom Ne20("Ne", 10, 20, 90.480000, 19.992440, 1.19579950605);
static const Atom Ne21("Ne", 10, 21, 0.270000, 20.993847, 1.25569622022);
static const Atom Ne22("Ne", 10, 22, 9.250000, 21.991386, 1.31536159163);
static const Atom Ne23("Ne", 10, 23, 0.000000, 22.994467, 1.37535850778);
static const Atom Ne24("Ne", 10, 24, 0.000000, 23.993615, 1.43512011105);
static const Atom Ne25("Ne", 10, 25, 0.000000, 24.997790, 1.49518241252);
static const Atom Ne26("Ne", 10, 26, 0.000000, 26.000460, 1.55515469606);
static const Atom Ne27("Ne", 10, 27, 0.000000, 27.007620, 1.61539553809);
static const Atom Ne28("Ne", 10, 28, 0.000000, 28.012110, 1.67547668053);
static const Atom Ne29("Ne", 10, 29, 0.000000, 29.019350, 1.73572230757);
static const Atom Ne30("Ne", 10, 30, 0.000000, 30.023870, 1.79580524438);
static const Atom Ne31("Ne", 10, 31, 0.000000, 31.033110, 1.85617049659);
static const Atom Na("Na", 11, 0, 0.000000, 22.989770, 0.971);
static const Atom Na32("Na", 11, 32, 0.000000, 32.019650, 1.35238761197);
static const Atom Na33("Na", 11, 33, 0.000000, 33.027390, 1.3949506972);
static const Atom Na34("Na", 11, 34, 0.000000, 34.034900, 1.43750406811);
static const Atom Na35("Na", 11, 35, 0.000000, 35.044180, 1.48013219706);
static const Atom Na18("Na", 11, 18, 0.000000, 18.027180, 0.761399169283);
static const Atom Na19("Na", 11, 19, 0.000000, 19.013879, 0.803073563111);
static const Atom Na20("Na", 11, 20, 0.000000, 20.007348, 0.845033895859);
static const Atom Na21("Na", 11, 21, 0.000000, 20.997655, 0.886860682038);
static const Atom Na22("Na", 11, 22, 0.000000, 21.994437, 0.92896093057);
static const Atom Na23("Na", 11, 23, 100.000000, 22.989770, 0.970999986062);
static const Atom Na24("Na", 11, 24, 0.000000, 23.990963, 1.01328657892);
static const Atom Na25("Na", 11, 25, 0.000000, 24.989954, 1.05548014279);
static const Atom Na26("Na", 11, 26, 0.000000, 25.992590, 1.09782763768);
static const Atom Na27("Na", 11, 27, 0.000000, 26.994010, 1.14012379028);
static const Atom Na28("Na", 11, 28, 0.000000, 27.998890, 1.18256608004);
static const Atom Na29("Na", 11, 29, 0.000000, 29.002810, 1.22496782308);
static const Atom Na30("Na", 11, 30, 0.000000, 30.009230, 1.26747515656);
static const Atom Na31("Na", 11, 31, 0.000000, 31.013600, 1.30989590587);
static const Atom Mg("Mg", 12, 0, 0.000000, 24.305000, 1.738);
static const Atom Mg32("Mg", 12, 32, 0.000000, 31.999150, 2.28819266406);
static const Atom Mg33("Mg", 12, 33, 0.000000, 33.005590, 2.36016109525);
static const Atom Mg34("Mg", 12, 34, 0.000000, 34.009070, 2.43191786299);
static const Atom Mg35("Mg", 12, 35, 0.000000, 35.017490, 2.50402787986);
static const Atom Mg36("Mg", 12, 36, 0.000000, 36.022450, 2.57589047933);
static const Atom Mg37("Mg", 12, 37, 0.000000, 37.031240, 2.64802695412);
static const Atom Mg20("Mg", 12, 20, 0.000000, 20.018863, 1.43150725752);
static const Atom Mg21("Mg", 12, 21, 0.000000, 21.011714, 1.50250396758);
static const Atom Mg22("Mg", 12, 22, 0.000000, 21.999574, 1.57314378876);
static const Atom Mg23("Mg", 12, 23, 0.000000, 22.994125, 1.64426204798);
static const Atom Mg24("Mg", 12, 24, 78.990000, 23.985042, 1.71512046172);
static const Atom Mg25("Mg", 12, 25, 10.000000, 24.985837, 1.78668523928);
static const Atom Mg26("Mg", 12, 26, 11.010000, 25.982593, 1.8579611892);
static const Atom Mg27("Mg", 12, 27, 0.000000, 26.984341, 1.92959408377);
static const Atom Mg28("Mg", 12, 28, 0.000000, 27.983877, 2.00106882142);
static const Atom Mg29("Mg", 12, 29, 0.000000, 28.988550, 2.07291091956);
static const Atom Mg30("Mg", 12, 30, 0.000000, 29.990460, 2.14455541987);
static const Atom Mg31("Mg", 12, 31, 0.000000, 30.996550, 2.21649882329);
static const Atom Al("Al", 13, 0, 0.000000, 26.981538, 2.6989);
static const Atom Al32("Al", 13, 32, 0.000000, 31.988120, 3.19969666177);
static const Atom Al33("Al", 13, 33, 0.000000, 32.990870, 3.29999939377);
static const Atom Al34("Al", 13, 34, 0.000000, 33.996930, 3.40063321731);
static const Atom Al35("Al", 13, 35, 0.000000, 34.999940, 3.50096195651);
static const Atom Al36("Al", 13, 36, 0.000000, 36.006350, 3.60163078973);
static const Atom Al37("Al", 13, 37, 0.000000, 37.010310, 3.70205455519);
static const Atom Al38("Al", 13, 38, 0.000000, 38.016900, 3.80274139339);
static const Atom Al39("Al", 13, 39, 0.000000, 39.021900, 3.90326918762);
static const Atom Al21("Al", 13, 21, 0.000000, 21.028040, 2.10338555037);
static const Atom Al22("Al", 13, 22, 0.000000, 22.019520, 2.20256097069);
static const Atom Al23("Al", 13, 23, 0.000000, 23.007265, 2.30136278771);
static const Atom Al24("Al", 13, 24, 0.000000, 23.999941, 2.40065784111);
static const Atom Al25("Al", 13, 25, 0.000000, 24.990429, 2.49973399398);
static const Atom Al26("Al", 13, 26, 0.000000, 25.986892, 2.59940785811);
static const Atom Al27("Al", 13, 27, 100.000000, 26.981538, 2.69890004401);
static const Atom Al28("Al", 13, 28, 0.000000, 27.981910, 2.79896488424);
static const Atom Al29("Al", 13, 29, 0.000000, 28.980445, 2.89884596166);
static const Atom Al30("Al", 13, 30, 0.000000, 29.982960, 2.99912520717);
static const Atom Al31("Al", 13, 31, 0.000000, 30.983946, 3.09925149039);
static const Atom Si("Si", 14, 0, 0.000000, 28.085500, 2.33);
static const Atom Si32("Si", 14, 32, 0.000000, 31.974148, 2.65260597365);
static const Atom Si33("Si", 14, 33, 0.000000, 32.978001, 2.73588657243);
static const Atom Si34("Si", 14, 34, 0.000000, 33.978576, 2.81889523348);
static const Atom Si35("Si", 14, 35, 0.000000, 34.984580, 2.90235428958);
static const Atom Si36("Si", 14, 36, 0.000000, 35.986690, 2.9854902957);
static const Atom Si37("Si", 14, 37, 0.000000, 36.993000, 3.06897473785);
static const Atom Si38("Si", 14, 38, 0.000000, 37.995980, 3.15218292001);
static const Atom Si39("Si", 14, 39, 0.000000, 39.002300, 3.23566819177);
static const Atom Si40("Si", 14, 40, 0.000000, 40.005800, 3.31891951363);
static const Atom Si41("Si", 14, 41, 0.000000, 41.012700, 3.40245290274);
static const Atom Si42("Si", 14, 42, 0.000000, 42.016100, 3.4856959285);
static const Atom Si22("Si", 14, 22, 0.000000, 22.034530, 1.82800572893);
static const Atom Si23("Si", 14, 23, 0.000000, 23.025520, 1.9102192092);
static const Atom Si24("Si", 14, 24, 0.000000, 24.011546, 1.99202087127);
static const Atom Si25("Si", 14, 25, 0.000000, 25.004107, 2.0743646832);
static const Atom Si26("Si", 14, 26, 0.000000, 25.992330, 2.15634861049);
static const Atom Si27("Si", 14, 27, 0.000000, 26.986705, 2.23884289369);
static const Atom Si28("Si", 14, 28, 92.229700, 27.976927, 2.32099264108);
static const Atom Si29("Si", 14, 29, 4.683200, 28.976495, 2.40391777599);
static const Atom Si30("Si", 14, 30, 3.087200, 29.973770, 2.48665270736);
static const Atom Si31("Si", 14, 31, 0.000000, 30.975363, 2.56974582682);
static const Atom P("P", 15, 0, 0.000000, 30.973761, 1.82);
static const Atom P24("P", 15, 24, 0.000000, 24.034350, 1.41224428638);
static const Atom P25("P", 15, 25, 0.000000, 25.020260, 1.47017577878);
static const Atom P26("P", 15, 26, 0.000000, 26.011780, 1.52843691149);
static const Atom P27("P", 15, 27, 0.000000, 26.999190, 1.58645654301);
static const Atom P28("P", 15, 28, 0.000000, 27.992312, 1.64481180829);
static const Atom P29("P", 15, 29, 0.000000, 28.981801, 1.70295362413);
static const Atom P30("P", 15, 30, 0.000000, 29.978314, 1.76150810733);
static const Atom P31("P", 15, 31, 100.000000, 30.973762, 1.82000002997);
static const Atom P32("P", 15, 32, 0.000000, 31.973907, 1.8787680008);
static const Atom P33("P", 15, 33, 0.000000, 32.971725, 1.93739920851);
static const Atom P34("P", 15, 34, 0.000000, 33.973636, 1.99627089264);
static const Atom P35("P", 15, 35, 0.000000, 34.973314, 2.05501139639);
static const Atom P36("P", 15, 36, 0.000000, 35.978260, 2.11406142121);
static const Atom P37("P", 15, 37, 0.000000, 36.979610, 2.17290015894);
static const Atom P38("P", 15, 38, 0.000000, 37.984470, 2.23194514221);
static const Atom P39("P", 15, 39, 0.000000, 38.986420, 2.29081913559);
static const Atom P40("P", 15, 40, 0.000000, 39.991050, 2.34985060419);
static const Atom P41("P", 15, 41, 0.000000, 40.994800, 2.40883036451);
static const Atom P42("P", 15, 42, 0.000000, 42.000090, 2.46790061433);
static const Atom P43("P", 15, 43, 0.000000, 43.003310, 2.52684923216);
static const Atom P44("P", 15, 44, 0.000000, 44.009880, 2.58599469403);
static const Atom P45("P", 15, 45, 0.000000, 45.015140, 2.64506318106);
static const Atom P46("P", 15, 46, 0.000000, 46.023830, 2.70433321288);
static const Atom S("S", 16, 0, 0.000000, 32.065000, 2.07);
static const Atom S26("S", 16, 26, 0.000000, 26.027880, 1.68026544831);
static const Atom S27("S", 16, 27, 0.000000, 27.018800, 1.74423564634);
static const Atom S28("S", 16, 28, 0.000000, 28.004370, 1.8078604678);
static const Atom S29("S", 16, 29, 0.000000, 28.996610, 1.87191588024);
static const Atom S30("S", 16, 30, 0.000000, 29.984903, 1.93571648869);
static const Atom S31("S", 16, 31, 0.000000, 30.979554, 1.99992757237);
static const Atom S32("S", 16, 32, 94.930000, 31.972071, 2.06400082109);
static const Atom S33("S", 16, 33, 0.760000, 32.971458, 2.1285176702);
static const Atom S34("S", 16, 34, 4.290000, 33.967867, 2.1928421749);
static const Atom S35("S", 16, 35, 0.000000, 34.969032, 2.25747377295);
static const Atom S36("S", 16, 36, 0.020000, 35.967081, 2.32190417657);
static const Atom S37("S", 16, 37, 0.000000, 36.971126, 2.38672166663);
static const Atom S38("S", 16, 38, 0.000000, 37.971163, 2.45128044316);
static const Atom S39("S", 16, 39, 0.000000, 38.975140, 2.51609355372);
static const Atom S40("S", 16, 40, 0.000000, 39.975470, 2.58067122719);
static const Atom S41("S", 16, 41, 0.000000, 40.980030, 2.64552197412);
static const Atom S42("S", 16, 42, 0.000000, 41.981490, 2.71017259629);
static const Atom S43("S", 16, 43, 0.000000, 42.986600, 2.77505884921);
static const Atom S44("S", 16, 44, 0.000000, 43.988320, 2.83972625604);
static const Atom S45("S", 16, 45, 0.000000, 44.994820, 2.90470224232);
static const Atom S46("S", 16, 46, 0.000000, 45.999570, 2.96956525495);
static const Atom S47("S", 16, 47, 0.000000, 47.007620, 3.0346413036);
static const Atom S48("S", 16, 48, 0.000000, 48.012990, 3.09954434118);
static const Atom S49("S", 16, 49, 0.000000, 49.022010, 3.16468300951);
static const Atom Cl("Cl", 17, 0, 0.000000, 35.453000, 1.56);
static const Atom Cl28("Cl", 17, 28, 0.000000, 28.028510, 1.23330819959);
static const Atom Cl29("Cl", 17, 29, 0.000000, 29.014110, 1.27667649);
static const Atom Cl30("Cl", 17, 30, 0.000000, 30.004770, 1.32026743012);
static const Atom Cl31("Cl", 17, 31, 0.000000, 30.992420, 1.36372592446);
static const Atom Cl32("Cl", 17, 32, 0.000000, 31.985689, 1.40743166559);
static const Atom Cl33("Cl", 17, 33, 0.000000, 32.977452, 1.45107113102);
static const Atom Cl34("Cl", 17, 34, 0.000000, 33.973762, 1.49491068945);
static const Atom Cl35("Cl", 17, 35, 75.780000, 34.968853, 1.53869659063);
static const Atom Cl36("Cl", 17, 36, 0.000000, 35.968307, 1.58267449418);
static const Atom Cl37("Cl", 17, 37, 24.220000, 36.965903, 1.6265706162);
static const Atom Cl38("Cl", 17, 38, 0.000000, 37.968011, 1.67066528807);
static const Atom Cl39("Cl", 17, 39, 0.000000, 38.968008, 1.7146670807);
static const Atom Cl40("Cl", 17, 40, 0.000000, 39.970420, 1.75877514456);
static const Atom Cl41("Cl", 17, 41, 0.000000, 40.970650, 1.80278718303);
static const Atom Cl42("Cl", 17, 42, 0.000000, 41.973170, 1.8468999859);
static const Atom Cl43("Cl", 17, 43, 0.000000, 42.974200, 1.8909472259);
static const Atom Cl44("Cl", 17, 44, 0.000000, 43.978540, 1.93514011226);
static const Atom Cl45("Cl", 17, 45, 0.000000, 44.979700, 1.97919307252);
static const Atom Cl46("Cl", 17, 46, 0.000000, 45.984120, 2.02338947903);
static const Atom Cl47("Cl", 17, 47, 0.000000, 46.987950, 2.06755992441);
static const Atom Cl48("Cl", 17, 48, 0.000000, 47.994850, 2.11186545567);
static const Atom Cl49("Cl", 17, 49, 0.000000, 48.999890, 2.15608914337);
static const Atom Cl50("Cl", 17, 50, 0.000000, 50.007730, 2.20043603644);
static const Atom Cl51("Cl", 17, 51, 0.000000, 51.013530, 2.2446931656);
static const Atom Ar("Ar", 18, 0, 0.000000, 39.948000, 1.4);
static const Atom Ar30("Ar", 18, 30, 0.000000, 30.021560, 1.05212235907);
static const Atom Ar31("Ar", 18, 31, 0.000000, 31.012130, 1.08683743867);
static const Atom Ar32("Ar", 18, 32, 0.000000, 31.997660, 1.12137588866);
static const Atom Ar33("Ar", 18, 33, 0.000000, 32.989930, 1.15615054571);
static const Atom Ar34("Ar", 18, 34, 0.000000, 33.980270, 1.19085756483);
static const Atom Ar35("Ar", 18, 35, 0.000000, 34.975257, 1.22572743016);
static const Atom Ar36("Ar", 18, 36, 0.336500, 35.967546, 1.26050277341);
static const Atom Ar37("Ar", 18, 37, 0.000000, 36.966776, 1.29552133423);
static const Atom Ar38("Ar", 18, 38, 0.063200, 37.962732, 1.33042517973);
static const Atom Ar39("Ar", 18, 39, 0.000000, 38.964313, 1.36552613898);
static const Atom Ar40("Ar", 18, 40, 99.600300, 39.962383, 1.40050406459);
static const Atom Ar41("Ar", 18, 41, 0.000000, 40.964501, 1.43562383899);
static const Atom Ar42("Ar", 18, 42, 0.000000, 41.963050, 1.47061855412);
static const Atom Ar43("Ar", 18, 43, 0.000000, 42.965670, 1.50575593271);
static const Atom Ar44("Ar", 18, 44, 0.000000, 43.965365, 1.54079080304);
static const Atom Ar45("Ar", 18, 45, 0.000000, 44.968090, 1.57593186142);
static const Atom Ar46("Ar", 18, 46, 0.000000, 45.968090, 1.61097742065);
static const Atom Ar47("Ar", 18, 47, 0.000000, 46.972190, 1.64616666667);
static const Atom Ar48("Ar", 18, 48, 0.000000, 47.975070, 1.6813131571);
static const Atom Ar49("Ar", 18, 49, 0.000000, 48.982180, 1.71660789026);
static const Atom Ar50("Ar", 18, 50, 0.000000, 49.985940, 1.75178522079);
static const Atom Ar51("Ar", 18, 51, 0.000000, 50.993240, 1.7870866126);
static const Atom Ar52("Ar", 18, 52, 0.000000, 51.998170, 1.82230494643);
static const Atom Ar53("Ar", 18, 53, 0.000000, 53.006230, 1.85763297286);
static const Atom K("K", 19, 0, 0.000000, 39.098300, 0.862);
static const Atom K32("K", 19, 32, 0.000000, 32.021920, 0.705987089976);
static const Atom K33("K", 19, 33, 0.000000, 33.007260, 0.727710875409);
static const Atom K34("K", 19, 34, 0.000000, 33.998410, 0.749562753879);
static const Atom K35("K", 19, 35, 0.000000, 34.988012, 0.771380503602);
static const Atom K36("K", 19, 36, 0.000000, 35.981293, 0.793279364218);
static const Atom K37("K", 19, 37, 0.000000, 36.973377, 0.815151832597);
static const Atom K38("K", 19, 38, 0.000000, 37.969080, 0.837104095222);
static const Atom K39("K", 19, 39, 93.258100, 38.963707, 0.859032626682);
static const Atom K40("K", 19, 40, 0.011700, 39.963999, 0.881086053704);
static const Atom K41("K", 19, 41, 6.730200, 40.961826, 0.90308514657);
static const Atom K42("K", 19, 42, 0.000000, 41.962403, 0.925144864923);
static const Atom K43("K", 19, 43, 0.000000, 42.960716, 0.947154663809);
static const Atom K44("K", 19, 44, 0.000000, 43.961560, 0.969220265843);
static const Atom K45("K", 19, 45, 0.000000, 44.960700, 0.991248299798);
static const Atom K46("K", 19, 46, 0.000000, 45.961976, 1.01332342613);
static const Atom K47("K", 19, 47, 0.000000, 46.961678, 1.0353638505);
static const Atom K48("K", 19, 48, 0.000000, 47.965513, 1.05749539509);
static const Atom K49("K", 19, 49, 0.000000, 48.967450, 1.07958509449);
static const Atom K50("K", 19, 50, 0.000000, 49.972780, 1.10174959934);
static const Atom K51("K", 19, 51, 0.000000, 50.976380, 1.12387596289);
static const Atom K52("K", 19, 52, 0.000000, 51.982610, 1.14606031004);
static const Atom K53("K", 19, 53, 0.000000, 52.987120, 1.16820673635);
static const Atom K54("K", 19, 54, 0.000000, 53.993990, 1.19040519358);
static const Atom K55("K", 19, 55, 0.000000, 54.999390, 1.21257124172);
static const Atom Ca("Ca", 20, 0, 0.000000, 40.078000, 1.55);
static const Atom Ca34("Ca", 20, 34, 0.000000, 34.014120, 1.31548196018);
static const Atom Ca35("Ca", 20, 35, 0.000000, 35.004770, 1.35379493737);
static const Atom Ca36("Ca", 20, 36, 0.000000, 35.993090, 1.39201780278);
static const Atom Ca37("Ca", 20, 37, 0.000000, 36.985872, 1.43041323419);
static const Atom Ca38("Ca", 20, 38, 0.000000, 37.976319, 1.46871836045);
static const Atom Ca39("Ca", 20, 39, 0.000000, 38.970718, 1.50717631706);
static const Atom Ca40("Ca", 20, 40, 96.941000, 39.962591, 1.54553661261);
static const Atom Ca41("Ca", 20, 41, 0.000000, 40.962278, 1.58419909589);
static const Atom Ca42("Ca", 20, 42, 0.647000, 41.958618, 1.62273213147);
static const Atom Ca43("Ca", 20, 43, 0.135000, 42.958767, 1.6614124592);
static const Atom Ca44("Ca", 20, 44, 2.086000, 43.955481, 1.69995997068);
static const Atom Ca45("Ca", 20, 45, 0.000000, 44.956186, 1.73866181309);
static const Atom Ca46("Ca", 20, 46, 0.004000, 45.953693, 1.77723997804);
static const Atom Ca47("Ca", 20, 47, 0.000000, 46.954546, 1.8159475791);
static const Atom Ca48("Ca", 20, 48, 0.187000, 47.952534, 1.85454433105);
static const Atom Ca49("Ca", 20, 49, 0.000000, 48.955673, 1.89334031514);
static const Atom Ca50("Ca", 20, 50, 0.000000, 49.957518, 1.9320862543);
static const Atom Ca51("Ca", 20, 51, 0.000000, 50.961470, 1.97091368082);
static const Atom Ca52("Ca", 20, 52, 0.000000, 51.965100, 2.00972865412);
static const Atom Ca53("Ca", 20, 53, 0.000000, 52.970050, 2.04859467788);
static const Atom Ca54("Ca", 20, 54, 0.000000, 53.974680, 2.08744832576);
static const Atom Ca55("Ca", 20, 55, 0.000000, 54.980550, 2.12634993014);
static const Atom Ca56("Ca", 20, 56, 0.000000, 55.985790, 2.16522716952);
static const Atom Ca57("Ca", 20, 57, 0.000000, 56.992360, 2.2041558461);
static const Atom Sc("Sc", 21, 0, 0.000000, 44.955910, 2.989);
static const Atom Sc36("Sc", 21, 36, 0.000000, 36.014920, 2.39453713383);
static const Atom Sc37("Sc", 21, 37, 0.000000, 37.003050, 2.46023529387);
static const Atom Sc38("Sc", 21, 38, 0.000000, 37.994700, 2.52616748944);
static const Atom Sc39("Sc", 21, 39, 0.000000, 38.984790, 2.59199596471);
static const Atom Sc40("Sc", 21, 40, 0.000000, 39.977964, 2.65802948702);
static const Atom Sc41("Sc", 21, 41, 0.000000, 40.969251, 2.72393756762);
static const Atom Sc42("Sc", 21, 42, 0.000000, 41.965517, 2.79017663562);
static const Atom Sc43("Sc", 21, 43, 0.000000, 42.961151, 2.85637373015);
static const Atom Sc44("Sc", 21, 44, 0.000000, 43.959403, 2.9227448753);
static const Atom Sc45("Sc", 21, 45, 100.000000, 44.955910, 2.9890000133);
static const Atom Sc46("Sc", 21, 46, 0.000000, 45.955170, 3.05543818436);
static const Atom Sc47("Sc", 21, 47, 0.000000, 46.952408, 3.12174189138);
static const Atom Sc48("Sc", 21, 48, 0.000000, 47.952235, 3.18821775413);
static const Atom Sc49("Sc", 21, 49, 0.000000, 48.950024, 3.25455811563);
static const Atom Sc50("Sc", 21, 50, 0.000000, 49.952187, 3.32118929286);
static const Atom Sc51("Sc", 21, 51, 0.000000, 50.953603, 3.38777080404);
static const Atom Sc52("Sc", 21, 52, 0.000000, 51.956650, 3.45446075611);
static const Atom Sc53("Sc", 21, 53, 0.000000, 52.959240, 3.52112032345);
static const Atom Sc54("Sc", 21, 54, 0.000000, 53.963000, 3.587857681);
static const Atom Sc55("Sc", 21, 55, 0.000000, 54.967430, 3.6546395851);
static const Atom Sc56("Sc", 21, 56, 0.000000, 55.972660, 3.72147467908);
static const Atom Sc57("Sc", 21, 57, 0.000000, 56.977040, 3.7882532588);
static const Atom Sc58("Sc", 21, 58, 0.000000, 57.983070, 3.85514154268);
static const Atom Sc59("Sc", 21, 59, 0.000000, 58.988040, 3.92195934995);
static const Atom Ti("Ti", 22, 0, 0.000000, 47.867000, 4.54);
static const Atom Ti38("Ti", 22, 38, 0.000000, 38.009770, 3.60507982117);
static const Atom Ti39("Ti", 22, 39, 0.000000, 39.001320, 3.69912450749);
static const Atom Ti40("Ti", 22, 40, 0.000000, 39.990500, 3.79294440847);
static const Atom Ti41("Ti", 22, 41, 0.000000, 40.983130, 3.88709152861);
static const Atom Ti42("Ti", 22, 42, 0.000000, 41.973032, 3.9809799085);
static const Atom Ti43("Ti", 22, 43, 0.000000, 42.968523, 4.07539838344);
static const Atom Ti44("Ti", 22, 44, 0.000000, 43.959690, 4.16940676265);
static const Atom Ti45("Ti", 22, 45, 0.000000, 44.958124, 4.26410437926);
static const Atom Ti46("Ti", 22, 46, 8.250000, 45.952630, 4.35842935488);
static const Atom Ti47("Ti", 22, 47, 7.440000, 46.951764, 4.45319338275);
static const Atom Ti48("Ti", 22, 48, 73.720000, 47.947947, 4.54767751967);
static const Atom Ti49("Ti", 22, 49, 5.410000, 48.947871, 4.64251641908);
static const Atom Ti50("Ti", 22, 50, 5.180000, 49.944792, 4.73707055245);
static const Atom Ti51("Ti", 22, 51, 0.000000, 50.946616, 4.83208967848);
static const Atom Ti52("Ti", 22, 52, 0.000000, 51.946898, 4.92696256126);
static const Atom Ti53("Ti", 22, 53, 0.000000, 52.949730, 5.02207730169);
static const Atom Ti54("Ti", 22, 54, 0.000000, 53.950870, 5.11703156245);
static const Atom Ti55("Ti", 22, 55, 0.000000, 54.955120, 5.2122807947);
static const Atom Ti56("Ti", 22, 56, 0.000000, 55.957990, 5.30739913928);
static const Atom Ti57("Ti", 22, 57, 0.000000, 56.962900, 5.40271096998);
static const Atom Ti58("Ti", 22, 58, 0.000000, 57.966110, 5.49786156225);
static const Atom Ti59("Ti", 22, 59, 0.000000, 58.971960, 5.59326254831);
static const Atom Ti60("Ti", 22, 60, 0.000000, 59.975640, 5.68845771826);
static const Atom Ti61("Ti", 22, 61, 0.000000, 60.982020, 5.78390897278);
static const Atom V("V", 23, 0, 0.000000, 50.941500, 6.11);
static const Atom V40("V", 23, 40, 0.000000, 40.011090, 4.7989902123);
static const Atom V41("V", 23, 41, 0.000000, 40.999740, 4.91757037779);
static const Atom V42("V", 23, 42, 0.000000, 41.991230, 5.03649117713);
static const Atom V43("V", 23, 43, 0.000000, 42.980650, 5.15516369757);
static const Atom V44("V", 23, 44, 0.000000, 43.974400, 5.27435556472);
static const Atom V45("V", 23, 45, 0.000000, 44.965782, 5.39326341038);
static const Atom V46("V", 23, 46, 0.000000, 45.960200, 5.51253533848);
static const Atom V47("V", 23, 47, 0.000000, 46.954907, 5.63184203761);
static const Atom V48("V", 23, 48, 0.000000, 47.952255, 5.7514654063);
static const Atom V49("V", 23, 49, 0.000000, 48.948517, 5.87095861447);
static const Atom V50("V", 23, 50, 0.250000, 49.947163, 5.99073770321);
static const Atom V51("V", 23, 51, 99.750000, 50.943964, 6.11029549988);
static const Atom V52("V", 23, 52, 0.000000, 51.944780, 6.23033487367);
static const Atom V53("V", 23, 53, 0.000000, 52.944343, 6.35022399674);
static const Atom V54("V", 23, 54, 0.000000, 53.946444, 6.47041749536);
static const Atom V55("V", 23, 55, 0.000000, 54.947240, 6.59045447032);
static const Atom V56("V", 23, 56, 0.000000, 55.950360, 6.71077018933);
static const Atom V57("V", 23, 57, 0.000000, 56.952360, 6.83095157386);
static const Atom V58("V", 23, 58, 0.000000, 57.956650, 6.95140762443);
static const Atom V59("V", 23, 59, 0.000000, 58.959300, 7.07166697094);
static const Atom V60("V", 23, 60, 0.000000, 59.964500, 7.19223216827);
static const Atom V61("V", 23, 61, 0.000000, 60.967410, 7.31252269957);
static const Atom V62("V", 23, 62, 0.000000, 61.973140, 7.4331514659);
static const Atom V63("V", 23, 63, 0.000000, 62.976750, 7.55352595624);
static const Atom Cr("Cr", 24, 0, 0.000000, 51.996100, 7.19);
static const Atom Cr42("Cr", 24, 42, 0.000000, 42.006430, 5.80863241089);
static const Atom Cr43("Cr", 24, 43, 0.000000, 42.997710, 5.9457062145);
static const Atom Cr44("Cr", 24, 44, 0.000000, 43.985470, 6.08229327392);
static const Atom Cr45("Cr", 24, 45, 0.000000, 44.979160, 6.21970033137);
static const Atom Cr46("Cr", 24, 46, 0.000000, 45.968362, 6.35648678997);
static const Atom Cr47("Cr", 24, 47, 0.000000, 46.962907, 6.49401207648);
static const Atom Cr48("Cr", 24, 48, 0.000000, 47.954036, 6.63106499987);
static const Atom Cr49("Cr", 24, 49, 0.000000, 48.951341, 6.76897195192);
static const Atom Cr50("Cr", 24, 50, 4.345000, 49.946050, 6.90651984714);
static const Atom Cr51("Cr", 24, 51, 0.000000, 50.944772, 7.04462275521);
static const Atom Cr52("Cr", 24, 52, 83.789000, 51.940512, 7.18231329967);
static const Atom Cr53("Cr", 24, 53, 9.501000, 52.940654, 7.32061252329);
static const Atom Cr54("Cr", 24, 54, 2.365000, 53.938885, 7.45864752224);
static const Atom Cr55("Cr", 24, 55, 0.000000, 54.940844, 7.5971980552);
static const Atom Cr56("Cr", 24, 56, 0.000000, 55.940645, 7.73545011164);
static const Atom Cr57("Cr", 24, 57, 0.000000, 56.943750, 7.87415907155);
static const Atom Cr58("Cr", 24, 58, 0.000000, 57.944250, 8.01250781309);
static const Atom Cr59("Cr", 24, 59, 0.000000, 58.948630, 8.15139307948);
static const Atom Cr60("Cr", 24, 60, 0.000000, 59.949730, 8.28982478878);
static const Atom Cr61("Cr", 24, 61, 0.000000, 60.954090, 8.42870728959);
static const Atom Cr62("Cr", 24, 62, 0.000000, 61.955800, 8.56722334944);
static const Atom Cr63("Cr", 24, 63, 0.000000, 62.961860, 8.70634092557);
static const Atom Cr64("Cr", 24, 64, 0.000000, 63.964200, 8.84494410158);
static const Atom Cr65("Cr", 24, 65, 0.000000, 64.970370, 8.98407688846);
static const Atom Mn("Mn", 25, 0, 0.000000, 54.938049, 7.33);
static const Atom Mn44("Mn", 25, 44, 0.000000, 44.006870, 5.87152916734);
static const Atom Mn45("Mn", 25, 45, 0.000000, 44.994510, 6.00330307143);
static const Atom Mn46("Mn", 25, 46, 0.000000, 45.986720, 6.13568671869);
static const Atom Mn47("Mn", 25, 47, 0.000000, 46.976100, 6.26769277882);
static const Atom Mn48("Mn", 25, 48, 0.000000, 47.968870, 6.40015114297);
static const Atom Mn49("Mn", 25, 49, 0.000000, 48.959623, 6.5323403929);
static const Atom Mn50("Mn", 25, 50, 0.000000, 49.954244, 6.66504572305);
static const Atom Mn51("Mn", 25, 51, 0.000000, 50.948216, 6.79766439494);
static const Atom Mn52("Mn", 25, 52, 0.000000, 51.945570, 6.93073445024);
static const Atom Mn53("Mn", 25, 53, 0.000000, 52.941295, 7.06358702602);
static const Atom Mn54("Mn", 25, 54, 0.000000, 53.940363, 7.19688575501);
static const Atom Mn55("Mn", 25, 55, 100.000000, 54.938050, 7.33000008005);
static const Atom Mn56("Mn", 25, 56, 0.000000, 55.938909, 7.46353780969);
static const Atom Mn57("Mn", 25, 57, 0.000000, 56.938287, 7.59687777973);
static const Atom Mn58("Mn", 25, 58, 0.000000, 57.939990, 7.73052801165);
static const Atom Mn59("Mn", 25, 59, 0.000000, 58.940450, 7.86401239877);
static const Atom Mn60("Mn", 25, 60, 0.000000, 59.943190, 7.99780099035);
static const Atom Mn61("Mn", 25, 61, 0.000000, 60.944460, 8.1313934501);
static const Atom Mn62("Mn", 25, 62, 0.000000, 61.947970, 8.2652847774);
static const Atom Mn63("Mn", 25, 63, 0.000000, 62.949810, 8.39895328828);
static const Atom Mn64("Mn", 25, 64, 0.000000, 63.953730, 8.53289931901);
static const Atom Mn65("Mn", 25, 65, 0.000000, 64.956100, 8.66663854408);
static const Atom Mn66("Mn", 25, 66, 0.000000, 65.960820, 8.80069131323);
static const Atom Mn67("Mn", 25, 67, 0.000000, 66.963820, 8.93451459479);
static const Atom Fe("Fe", 26, 0, 0.000000, 55.845000, 7.874);
static const Atom Fe45("Fe", 26, 45, 0.000000, 45.014560, 6.34693608094);
static const Atom Fe46("Fe", 26, 46, 0.000000, 46.000810, 6.48599477017);
static const Atom Fe47("Fe", 26, 47, 0.000000, 46.992890, 6.62587547426);
static const Atom Fe48("Fe", 26, 48, 0.000000, 47.980560, 6.7651343798);
static const Atom Fe49("Fe", 26, 49, 0.000000, 48.973610, 6.90515185137);
static const Atom Fe50("Fe", 26, 50, 0.000000, 49.962990, 7.04465186248);
static const Atom Fe51("Fe", 26, 51, 0.000000, 50.956825, 7.18478001701);
static const Atom Fe52("Fe", 26, 52, 0.000000, 51.948117, 7.32454961515);
static const Atom Fe53("Fe", 26, 53, 0.000000, 52.945312, 7.46515156326);
static const Atom Fe54("Fe", 26, 54, 5.845000, 53.939615, 7.60534563408);
static const Atom Fe55("Fe", 26, 55, 0.000000, 54.938298, 7.74615737223);
static const Atom Fe56("Fe", 26, 56, 91.754000, 55.934942, 7.88668160257);
static const Atom Fe57("Fe", 26, 57, 2.119000, 56.935399, 8.02774338551);
static const Atom Fe58("Fe", 26, 58, 0.282000, 57.933281, 8.16844212834);
static const Atom Fe59("Fe", 26, 59, 0.000000, 58.934880, 8.30966512771);
static const Atom Fe60("Fe", 26, 60, 0.000000, 59.934077, 8.45054923982);
static const Atom Fe61("Fe", 26, 61, 0.000000, 60.936749, 8.59192338841);
static const Atom Fe62("Fe", 26, 62, 0.000000, 61.936770, 8.73292375289);
static const Atom Fe63("Fe", 26, 63, 0.000000, 62.940120, 8.87439349772);
static const Atom Fe64("Fe", 26, 64, 0.000000, 63.940870, 9.0154966493);
static const Atom Fe65("Fe", 26, 65, 0.000000, 64.944940, 9.15706791226);
static const Atom Fe66("Fe", 26, 66, 0.000000, 65.945980, 9.29821195308);
static const Atom Fe67("Fe", 26, 67, 0.000000, 66.950000, 9.43977616617);
static const Atom Fe68("Fe", 26, 68, 0.000000, 67.952510, 9.58112747318);
static const Atom Fe69("Fe", 26, 69, 0.000000, 68.957700, 9.72285665324);
static const Atom Co("Co", 27, 0, 0.000000, 58.933200, 8.9);
static const Atom Co48("Co", 27, 48, 0.000000, 48.001760, 7.24915097093);
static const Atom Co49("Co", 27, 49, 0.000000, 48.989720, 7.39835115012);
static const Atom Co50("Co", 27, 50, 0.000000, 49.981540, 7.54813426048);
static const Atom Co51("Co", 27, 51, 0.000000, 50.970720, 7.69751868217);
static const Atom Co52("Co", 27, 52, 0.000000, 51.963590, 7.8474603619);
static const Atom Co53("Co", 27, 53, 0.000000, 52.954225, 7.99706451542);
static const Atom Co54("Co", 27, 54, 0.000000, 53.948464, 8.1472129545);
static const Atom Co55("Co", 27, 55, 0.000000, 54.942003, 8.29725566557);
static const Atom Co56("Co", 27, 56, 0.000000, 55.939844, 8.44794802777);
static const Atom Co57("Co", 27, 57, 0.000000, 56.936296, 8.59843070086);
static const Atom Co58("Co", 27, 58, 0.000000, 57.935758, 8.74936780355);
static const Atom Co59("Co", 27, 59, 100.000000, 58.933200, 8.9000000302);
static const Atom Co60("Co", 27, 60, 0.000000, 59.933822, 9.05111240489);
static const Atom Co61("Co", 27, 61, 0.000000, 60.932479, 9.20192805855);
static const Atom Co62("Co", 27, 62, 0.000000, 61.934054, 9.3531842934);
static const Atom Co63("Co", 27, 63, 0.000000, 62.933615, 9.50413643753);
static const Atom Co64("Co", 27, 64, 0.000000, 63.935814, 9.6554869683);
static const Atom Co65("Co", 27, 65, 0.000000, 64.936485, 9.80660674289);
static const Atom Co66("Co", 27, 66, 0.000000, 65.939830, 9.95813034079);
static const Atom Co67("Co", 27, 67, 0.000000, 66.940610, 10.1092665764);
static const Atom Co68("Co", 27, 68, 0.000000, 67.944360, 10.2608513368);
static const Atom Co69("Co", 27, 69, 0.000000, 68.945200, 10.4119966335);
static const Atom Co70("Co", 27, 70, 0.000000, 69.949810, 10.5637112697);
static const Atom Co71("Co", 27, 71, 0.000000, 70.951730, 10.7150196663);
static const Atom Co72("Co", 27, 72, 0.000000, 71.956410, 10.8667448739);
static const Atom Ni("Ni", 28, 0, 0.000000, 58.693400, 8.902);
static const Atom Ni50("Ni", 28, 50, 0.000000, 49.995930, 7.58285887101);
static const Atom Ni51("Ni", 28, 51, 0.000000, 50.987720, 7.73328318755);
static const Atom Ni52("Ni", 28, 52, 0.000000, 51.975680, 7.88312660981);
static const Atom Ni53("Ni", 28, 53, 0.000000, 52.968460, 8.03370107917);
static const Atom Ni54("Ni", 28, 54, 0.000000, 53.957910, 8.18377048902);
static const Atom Ni55("Ni", 28, 55, 0.000000, 54.951336, 8.33444293689);
static const Atom Ni56("Ni", 28, 56, 0.000000, 55.942136, 8.48471710059);
static const Atom Ni57("Ni", 28, 57, 0.000000, 56.939800, 8.63603232391);
static const Atom Ni58("Ni", 28, 58, 68.076900, 57.935348, 8.78702659934);
static const Atom Ni59("Ni", 28, 59, 0.000000, 58.934352, 8.93854501432);
static const Atom Ni60("Ni", 28, 60, 26.223100, 59.930791, 9.08967444246);
static const Atom Ni61("Ni", 28, 61, 1.139900, 60.931060, 9.24138488622);
static const Atom Ni62("Ni", 28, 62, 3.634500, 61.928349, 9.39264314246);
static const Atom Ni63("Ni", 28, 63, 0.000000, 62.929673, 9.54451349139);
static const Atom Ni64("Ni", 28, 64, 0.925600, 63.927970, 9.69592467601);
static const Atom Ni65("Ni", 28, 65, 0.000000, 64.930088, 9.84791549605);
static const Atom Ni66("Ni", 28, 66, 0.000000, 65.929115, 9.99943744493);
static const Atom Ni67("Ni", 28, 67, 0.000000, 66.931570, 10.1514793169);
static const Atom Ni68("Ni", 28, 68, 0.000000, 67.931845, 10.3031905494);
static const Atom Ni69("Ni", 28, 69, 0.000000, 68.935180, 10.4553658905);
static const Atom Ni70("Ni", 28, 70, 0.000000, 69.936140, 10.6071810166);
static const Atom Ni71("Ni", 28, 71, 0.000000, 70.940000, 10.7594359843);
static const Atom Ni72("Ni", 28, 72, 0.000000, 71.941300, 10.911302678);
static const Atom Ni73("Ni", 28, 73, 0.000000, 72.946080, 11.0636971816);
static const Atom Ni74("Ni", 28, 74, 0.000000, 73.947910, 11.2156442602);
static const Atom Ni75("Ni", 28, 75, 0.000000, 74.952970, 11.3680812313);
static const Atom Ni76("Ni", 28, 76, 0.000000, 75.955330, 11.5201086947);
static const Atom Ni77("Ni", 28, 77, 0.000000, 76.960830, 11.6726124004);
static const Atom Ni78("Ni", 28, 78, 0.000000, 77.963800, 11.8247323822);
static const Atom Cu("Cu", 29, 0, 0.000000, 63.546000, 8.96);
static const Atom Cu52("Cu", 29, 52, 0.000000, 51.997180, 7.33161383565);
static const Atom Cu53("Cu", 29, 53, 0.000000, 52.985550, 7.4709742234);
static const Atom Cu54("Cu", 29, 54, 0.000000, 53.976710, 7.61072800176);
static const Atom Cu55("Cu", 29, 55, 0.000000, 54.966050, 7.75022515973);
static const Atom Cu56("Cu", 29, 56, 0.000000, 55.958560, 7.89016928839);
static const Atom Cu57("Cu", 29, 57, 0.000000, 56.949216, 8.02985200264);
static const Atom Cu58("Cu", 29, 58, 0.000000, 57.944541, 8.17019300463);
static const Atom Cu59("Cu", 29, 59, 0.000000, 58.939504, 8.31048306323);
static const Atom Cu60("Cu", 29, 60, 0.000000, 59.937368, 8.45118210707);
static const Atom Cu61("Cu", 29, 61, 0.000000, 60.933462, 8.59163159462);
static const Atom Cu62("Cu", 29, 62, 0.000000, 61.932587, 8.73250841154);
static const Atom Cu63("Cu", 29, 63, 69.170000, 62.929601, 8.8730876193);
static const Atom Cu64("Cu", 29, 64, 0.000000, 63.929768, 9.01411135845);
static const Atom Cu65("Cu", 29, 65, 30.830000, 64.927794, 9.15483321613);
static const Atom Cu66("Cu", 29, 66, 0.000000, 65.928873, 9.29598561798);
static const Atom Cu67("Cu", 29, 67, 0.000000, 66.927750, 9.43682749504);
static const Atom Cu68("Cu", 29, 68, 0.000000, 67.929640, 9.57809420577);
static const Atom Cu69("Cu", 29, 69, 0.000000, 68.929425, 9.71906411104);
static const Atom Cu70("Cu", 29, 70, 0.000000, 69.932409, 9.86048507601);
static const Atom Cu71("Cu", 29, 71, 0.000000, 70.932620, 10.0015150474);
static const Atom Cu72("Cu", 29, 72, 0.000000, 71.935520, 10.1429241683);
static const Atom Cu73("Cu", 29, 73, 0.000000, 72.936490, 10.2840611588);
static const Atom Cu74("Cu", 29, 74, 0.000000, 73.940200, 10.42558449);
static const Atom Cu75("Cu", 29, 75, 0.000000, 74.941700, 10.5667962106);
static const Atom Cu76("Cu", 29, 76, 0.000000, 75.945990, 10.7084013219);
static const Atom Cu77("Cu", 29, 77, 0.000000, 76.947950, 10.8496779026);
static const Atom Cu78("Cu", 29, 78, 0.000000, 77.952810, 10.991363384);
static const Atom Cu79("Cu", 29, 79, 0.000000, 78.955280, 11.1327118749);
static const Atom Cu80("Cu", 29, 80, 0.000000, 79.961890, 11.2746441066);
static const Atom Zn("Zn", 30, 0, 0.000000, 65.409000, 7.133);
static const Atom Zn54("Zn", 30, 54, 0.000000, 53.992950, 5.88805382058);
static const Atom Zn55("Zn", 30, 55, 0.000000, 54.983980, 5.9961278928);
static const Atom Zn56("Zn", 30, 56, 0.000000, 55.972380, 6.10391515755);
static const Atom Zn57("Zn", 30, 57, 0.000000, 56.964910, 6.21215280818);
static const Atom Zn58("Zn", 30, 58, 0.000000, 57.954600, 6.32008075036);
static const Atom Zn59("Zn", 30, 59, 0.000000, 58.949270, 6.42855177284);
static const Atom Zn60("Zn", 30, 60, 0.000000, 59.941832, 6.53679291315);
static const Atom Zn61("Zn", 30, 61, 0.000000, 60.939514, 6.64559240108);
static const Atom Zn62("Zn", 30, 62, 0.000000, 61.934334, 6.75407978141);
static const Atom Zn63("Zn", 30, 63, 0.000000, 62.933216, 6.86301008844);
static const Atom Zn64("Zn", 30, 64, 48.630000, 63.929147, 6.97161862584);
static const Atom Zn65("Zn", 30, 65, 0.000000, 64.929245, 7.08068163859);
static const Atom Zn66("Zn", 30, 66, 27.900000, 65.926037, 7.18938403728);
static const Atom Zn67("Zn", 30, 67, 4.100000, 66.927131, 7.29855562246);
static const Atom Zn68("Zn", 30, 68, 18.750000, 67.924848, 7.40735889451);
static const Atom Zn69("Zn", 30, 69, 0.000000, 68.926553, 7.51659719787);
static const Atom Zn70("Zn", 30, 70, 0.620000, 69.925325, 7.62551549825);
static const Atom Zn71("Zn", 30, 71, 0.000000, 70.927727, 7.7348297129);
static const Atom Zn72("Zn", 30, 72, 0.000000, 71.926861, 7.84378754473);
static const Atom Zn73("Zn", 30, 73, 0.000000, 72.929780, 7.9531581394);
static const Atom Zn74("Zn", 30, 74, 0.000000, 73.929460, 8.06217551377);
static const Atom Zn75("Zn", 30, 75, 0.000000, 74.932940, 8.17160728676);
static const Atom Zn76("Zn", 30, 76, 0.000000, 75.933390, 8.28070863138);
static const Atom Zn77("Zn", 30, 77, 0.000000, 76.937090, 8.39016439588);
static const Atom Zn78("Zn", 30, 78, 0.000000, 77.938570, 8.49937806433);
static const Atom Zn79("Zn", 30, 79, 0.000000, 78.942680, 8.60887854026);
static const Atom Zn80("Zn", 30, 80, 0.000000, 79.944410, 8.71811947179);
static const Atom Zn81("Zn", 30, 81, 0.000000, 80.950480, 8.82783369016);
static const Atom Zn82("Zn", 30, 82, 0.000000, 81.954840, 8.93736142916);
static const Atom Ga("Ga", 31, 0, 0.000000, 69.723000, 5.904);
static const Atom Ga56("Ga", 31, 56, 0.000000, 55.994910, 4.74153362076);
static const Atom Ga57("Ga", 31, 57, 0.000000, 56.982930, 4.82519711888);
static const Atom Ga58("Ga", 31, 58, 0.000000, 57.974250, 4.90914005421);
static const Atom Ga59("Ga", 31, 59, 0.000000, 58.963370, 4.99289669808);
static const Atom Ga60("Ga", 31, 60, 0.000000, 59.957060, 5.07704032012);
static const Atom Ga61("Ga", 31, 61, 0.000000, 60.949170, 5.16105015103);
static const Atom Ga62("Ga", 31, 62, 0.000000, 61.944180, 5.24530554795);
static const Atom Ga63("Ga", 31, 63, 0.000000, 62.939140, 5.32955671098);
static const Atom Ga64("Ga", 31, 64, 0.000000, 63.936838, 5.41403972222);
static const Atom Ga65("Ga", 31, 65, 0.000000, 64.932739, 5.49837059259);
static const Atom Ga66("Ga", 31, 66, 0.000000, 65.931592, 5.58295138144);
static const Atom Ga67("Ga", 31, 67, 0.000000, 66.928205, 5.66734250864);
static const Atom Ga68("Ga", 31, 68, 0.000000, 67.927983, 5.75200170079);
static const Atom Ga69("Ga", 31, 69, 60.108000, 68.925581, 5.83647620188);
static const Atom Ga70("Ga", 31, 70, 0.000000, 69.926028, 5.92119199277);
static const Atom Ga71("Ga", 31, 71, 39.892000, 70.924705, 6.0057579037);
static const Atom Ga72("Ga", 31, 72, 0.000000, 71.926369, 6.09057678152);
static const Atom Ga73("Ga", 31, 73, 0.000000, 72.925170, 6.17515315864);
static const Atom Ga74("Ga", 31, 74, 0.000000, 73.926940, 6.25998097844);
static const Atom Ga75("Ga", 31, 75, 0.000000, 74.926501, 6.34462174468);
static const Atom Ga76("Ga", 31, 76, 0.000000, 75.928930, 6.42950536724);
static const Atom Ga77("Ga", 31, 77, 0.000000, 76.929280, 6.51421294437);
static const Atom Ga78("Ga", 31, 78, 0.000000, 77.931660, 6.59909241771);
static const Atom Ga79("Ga", 31, 79, 0.000000, 78.932920, 6.68387705176);
static const Atom Ga80("Ga", 31, 80, 0.000000, 79.936590, 6.76886575965);
static const Atom Ga81("Ga", 31, 81, 0.000000, 80.937750, 6.85364192591);
static const Atom Ga82("Ga", 31, 82, 0.000000, 81.943160, 6.93877797341);
static const Atom Ga83("Ga", 31, 83, 0.000000, 82.946870, 7.02377006841);
static const Atom Ga84("Ga", 31, 84, 0.000000, 83.952340, 7.10891119659);
static const Atom Ge("Ge", 32, 0, 0.000000, 72.640000, 5.323);
static const Atom Ge58("Ge", 32, 58, 0.000000, 57.991010, 4.2495339514);
static const Atom Ge59("Ge", 32, 59, 0.000000, 58.981750, 4.32213457117);
static const Atom Ge60("Ge", 32, 60, 0.000000, 59.970190, 4.39456664882);
static const Atom Ge61("Ge", 32, 61, 0.000000, 60.963790, 4.46737684705);
static const Atom Ge62("Ge", 32, 62, 0.000000, 61.954650, 4.53998626032);
static const Atom Ge63("Ge", 32, 63, 0.000000, 62.949640, 4.61289831663);
static const Atom Ge64("Ge", 32, 64, 0.000000, 63.941570, 4.68558613863);
static const Atom Ge65("Ge", 32, 65, 0.000000, 64.939440, 4.75870923899);
static const Atom Ge66("Ge", 32, 66, 0.000000, 65.933850, 4.83157879336);
static const Atom Ge67("Ge", 32, 67, 0.000000, 66.932738, 4.90477649193);
static const Atom Ge68("Ge", 32, 68, 0.000000, 67.928097, 4.97771558826);
static const Atom Ge69("Ge", 32, 69, 0.000000, 68.927972, 5.05098561338);
static const Atom Ge70("Ge", 32, 70, 20.840000, 69.924250, 5.12399208259);
static const Atom Ge71("Ge", 32, 71, 0.000000, 70.924954, 5.19732282684);
static const Atom Ge72("Ge", 32, 72, 27.540000, 71.922076, 5.27039112903);
static const Atom Ge73("Ge", 32, 73, 7.730000, 72.923459, 5.34377167382);
static const Atom Ge74("Ge", 32, 74, 36.280000, 73.921178, 5.41688369436);
static const Atom Ge75("Ge", 32, 75, 0.000000, 74.922860, 5.49028608368);
static const Atom Ge76("Ge", 32, 76, 7.610000, 75.921403, 5.56345851559);
static const Atom Ge77("Ge", 32, 77, 0.000000, 76.923548, 5.63689494308);
static const Atom Ge78("Ge", 32, 78, 0.000000, 77.922853, 5.71012316243);
static const Atom Ge79("Ge", 32, 79, 0.000000, 78.925400, 5.78358898954);
static const Atom Ge80("Ge", 32, 80, 0.000000, 79.925445, 5.85687147212);
static const Atom Ge81("Ge", 32, 81, 0.000000, 80.928820, 5.93039797439);
static const Atom Ge82("Ge", 32, 82, 0.000000, 81.929550, 6.00373065322);
static const Atom Ge83("Ge", 32, 83, 0.000000, 82.934510, 6.077373303);
static const Atom Ge84("Ge", 32, 84, 0.000000, 83.937310, 6.15085766974);
static const Atom Ge85("Ge", 32, 85, 0.000000, 84.942690, 6.22453109678);
static const Atom Ge86("Ge", 32, 86, 0.000000, 85.946270, 6.29807262128);
static const Atom As("As", 33, 0, 0.000000, 74.921600, 5.73);
static const Atom As60("As", 33, 60, 0.000000, 59.993130, 4.58827140504);
static const Atom As61("As", 33, 61, 0.000000, 60.980620, 4.66379458794);
static const Atom As62("As", 33, 62, 0.000000, 61.973200, 4.73970705377);
static const Atom As63("As", 33, 63, 0.000000, 62.963690, 4.81545967652);
static const Atom As64("As", 33, 64, 0.000000, 63.957570, 4.89147156628);
static const Atom As65("As", 33, 65, 0.000000, 64.949480, 4.96733279054);
static const Atom As66("As", 33, 66, 0.000000, 65.944370, 5.04342192505);
static const Atom As67("As", 33, 67, 0.000000, 66.939190, 5.11950570596);
static const Atom As68("As", 33, 68, 0.000000, 67.936790, 5.19580210113);
static const Atom As69("As", 33, 69, 0.000000, 68.932280, 5.27193712361);
static const Atom As70("As", 33, 70, 0.000000, 69.930930, 5.34831382272);
static const Atom As71("As", 33, 71, 0.000000, 70.927115, 5.42450199876);
static const Atom As72("As", 33, 72, 0.000000, 71.926753, 5.50095426005);
static const Atom As73("As", 33, 73, 0.000000, 72.923825, 5.57721027381);
static const Atom As74("As", 33, 74, 0.000000, 73.923929, 5.65369818241);
static const Atom As75("As", 33, 75, 100.000000, 74.921596, 5.72999972467);
static const Atom As76("As", 33, 76, 0.000000, 75.922394, 5.80654066447);
static const Atom As77("As", 33, 77, 0.000000, 76.920648, 5.88288706222);
static const Atom As78("As", 33, 78, 0.000000, 77.921829, 5.95945735502);
static const Atom As79("As", 33, 79, 0.000000, 78.920948, 6.03586992323);
static const Atom As80("As", 33, 80, 0.000000, 79.922578, 6.11247453258);
static const Atom As81("As", 33, 81, 0.000000, 80.922133, 6.18892044604);
static const Atom As82("As", 33, 82, 0.000000, 81.924500, 6.26558142111);
static const Atom As83("As", 33, 83, 0.000000, 82.924980, 6.34209807852);
static const Atom As84("As", 33, 84, 0.000000, 83.929060, 6.41889006375);
static const Atom As85("As", 33, 85, 0.000000, 84.931810, 6.49558033064);
static const Atom As86("As", 33, 86, 0.000000, 85.936230, 6.57239831904);
static const Atom As87("As", 33, 87, 0.000000, 86.939580, 6.6491344739);
static const Atom As88("As", 33, 88, 0.000000, 87.944560, 6.72599529108);
static const Atom As89("As", 33, 89, 0.000000, 88.949230, 6.80283239947);
static const Atom Se("Se", 34, 0, 0.000000, 78.960000, 4.79);
static const Atom Se65("Se", 34, 65, 0.000000, 64.964660, 3.94099191236);
static const Atom Se66("Se", 34, 66, 0.000000, 65.955210, 4.00108226824);
static const Atom Se67("Se", 34, 67, 0.000000, 66.950090, 4.06143529762);
static const Atom Se68("Se", 34, 68, 0.000000, 67.941870, 4.12160026976);
static const Atom Se69("Se", 34, 69, 0.000000, 68.939560, 4.18212376393);
static const Atom Se70("Se", 34, 70, 0.000000, 69.933500, 4.2424197695);
static const Atom Se71("Se", 34, 71, 0.000000, 70.932270, 4.3030087804);
static const Atom Se72("Se", 34, 72, 0.000000, 71.927112, 4.36335950456);
static const Atom Se73("Se", 34, 73, 0.000000, 72.926767, 4.42400220276);
static const Atom Se74("Se", 34, 74, 0.890000, 73.922477, 4.48440555869);
static const Atom Se75("Se", 34, 75, 0.000000, 74.922524, 4.54507203703);
static const Atom Se76("Se", 34, 76, 9.370000, 75.919214, 4.60553489791);
static const Atom Se77("Se", 34, 77, 7.630000, 76.919915, 4.66624101993);
static const Atom Se78("Se", 34, 78, 23.770000, 77.917310, 4.72674661227);
static const Atom Se79("Se", 34, 79, 0.000000, 78.918500, 4.78748244734);
static const Atom Se80("Se", 34, 80, 49.610000, 79.916522, 4.84802608184);
static const Atom Se81("Se", 34, 81, 0.000000, 80.917993, 4.90877895125);
static const Atom Se82("Se", 34, 82, 8.730000, 81.916700, 4.9693641464);
static const Atom Se83("Se", 34, 83, 0.000000, 82.919119, 5.03017451887);
static const Atom Se84("Se", 34, 84, 0.000000, 83.918465, 5.09079847201);
static const Atom Se85("Se", 34, 85, 0.000000, 84.922240, 5.15169110436);
static const Atom Se86("Se", 34, 86, 0.000000, 85.924271, 5.21247793934);
static const Atom Se87("Se", 34, 87, 0.000000, 86.928520, 5.27339932624);
static const Atom Se88("Se", 34, 88, 0.000000, 87.931420, 5.33423887791);
static const Atom Se89("Se", 34, 89, 0.000000, 88.936020, 5.39518155775);
static const Atom Se90("Se", 34, 90, 0.000000, 89.939420, 5.45605144124);
static const Atom Se91("Se", 34, 91, 0.000000, 90.945370, 5.51707601697);
static const Atom Se92("Se", 34, 92, 0.000000, 91.949330, 5.57797987209);
static const Atom Br("Br", 35, 0, 0.000000, 79.904000, 3.12);
static const Atom Br67("Br", 35, 67, 0.000000, 66.964790, 2.61476452743);
static const Atom Br68("Br", 35, 68, 0.000000, 67.958250, 2.65355601722);
static const Atom Br69("Br", 35, 69, 0.000000, 68.950180, 2.69228776532);
static const Atom Br70("Br", 35, 70, 0.000000, 69.944620, 2.73111752103);
static const Atom Br71("Br", 35, 71, 0.000000, 70.939250, 2.76995469563);
static const Atom Br72("Br", 35, 72, 0.000000, 71.936500, 2.80889417301);
static const Atom Br73("Br", 35, 73, 0.000000, 72.931790, 2.84775711854);
static const Atom Br74("Br", 35, 74, 0.000000, 73.929891, 2.88672982479);
static const Atom Br75("Br", 35, 75, 0.000000, 74.925776, 2.9256160032);
static const Atom Br76("Br", 35, 76, 0.000000, 75.924542, 2.96461467561);
static const Atom Br77("Br", 35, 77, 0.000000, 76.921380, 3.00353806568);
static const Atom Br78("Br", 35, 78, 0.000000, 77.921146, 3.04257578494);
static const Atom Br79("Br", 35, 79, 50.690000, 78.918338, 3.08151298198);
static const Atom Br80("Br", 35, 80, 0.000000, 79.918530, 3.12056735082);
static const Atom Br81("Br", 35, 81, 49.310000, 80.916291, 3.15952678114);
static const Atom Br82("Br", 35, 82, 0.000000, 81.916805, 3.19859370745);
static const Atom Br83("Br", 35, 83, 0.000000, 82.915180, 3.23757711254);
static const Atom Br84("Br", 35, 84, 0.000000, 83.916504, 3.2766756668);
static const Atom Br85("Br", 35, 85, 0.000000, 84.915608, 3.31568753704);
static const Atom Br86("Br", 35, 86, 0.000000, 85.918797, 3.3548589137);
static const Atom Br87("Br", 35, 87, 0.000000, 86.920711, 3.39398050561);
static const Atom Br88("Br", 35, 88, 0.000000, 87.924070, 3.43315852022);
static const Atom Br89("Br", 35, 89, 0.000000, 88.926390, 3.47229596516);
static const Atom Br90("Br", 35, 90, 0.000000, 89.930630, 3.51150838006);
static const Atom Br91("Br", 35, 91, 0.000000, 90.933970, 3.55068565278);
static const Atom Br92("Br", 35, 92, 0.000000, 91.939260, 3.58993906688);
static const Atom Br93("Br", 35, 93, 0.000000, 92.943100, 3.62913586304);
static const Atom Br94("Br", 35, 94, 0.000000, 93.948680, 3.66840060072);
static const Atom Kr("Kr", 36, 0, 0.000000, 83.798000, 2.16);
static const Atom Kr69("Kr", 36, 69, 0.000000, 68.965320, 1.7776688131);
static const Atom Kr70("Kr", 36, 70, 0.000000, 69.956010, 1.80320510752);
static const Atom Kr71("Kr", 36, 71, 0.000000, 70.950510, 1.82883960954);
static const Atom Kr72("Kr", 36, 72, 0.000000, 71.941910, 1.85439420511);
static const Atom Kr73("Kr", 36, 73, 0.000000, 72.938930, 1.88009366333);
static const Atom Kr74("Kr", 36, 74, 0.000000, 73.933260, 1.90572378338);
static const Atom Kr75("Kr", 36, 75, 0.000000, 74.931034, 1.93144267691);
static const Atom Kr76("Kr", 36, 76, 0.000000, 75.925948, 1.95708785031);
static const Atom Kr77("Kr", 36, 77, 0.000000, 76.924668, 1.98283112819);
static const Atom Kr78("Kr", 36, 78, 0.350000, 77.920386, 2.0084970257);
static const Atom Kr79("Kr", 36, 79, 0.000000, 78.920083, 2.034265487);
static const Atom Kr80("Kr", 36, 80, 2.280000, 79.916378, 2.05994625743);
static const Atom Kr81("Kr", 36, 81, 0.000000, 80.916592, 2.08572804506);
static const Atom Kr82("Kr", 36, 82, 11.580000, 81.913485, 2.11142421938);
static const Atom Kr83("Kr", 36, 83, 11.490000, 82.914136, 2.13721728156);
static const Atom Kr84("Kr", 36, 84, 57.000000, 83.911507, 2.16292578725);
static const Atom Kr85("Kr", 36, 85, 0.000000, 84.912527, 2.18872835056);
static const Atom Kr86("Kr", 36, 86, 17.300000, 85.910610, 2.21445521669);
static const Atom Kr87("Kr", 36, 87, 0.000000, 86.913354, 2.24030221829);
static const Atom Kr88("Kr", 36, 88, 0.000000, 87.914447, 2.26610665553);
static const Atom Kr89("Kr", 36, 89, 0.000000, 88.917630, 2.29196497291);
static const Atom Kr90("Kr", 36, 90, 0.000000, 89.919524, 2.31779006468);
static const Atom Kr91("Kr", 36, 91, 0.000000, 90.923440, 2.34366727607);
static const Atom Kr92("Kr", 36, 92, 0.000000, 91.926153, 2.3695134786);
static const Atom Kr93("Kr", 36, 93, 0.000000, 92.931270, 2.39542164729);
static const Atom Kr94("Kr", 36, 94, 0.000000, 93.934360, 2.42127756748);
static const Atom Kr95("Kr", 36, 95, 0.000000, 94.939840, 2.44719509296);
static const Atom Kr96("Kr", 36, 96, 0.000000, 95.943070, 2.47305462183);
static const Atom Kr97("Kr", 36, 97, 0.000000, 96.948560, 2.49897240507);
static const Atom Rb("Rb", 37, 0, 0.000000, 85.467800, 1.532);
static const Atom Rb71("Rb", 37, 71, 0.000000, 70.965320, 1.27204479629);
static const Atom Rb72("Rb", 37, 72, 0.000000, 71.959080, 1.28985782435);
static const Atom Rb73("Rb", 37, 73, 0.000000, 72.950370, 1.30762657796);
static const Atom Rb74("Rb", 37, 74, 0.000000, 73.944470, 1.32544570049);
static const Atom Rb75("Rb", 37, 75, 0.000000, 74.938569, 1.34326480508);
static const Atom Rb76("Rb", 37, 76, 0.000000, 75.935071, 1.36112698317);
static const Atom Rb77("Rb", 37, 77, 0.000000, 76.930407, 1.37896826084);
static const Atom Rb78("Rb", 37, 78, 0.000000, 77.928141, 1.39685252238);
static const Atom Rb79("Rb", 37, 79, 0.000000, 78.923997, 1.41470312099);
static const Atom Rb80("Rb", 37, 80, 0.000000, 79.922519, 1.43260150733);
static const Atom Rb81("Rb", 37, 81, 0.000000, 80.918994, 1.45046320144);
static const Atom Rb82("Rb", 37, 82, 0.000000, 81.918208, 1.4683739918);
static const Atom Rb83("Rb", 37, 83, 0.000000, 82.915112, 1.48624337568);
static const Atom Rb84("Rb", 37, 84, 0.000000, 83.914385, 1.5041552236);
static const Atom Rb85("Rb", 37, 85, 72.170000, 84.911789, 1.52203357531);
static const Atom Rb86("Rb", 37, 86, 0.000000, 85.911167, 1.53994730176);
static const Atom Rb87("Rb", 37, 87, 27.830000, 86.909183, 1.55783662528);
static const Atom Rb88("Rb", 37, 88, 0.000000, 87.911319, 1.57579978317);
static const Atom Rb89("Rb", 37, 89, 0.000000, 88.912280, 1.59374188829);
static const Atom Rb90("Rb", 37, 90, 0.000000, 89.914809, 1.61171209962);
static const Atom Rb91("Rb", 37, 91, 0.000000, 90.916534, 1.62966789935);
static const Atom Rb92("Rb", 37, 92, 0.000000, 91.919725, 1.64764997695);
static const Atom Rb93("Rb", 37, 93, 0.000000, 92.922033, 1.66561622688);
static const Atom Rb94("Rb", 37, 94, 0.000000, 93.926407, 1.68361950962);
static const Atom Rb95("Rb", 37, 95, 0.000000, 94.929319, 1.70159658618);
static const Atom Rb96("Rb", 37, 96, 0.000000, 95.934284, 1.71961046251);
static const Atom Rb97("Rb", 37, 97, 0.000000, 96.937340, 1.73759012026);
static const Atom Rb98("Rb", 37, 98, 0.000000, 97.941700, 1.75559315204);
static const Atom Rb99("Rb", 37, 99, 0.000000, 98.945420, 1.7735847119);
static const Atom Rb100("Rb", 37, 100, 0.000000, 99.949870, 1.79158935693);
static const Atom Rb101("Rb", 37, 101, 0.000000, 100.953200, 1.80957392609);
static const Atom Rb102("Rb", 37, 102, 0.000000, 101.959210, 1.82760653392);
static const Atom Sr("Sr", 38, 0, 0.000000, 87.620000, 2.54);
static const Atom Sr73("Sr", 38, 73, 0.000000, 72.965970, 2.11519703036);
static const Atom Sr74("Sr", 38, 74, 0.000000, 73.956310, 2.14390581374);
static const Atom Sr75("Sr", 38, 75, 0.000000, 74.949920, 2.17270939055);
static const Atom Sr76("Sr", 38, 76, 0.000000, 75.941610, 2.20145730883);
static const Atom Sr77("Sr", 38, 77, 0.000000, 76.937760, 2.23033451723);
static const Atom Sr78("Sr", 38, 78, 0.000000, 77.932179, 2.25916154599);
static const Atom Sr79("Sr", 38, 79, 0.000000, 78.929707, 2.28807870098);
static const Atom Sr80("Sr", 38, 80, 0.000000, 79.924525, 2.31691729628);
static const Atom Sr81("Sr", 38, 81, 0.000000, 80.923213, 2.34586807829);
static const Atom Sr82("Sr", 38, 82, 0.000000, 81.918401, 2.37471739945);
static const Atom Sr83("Sr", 38, 83, 0.000000, 82.917555, 2.40368169025);
static const Atom Sr84("Sr", 38, 84, 0.560000, 83.913425, 2.43255078178);
static const Atom Sr85("Sr", 38, 85, 0.000000, 84.912933, 2.46152533463);
static const Atom Sr86("Sr", 38, 86, 9.860000, 85.909262, 2.49040774362);
static const Atom Sr87("Sr", 38, 87, 7.000000, 86.908879, 2.51938545334);
static const Atom Sr88("Sr", 38, 88, 82.580000, 87.905614, 2.5482796202);
static const Atom Sr89("Sr", 38, 89, 0.000000, 88.907453, 2.57732173438);
static const Atom Sr90("Sr", 38, 90, 0.000000, 89.907738, 2.60631880283);
static const Atom Sr91("Sr", 38, 91, 0.000000, 90.910210, 2.63537929012);
static const Atom Sr92("Sr", 38, 92, 0.000000, 91.911030, 2.66439187628);
static const Atom Sr93("Sr", 38, 93, 0.000000, 92.914022, 2.69346742616);
static const Atom Sr94("Sr", 38, 94, 0.000000, 93.915360, 2.72249502853);
static const Atom Sr95("Sr", 38, 95, 0.000000, 94.919358, 2.75159974115);
static const Atom Sr96("Sr", 38, 96, 0.000000, 95.921680, 2.78065586852);
static const Atom Sr97("Sr", 38, 97, 0.000000, 96.926149, 2.80977423488);
static const Atom Sr98("Sr", 38, 98, 0.000000, 97.928471, 2.83883036225);
static const Atom Sr99("Sr", 38, 99, 0.000000, 98.933320, 2.86795974435);
static const Atom Sr100("Sr", 38, 100, 0.000000, 99.935350, 2.89700740698);
static const Atom Sr101("Sr", 38, 101, 0.000000, 100.940520, 2.9261460945);
static const Atom Sr102("Sr", 38, 102, 0.000000, 101.943020, 2.95520738188);
static const Atom Sr103("Sr", 38, 103, 0.000000, 102.948950, 2.98436810089);
static const Atom Sr104("Sr", 38, 104, 0.000000, 103.952330, 3.01345489843);
static const Atom Y("Y", 39, 0, 0.000000, 88.905850, 4.469);
static const Atom Y77("Y", 39, 77, 0.000000, 76.949620, 3.86800026972);
static const Atom Y78("Y", 39, 78, 0.000000, 77.943500, 3.91795929627);
static const Atom Y79("Y", 39, 79, 0.000000, 78.937350, 3.96791681481);
static const Atom Y80("Y", 39, 80, 0.000000, 79.934340, 4.01803217066);
static const Atom Y81("Y", 39, 81, 0.000000, 80.929130, 4.06803693986);
static const Atom Y82("Y", 39, 82, 0.000000, 81.926790, 4.11818597438);
static const Atom Y83("Y", 39, 83, 0.000000, 82.922350, 4.16822944891);
static const Atom Y84("Y", 39, 84, 0.000000, 83.920390, 4.21839758475);
static const Atom Y85("Y", 39, 85, 0.000000, 84.916427, 4.26846503647);
static const Atom Y86("Y", 39, 86, 0.000000, 85.914888, 4.31865433458);
static const Atom Y87("Y", 39, 87, 0.000000, 86.910878, 4.36871941372);
static const Atom Y88("Y", 39, 88, 0.000000, 87.909503, 4.41891698572);
static const Atom Y89("Y", 39, 89, 100.000000, 88.905848, 4.46899989444);
static const Atom Y90("Y", 39, 90, 0.000000, 89.907151, 4.51933207552);
static const Atom Y91("Y", 39, 91, 0.000000, 90.907303, 4.56960635444);
static const Atom Y92("Y", 39, 92, 0.000000, 91.908947, 4.61995565132);
static const Atom Y93("Y", 39, 93, 0.000000, 92.909582, 4.67025422914);
static const Atom Y94("Y", 39, 94, 0.000000, 93.911594, 4.72062202415);
static const Atom Y95("Y", 39, 95, 0.000000, 94.912824, 4.77095051064);
static const Atom Y96("Y", 39, 96, 0.000000, 95.915898, 4.82137168884);
static const Atom Y97("Y", 39, 97, 0.000000, 96.918131, 4.87175059278);
static const Atom Y98("Y", 39, 98, 0.000000, 97.922220, 4.92222279164);
static const Atom Y99("Y", 39, 99, 0.000000, 98.924635, 4.97261084411);
static const Atom Y100("Y", 39, 100, 0.000000, 99.927760, 5.02303458591);
static const Atom Y101("Y", 39, 101, 0.000000, 100.930310, 5.07342942439);
static const Atom Y102("Y", 39, 102, 0.000000, 101.933560, 5.12385944952);
static const Atom Y103("Y", 39, 103, 0.000000, 102.936940, 5.17429600932);
static const Atom Y104("Y", 39, 104, 0.000000, 103.941450, 5.22478937044);
static const Atom Y105("Y", 39, 105, 0.000000, 104.945090, 5.27523899957);
static const Atom Y106("Y", 39, 106, 0.000000, 105.950220, 5.32576352602);
static const Atom Zr("Zr", 40, 0, 0.000000, 91.224000, 6.506);
static const Atom Zr79("Zr", 40, 79, 0.000000, 78.949160, 5.63057128563);
static const Atom Zr80("Zr", 40, 80, 0.000000, 79.940550, 5.70127618061);
static const Atom Zr81("Zr", 40, 81, 0.000000, 80.936820, 5.77232911208);
static const Atom Zr82("Zr", 40, 82, 0.000000, 81.931090, 5.84323940564);
static const Atom Zr83("Zr", 40, 83, 0.000000, 82.928650, 5.91438433855);
static const Atom Zr84("Zr", 40, 84, 0.000000, 83.923250, 5.98531816737);
static const Atom Zr85("Zr", 40, 85, 0.000000, 84.921470, 6.05651017079);
static const Atom Zr86("Zr", 40, 86, 0.000000, 85.916470, 6.12747252719);
static const Atom Zr87("Zr", 40, 87, 0.000000, 86.914817, 6.19867358811);
static const Atom Zr88("Zr", 40, 88, 0.000000, 87.910226, 6.26966511396);
static const Atom Zr89("Zr", 40, 89, 0.000000, 88.908889, 6.34088871168);
static const Atom Zr90("Zr", 40, 90, 51.450000, 89.904704, 6.41190917162);
static const Atom Zr91("Zr", 40, 91, 11.220000, 90.905645, 6.48329525531);
static const Atom Zr92("Zr", 40, 92, 17.150000, 91.905040, 6.55457106563);
static const Atom Zr93("Zr", 40, 93, 0.000000, 92.906476, 6.62599239513);
static const Atom Zr94("Zr", 40, 94, 17.380000, 93.906316, 6.69729994952);
static const Atom Zr95("Zr", 40, 95, 0.000000, 94.908043, 6.76874206137);
static const Atom Zr96("Zr", 40, 96, 2.800000, 95.908276, 6.84007765123);
static const Atom Zr97("Zr", 40, 97, 0.000000, 96.910951, 6.91158738058);
static const Atom Zr98("Zr", 40, 98, 0.000000, 97.912746, 6.98303434925);
static const Atom Zr99("Zr", 40, 99, 0.000000, 98.916511, 7.05462181625);
static const Atom Zr100("Zr", 40, 100, 0.000000, 99.917760, 7.12602984478);
static const Atom Zr101("Zr", 40, 101, 0.000000, 100.921140, 7.19758985399);
static const Atom Zr102("Zr", 40, 102, 0.000000, 101.922980, 7.26904003201);
static const Atom Zr103("Zr", 40, 103, 0.000000, 102.926600, 7.34061715777);
static const Atom Zr104("Zr", 40, 104, 0.000000, 103.928780, 7.41209158423);
static const Atom Zr105("Zr", 40, 105, 0.000000, 104.933050, 7.48371506731);
static const Atom Zr106("Zr", 40, 106, 0.000000, 105.935910, 7.55523799066);
static const Atom Zr107("Zr", 40, 107, 0.000000, 106.940860, 7.62690997062);
static const Atom Zr108("Zr", 40, 108, 0.000000, 107.944280, 7.69847283259);
static const Atom Nb("Nb", 41, 0, 0.000000, 92.906380, 8.57);
static const Atom Nb81("Nb", 41, 81, 0.000000, 80.949050, 7.46701527387);
static const Atom Nb82("Nb", 41, 82, 0.000000, 81.943130, 7.55871258895);
static const Atom Nb83("Nb", 41, 83, 0.000000, 82.936700, 7.6503628599);
static const Atom Nb84("Nb", 41, 84, 0.000000, 83.933570, 7.74231753406);
static const Atom Nb85("Nb", 41, 85, 0.000000, 84.927910, 7.83403883242);
static const Atom Nb86("Nb", 41, 86, 0.000000, 85.925040, 7.92601748986);
static const Atom Nb87("Nb", 41, 87, 0.000000, 86.920360, 8.01782918676);
static const Atom Nb88("Nb", 41, 88, 0.000000, 87.917960, 8.10985119859);
static const Atom Nb89("Nb", 41, 89, 0.000000, 88.913500, 8.20168318903);
static const Atom Nb90("Nb", 41, 90, 0.000000, 89.911264, 8.29372032879);
static const Atom Nb91("Nb", 41, 91, 0.000000, 90.906991, 8.38556956874);
static const Atom Nb92("Nb", 41, 92, 0.000000, 91.907193, 8.47783161634);
static const Atom Nb93("Nb", 41, 93, 100.000000, 92.906378, 8.56999976939);
static const Atom Nb94("Nb", 41, 94, 0.000000, 93.907284, 8.66232673789);
static const Atom Nb95("Nb", 41, 95, 0.000000, 94.906835, 8.75452878117);
static const Atom Nb96("Nb", 41, 96, 0.000000, 95.908100, 8.8468888466);
static const Atom Nb97("Nb", 41, 97, 0.000000, 96.908097, 8.93913197508);
static const Atom Nb98("Nb", 41, 98, 0.000000, 97.910331, 9.03158143359);
static const Atom Nb99("Nb", 41, 99, 0.000000, 98.911618, 9.12394354683);
static const Atom Nb100("Nb", 41, 100, 0.000000, 99.914181, 9.21642336264);
static const Atom Nb101("Nb", 41, 101, 0.000000, 100.915252, 9.3087655513);
static const Atom Nb102("Nb", 41, 102, 0.000000, 101.918040, 9.40126612187);
static const Atom Nb103("Nb", 41, 103, 0.000000, 102.919140, 9.4936109856);
static const Atom Nb104("Nb", 41, 104, 0.000000, 103.922460, 9.58616062966);
static const Atom Nb105("Nb", 41, 105, 0.000000, 104.923930, 9.67853962344);
static const Atom Nb106("Nb", 41, 106, 0.000000, 105.928190, 9.77117597629);
static const Atom Nb107("Nb", 41, 107, 0.000000, 106.930310, 9.86361492828);
static const Atom Nb108("Nb", 41, 108, 0.000000, 107.935010, 9.95629186822);
static const Atom Nb109("Nb", 41, 109, 0.000000, 108.937630, 10.0487769419);
static const Atom Nb110("Nb", 41, 110, 0.000000, 109.942680, 10.141486167);
static const Atom Mo("Mo", 42, 0, 0.000000, 95.940000, 10.22);
static const Atom Mo83("Mo", 42, 83, 0.000000, 82.948740, 8.83610717949);
static const Atom Mo84("Mo", 42, 84, 0.000000, 83.940090, 8.94171065041);
static const Atom Mo85("Mo", 42, 85, 0.000000, 84.936590, 9.04786272462);
static const Atom Mo86("Mo", 42, 86, 0.000000, 85.930700, 9.15376020429);
static const Atom Mo87("Mo", 42, 87, 0.000000, 86.927330, 9.25992612675);
static const Atom Mo88("Mo", 42, 88, 0.000000, 87.921953, 9.3658782537);
static const Atom Mo89("Mo", 42, 89, 0.000000, 88.919481, 9.47213983552);
static const Atom Mo90("Mo", 42, 90, 0.000000, 89.913936, 9.57807406629);
static const Atom Mo91("Mo", 42, 91, 0.000000, 90.911751, 9.68436622076);
static const Atom Mo92("Mo", 42, 92, 14.840000, 91.906810, 9.79036479258);
static const Atom Mo93("Mo", 42, 93, 0.000000, 92.906812, 9.89688991703);
static const Atom Mo94("Mo", 42, 94, 9.250000, 93.905088, 10.0032311369);
static const Atom Mo95("Mo", 42, 95, 15.920000, 94.905841, 10.1098363574);
static const Atom Mo96("Mo", 42, 96, 16.680000, 95.904679, 10.216237423);
static const Atom Mo97("Mo", 42, 97, 9.550000, 96.906021, 10.3229053014);
static const Atom Mo98("Mo", 42, 98, 24.130000, 97.905408, 10.4293648918);
static const Atom Mo99("Mo", 42, 99, 0.000000, 98.907712, 10.5361352153);
static const Atom Mo100("Mo", 42, 100, 9.630000, 99.907477, 10.6426351359);
static const Atom Mo101("Mo", 42, 101, 0.000000, 100.910347, 10.7494657738);
static const Atom Mo102("Mo", 42, 102, 0.000000, 101.910297, 10.855985359);
static const Atom Mo103("Mo", 42, 103, 0.000000, 102.913200, 10.9628195122);
static const Atom Mo104("Mo", 42, 104, 0.000000, 103.913760, 11.0694040775);
static const Atom Mo105("Mo", 42, 105, 0.000000, 104.916970, 11.1762709339);
static const Atom Mo106("Mo", 42, 106, 0.000000, 105.918134, 11.2829198403);
static const Atom Mo107("Mo", 42, 107, 0.000000, 106.921690, 11.3898235543);
static const Atom Mo108("Mo", 42, 108, 0.000000, 107.923580, 11.4965497978);
static const Atom Mo109("Mo", 42, 109, 0.000000, 108.927810, 11.6035253096);
static const Atom Mo110("Mo", 42, 110, 0.000000, 109.929730, 11.7102547488);
static const Atom Mo111("Mo", 42, 111, 0.000000, 110.934510, 11.8172888493);
static const Atom Mo112("Mo", 42, 112, 0.000000, 111.936840, 11.9240619637);
static const Atom Mo113("Mo", 42, 113, 0.000000, 112.942030, 12.0311397394);
static const Atom Tc("Tc", 43, 0, 0.000000, 98.000000, 11.5);
static const Atom Tc85("Tc", 43, 85, 0.000000, 84.948940, 9.96849806122);
static const Atom Tc86("Tc", 43, 86, 0.000000, 85.942880, 10.0851338776);
static const Atom Tc87("Tc", 43, 87, 0.000000, 86.936530, 10.2017356633);
static const Atom Tc88("Tc", 43, 88, 0.000000, 87.932830, 10.3186484184);
static const Atom Tc89("Tc", 43, 89, 0.000000, 88.927540, 10.4353745918);
static const Atom Tc90("Tc", 43, 90, 0.000000, 89.923560, 10.5522544898);
static const Atom Tc91("Tc", 43, 91, 0.000000, 90.918430, 10.6689994388);
static const Atom Tc92("Tc", 43, 92, 0.000000, 91.915260, 10.7859743878);
static const Atom Tc93("Tc", 43, 93, 0.000000, 92.910248, 10.9027331837);
static const Atom Tc94("Tc", 43, 94, 0.000000, 93.909656, 11.0200106531);
static const Atom Tc95("Tc", 43, 95, 0.000000, 94.907656, 11.137122898);
static const Atom Tc96("Tc", 43, 96, 0.000000, 95.907871, 11.2544950663);
static const Atom Tc97("Tc", 43, 97, 0.000000, 96.906365, 11.3716652806);
static const Atom Tc98("Tc", 43, 98, 0.000000, 97.907216, 11.4891120816);
static const Atom Tc99("Tc", 43, 99, 0.000000, 98.906255, 11.6063462031);
static const Atom Tc100("Tc", 43, 100, 0.000000, 99.907658, 11.7238577796);
static const Atom Tc101("Tc", 43, 101, 0.000000, 100.907314, 11.841164398);
static const Atom Tc102("Tc", 43, 102, 0.000000, 101.909213, 11.9587341786);
static const Atom Tc103("Tc", 43, 103, 0.000000, 102.909179, 12.0760771276);
static const Atom Tc104("Tc", 43, 104, 0.000000, 103.911440, 12.1936893878);
static const Atom Tc105("Tc", 43, 105, 0.000000, 104.911660, 12.3110621429);
static const Atom Tc106("Tc", 43, 106, 0.000000, 105.914355, 12.4287253316);
static const Atom Tc107("Tc", 43, 107, 0.000000, 106.915080, 12.5461573469);
static const Atom Tc108("Tc", 43, 108, 0.000000, 107.918480, 12.6639032653);
static const Atom Tc109("Tc", 43, 109, 0.000000, 108.919630, 12.7813851531);
static const Atom Tc110("Tc", 43, 110, 0.000000, 109.923390, 12.8991733163);
static const Atom Tc111("Tc", 43, 111, 0.000000, 110.925050, 13.016715051);
static const Atom Tc112("Tc", 43, 112, 0.000000, 111.929240, 13.1345536735);
static const Atom Tc113("Tc", 43, 113, 0.000000, 112.931330, 13.2521458673);
static const Atom Tc114("Tc", 43, 114, 0.000000, 113.935880, 13.3700267347);
static const Atom Tc115("Tc", 43, 115, 0.000000, 114.938280, 13.4876553061);
static const Atom Ru("Ru", 44, 0, 0.000000, 101.070000, 12.41);
static const Atom Ru87("Ru", 44, 87, 0.000000, 86.949180, 10.6761583437);
static const Atom Ru88("Ru", 44, 88, 0.000000, 87.940420, 10.7978689245);
static const Atom Ru89("Ru", 44, 89, 0.000000, 88.936110, 10.9201259038);
static const Atom Ru90("Ru", 44, 90, 0.000000, 89.929780, 11.0421348551);
static const Atom Ru91("Ru", 44, 91, 0.000000, 90.926380, 11.1645035698);
static const Atom Ru92("Ru", 44, 92, 0.000000, 91.920120, 11.2865211161);
static const Atom Ru93("Ru", 44, 93, 0.000000, 92.917050, 11.4089303503);
static const Atom Ru94("Ru", 44, 94, 0.000000, 93.911360, 11.5310178846);
static const Atom Ru95("Ru", 44, 95, 0.000000, 94.910413, 11.6536877939);
static const Atom Ru96("Ru", 44, 96, 5.540000, 95.907598, 11.7761283386);
static const Atom Ru97("Ru", 44, 97, 0.000000, 96.907555, 11.8989092466);
static const Atom Ru98("Ru", 44, 98, 1.870000, 97.905287, 12.0214169553);
static const Atom Ru99("Ru", 44, 99, 12.760000, 98.905939, 12.1442832365);
static const Atom Ru100("Ru", 44, 100, 12.600000, 99.904220, 12.2668582812);
static const Atom Ru101("Ru", 44, 101, 17.060000, 100.905582, 12.3898117651);
static const Atom Ru102("Ru", 44, 102, 31.550000, 101.904349, 12.5124465944);
static const Atom Ru103("Ru", 44, 103, 0.000000, 102.906324, 12.6354751867);
static const Atom Ru104("Ru", 44, 104, 18.620000, 103.905430, 12.7581516404);
static const Atom Ru105("Ru", 44, 105, 0.000000, 104.907750, 12.8812226922);
static const Atom Ru106("Ru", 44, 106, 0.000000, 105.907327, 13.0039569414);
static const Atom Ru107("Ru", 44, 107, 0.000000, 106.909910, 13.1270602859);
static const Atom Ru108("Ru", 44, 108, 0.000000, 107.910190, 13.2498808539);
static const Atom Ru109("Ru", 44, 109, 0.000000, 108.913200, 13.3730366281);
static const Atom Ru110("Ru", 44, 110, 0.000000, 109.913970, 13.4959173612);
static const Atom Ru111("Ru", 44, 111, 0.000000, 110.917560, 13.6191443514);
static const Atom Ru112("Ru", 44, 112, 0.000000, 111.918550, 13.7420520976);
static const Atom Ru113("Ru", 44, 113, 0.000000, 112.922540, 13.8653282022);
static const Atom Ru114("Ru", 44, 114, 0.000000, 113.924000, 13.9882936579);
static const Atom Ru115("Ru", 44, 115, 0.000000, 114.928310, 14.1116090541);
static const Atom Ru116("Ru", 44, 116, 0.000000, 115.930160, 14.2346223964);
static const Atom Ru117("Ru", 44, 117, 0.000000, 116.934790, 14.3579770842);
static const Atom Ru118("Ru", 44, 118, 0.000000, 117.937030, 14.4810383131);
static const Atom Rh("Rh", 45, 0, 0.000000, 102.905500, 12.41);
static const Atom Rh89("Rh", 45, 89, 0.000000, 88.949380, 10.7269466238);
static const Atom Rh90("Rh", 45, 90, 0.000000, 89.942870, 10.8467576242);
static const Atom Rh91("Rh", 45, 91, 0.000000, 90.936550, 10.9665915379);
static const Atom Rh92("Rh", 45, 92, 0.000000, 91.931980, 11.0866364946);
static const Atom Rh93("Rh", 45, 93, 0.000000, 92.925740, 11.206480056);
static const Atom Rh94("Rh", 45, 94, 0.000000, 93.921700, 11.3265889287);
static const Atom Rh95("Rh", 45, 95, 0.000000, 94.915900, 11.4464855523);
static const Atom Rh96("Rh", 45, 96, 0.000000, 95.914518, 11.5669149694);
static const Atom Rh97("Rh", 45, 97, 0.000000, 96.911340, 11.6871277959);
static const Atom Rh98("Rh", 45, 98, 0.000000, 97.910716, 11.8076486248);
static const Atom Rh99("Rh", 45, 99, 0.000000, 98.908132, 11.9279330854);
static const Atom Rh100("Rh", 45, 100, 0.000000, 99.908117, 12.0485273573);
static const Atom Rh101("Rh", 45, 101, 0.000000, 100.906164, 12.1688879141);
static const Atom Rh102("Rh", 45, 102, 0.000000, 101.906843, 12.2895658797);
static const Atom Rh103("Rh", 45, 103, 100.000000, 102.905504, 12.4100004824);
static const Atom Rh104("Rh", 45, 104, 0.000000, 103.906655, 12.5307353693);
static const Atom Rh105("Rh", 45, 105, 0.000000, 104.905692, 12.6512153162);
static const Atom Rh106("Rh", 45, 106, 0.000000, 105.907285, 12.7720035066);
static const Atom Rh107("Rh", 45, 107, 0.000000, 106.906751, 12.8925351892);
static const Atom Rh108("Rh", 45, 108, 0.000000, 107.908730, 13.0133699297);
static const Atom Rh109("Rh", 45, 109, 0.000000, 108.908736, 13.1339667341);
static const Atom Rh110("Rh", 45, 110, 0.000000, 109.910950, 13.2548298147);
static const Atom Rh111("Rh", 45, 111, 0.000000, 110.911660, 13.3755115188);
static const Atom Rh112("Rh", 45, 112, 0.000000, 111.914610, 13.4964633581);
static const Atom Rh113("Rh", 45, 113, 0.000000, 112.915420, 13.6171571218);
static const Atom Rh114("Rh", 45, 114, 0.000000, 113.918850, 13.7381668473);
static const Atom Rh115("Rh", 45, 115, 0.000000, 114.920120, 13.8589160851);
static const Atom Rh116("Rh", 45, 116, 0.000000, 115.923710, 13.9799451059);
static const Atom Rh117("Rh", 45, 117, 0.000000, 116.925350, 14.1007389644);
static const Atom Rh118("Rh", 45, 118, 0.000000, 117.929430, 14.2218270773);
static const Atom Rh119("Rh", 45, 119, 0.000000, 118.931360, 14.3426559086);
static const Atom Rh120("Rh", 45, 120, 0.000000, 119.935780, 14.4637850241);
static const Atom Rh121("Rh", 45, 121, 0.000000, 120.938080, 14.584658476);
static const Atom Pd("Pd", 46, 0, 0.000000, 106.420000, 12.02);
static const Atom Pd91("Pd", 46, 91, 0.000000, 90.949480, 10.2726249727);
static const Atom Pd92("Pd", 46, 92, 0.000000, 91.940420, 10.3845503514);
static const Atom Pd93("Pd", 46, 93, 0.000000, 92.935910, 10.4969896467);
static const Atom Pd94("Pd", 46, 94, 0.000000, 93.928770, 10.6091318869);
static const Atom Pd95("Pd", 46, 95, 0.000000, 94.924690, 10.72161975);
static const Atom Pd96("Pd", 46, 96, 0.000000, 95.918220, 10.8338376659);
static const Atom Pd97("Pd", 46, 97, 0.000000, 96.916480, 10.946589829);
static const Atom Pd98("Pd", 46, 98, 0.000000, 97.912721, 11.0591139487);
static const Atom Pd99("Pd", 46, 99, 0.000000, 98.911768, 11.1719550024);
static const Atom Pd100("Pd", 46, 100, 0.000000, 99.908505, 11.2845351447);
static const Atom Pd101("Pd", 46, 101, 0.000000, 100.908289, 11.3974594416);
static const Atom Pd102("Pd", 46, 102, 1.020000, 101.905608, 11.5101053201);
static const Atom Pd103("Pd", 46, 103, 0.000000, 102.906087, 11.6231081163);
static const Atom Pd104("Pd", 46, 104, 11.140000, 103.904035, 11.7358250395);
static const Atom Pd105("Pd", 46, 105, 22.330000, 104.905084, 11.8488922165);
static const Atom Pd106("Pd", 46, 106, 27.330000, 105.903483, 11.9616600795);
static const Atom Pd107("Pd", 46, 107, 0.000000, 106.905128, 12.074794574);
static const Atom Pd108("Pd", 46, 108, 26.460000, 107.903894, 12.1876038891);
static const Atom Pd109("Pd", 46, 109, 0.000000, 108.905954, 12.3007852573);
static const Atom Pd110("Pd", 46, 110, 11.720000, 109.905152, 12.4136433663);
static const Atom Pd111("Pd", 46, 111, 0.000000, 110.907640, 12.5268730765);
static const Atom Pd112("Pd", 46, 112, 0.000000, 111.907313, 12.6397848361);
static const Atom Pd113("Pd", 46, 113, 0.000000, 112.910150, 12.7530539654);
static const Atom Pd114("Pd", 46, 114, 0.000000, 113.910365, 12.8660269432);
static const Atom Pd115("Pd", 46, 115, 0.000000, 114.913680, 12.979350062);
static const Atom Pd116("Pd", 46, 116, 0.000000, 115.914160, 13.0923529712);
static const Atom Pd117("Pd", 46, 117, 0.000000, 116.917840, 13.2057173163);
static const Atom Pd118("Pd", 46, 118, 0.000000, 117.918980, 13.3187947717);
static const Atom Pd119("Pd", 46, 119, 0.000000, 118.922680, 13.4321613757);
static const Atom Pd120("Pd", 46, 120, 0.000000, 119.924030, 13.5452625503);
static const Atom Pd121("Pd", 46, 121, 0.000000, 120.928180, 13.6586799812);
static const Atom Pd122("Pd", 46, 122, 0.000000, 121.929800, 13.7718116519);
static const Atom Pd123("Pd", 46, 123, 0.000000, 122.934260, 13.885264097);
static const Atom Ag("Ag", 47, 0, 0.000000, 107.868200, 10.5);
static const Atom Ag94("Ag", 47, 94, 0.000000, 93.942780, 9.14448549248);
static const Atom Ag95("Ag", 47, 95, 0.000000, 94.935480, 9.24111591739);
static const Atom Ag96("Ag", 47, 96, 0.000000, 95.930680, 9.33798969483);
static const Atom Ag97("Ag", 47, 97, 0.000000, 96.924000, 9.43468047117);
static const Atom Ag98("Ag", 47, 98, 0.000000, 97.921760, 9.53180344161);
static const Atom Ag99("Ag", 47, 99, 0.000000, 98.917600, 9.6287395173);
static const Atom Ag100("Ag", 47, 100, 0.000000, 99.916070, 9.72593159986);
static const Atom Ag101("Ag", 47, 101, 0.000000, 100.912800, 9.82295430905);
static const Atom Ag102("Ag", 47, 102, 0.000000, 101.912000, 9.92021745056);
static const Atom Ag103("Ag", 47, 103, 0.000000, 102.908972, 10.0172637163);
static const Atom Ag104("Ag", 47, 104, 0.000000, 103.908628, 10.1145712453);
static const Atom Ag105("Ag", 47, 105, 0.000000, 104.906528, 10.2117078435);
static const Atom Ag106("Ag", 47, 106, 0.000000, 105.906666, 10.3090622908);
static const Atom Ag107("Ag", 47, 107, 51.839000, 106.905093, 10.4062501877);
static const Atom Ag108("Ag", 47, 108, 0.000000, 107.905954, 10.5036750127);
static const Atom Ag109("Ag", 47, 109, 48.161000, 108.904756, 10.6008994124);
static const Atom Ag110("Ag", 47, 110, 0.000000, 109.906110, 10.6983722265);
static const Atom Ag111("Ag", 47, 111, 0.000000, 110.905295, 10.7956339079);
static const Atom Ag112("Ag", 47, 112, 0.000000, 111.907004, 10.893141278);
static const Atom Ag113("Ag", 47, 113, 0.000000, 112.906566, 10.9904396569);
static const Atom Ag114("Ag", 47, 114, 0.000000, 113.908808, 11.0879989098);
static const Atom Ag115("Ag", 47, 115, 0.000000, 114.908760, 11.1853352517);
static const Atom Ag116("Ag", 47, 116, 0.000000, 115.911360, 11.2829293527);
static const Atom Ag117("Ag", 47, 117, 0.000000, 116.911680, 11.3803015161);
static const Atom Ag118("Ag", 47, 118, 0.000000, 117.914580, 11.4779248194);
static const Atom Ag119("Ag", 47, 119, 0.000000, 118.915670, 11.5753719354);
static const Atom Ag120("Ag", 47, 120, 0.000000, 119.918790, 11.6730166537);
static const Atom Ag121("Ag", 47, 121, 0.000000, 120.919850, 11.7704608494);
static const Atom Ag122("Ag", 47, 122, 0.000000, 121.923320, 11.8681396371);
static const Atom Ag123("Ag", 47, 123, 0.000000, 122.924900, 11.9656344502);
static const Atom Ag124("Ag", 47, 124, 0.000000, 123.928530, 12.0633288124);
static const Atom Ag125("Ag", 47, 125, 0.000000, 124.930540, 12.1608654821);
static const Atom Ag126("Ag", 47, 126, 0.000000, 125.934500, 12.2585919669);
static const Atom Ag127("Ag", 47, 127, 0.000000, 126.936880, 12.3561646528);
static const Atom Cd("Cd", 48, 0, 0.000000, 112.411000, 8.65);
static const Atom Cd128("Cd", 48, 128, 0.000000, 127.927760, 9.84401103095);
static const Atom Cd129("Cd", 48, 129, 0.000000, 128.932260, 9.92130706959);
static const Atom Cd130("Cd", 48, 130, 0.000000, 129.933980, 9.99838918789);
static const Atom Cd96("Cd", 48, 96, 0.000000, 95.939770, 7.38254272714);
static const Atom Cd97("Cd", 48, 97, 0.000000, 96.934940, 7.45912082447);
static const Atom Cd98("Cd", 48, 98, 0.000000, 97.927580, 7.53550423891);
static const Atom Cd99("Cd", 48, 99, 0.000000, 98.925010, 7.61225624272);
static const Atom Cd100("Cd", 48, 100, 0.000000, 99.920230, 7.68883818754);
static const Atom Cd101("Cd", 48, 101, 0.000000, 100.918680, 7.76566868011);
static const Atom Cd102("Cd", 48, 102, 0.000000, 101.914780, 7.84231834073);
static const Atom Cd103("Cd", 48, 103, 0.000000, 102.913419, 7.9191633768);
static const Atom Cd104("Cd", 48, 104, 0.000000, 103.909848, 7.9958383539);
static const Atom Cd105("Cd", 48, 105, 0.000000, 104.909468, 8.07275887769);
static const Atom Cd106("Cd", 48, 106, 1.250000, 105.906458, 8.1494770236);
static const Atom Cd107("Cd", 48, 107, 0.000000, 106.906614, 8.22643879247);
static const Atom Cd108("Cd", 48, 108, 0.890000, 107.904183, 8.30320149229);
static const Atom Cd109("Cd", 48, 109, 0.000000, 108.904986, 8.38021304766);
static const Atom Cd110("Cd", 48, 110, 12.490000, 109.903006, 8.45701045182);
static const Atom Cd111("Cd", 48, 111, 12.800000, 110.904182, 8.53405070945);
static const Atom Cd112("Cd", 48, 112, 24.130000, 111.902757, 8.61089083613);
static const Atom Cd113("Cd", 48, 113, 12.220000, 112.904401, 8.68796708316);
static const Atom Cd114("Cd", 48, 114, 28.730000, 113.903358, 8.76483660465);
static const Atom Cd115("Cd", 48, 115, 0.000000, 114.905431, 8.84194587852);
static const Atom Cd116("Cd", 48, 116, 7.490000, 115.904755, 8.91884362518);
static const Atom Cd117("Cd", 48, 117, 0.000000, 116.907218, 8.99598291715);
static const Atom Cd118("Cd", 48, 118, 0.000000, 117.906914, 9.07290928913);
static const Atom Cd119("Cd", 48, 119, 0.000000, 118.909920, 9.15009036482);
static const Atom Cd120("Cd", 48, 120, 0.000000, 119.909851, 9.22703481999);
static const Atom Cd121("Cd", 48, 121, 0.000000, 120.912980, 9.30422536051);
static const Atom Cd122("Cd", 48, 122, 0.000000, 121.913500, 9.38121513909);
static const Atom Cd123("Cd", 48, 123, 0.000000, 122.917000, 9.45843422797);
static const Atom Cd124("Cd", 48, 124, 0.000000, 123.917650, 9.53543401002);
static const Atom Cd125("Cd", 48, 125, 0.000000, 124.921250, 9.61266079387);
static const Atom Cd126("Cd", 48, 126, 0.000000, 125.922350, 9.68969520332);
static const Atom Cd127("Cd", 48, 127, 0.000000, 126.926430, 9.76695892306);
static const Atom In("In", 49, 0, 0.000000, 114.818000, 7.31);
static const Atom In128("In", 49, 128, 0.000000, 127.920170, 8.14416243707);
static const Atom In129("In", 49, 129, 0.000000, 128.921660, 8.2079232751);
static const Atom In130("In", 49, 130, 0.000000, 129.924850, 8.27179234528);
static const Atom In131("In", 49, 131, 0.000000, 130.926770, 8.33558055967);
static const Atom In132("In", 49, 132, 0.000000, 131.932920, 8.39963808114);
static const Atom In133("In", 49, 133, 0.000000, 132.938340, 8.46364912644);
static const Atom In134("In", 49, 134, 0.000000, 133.944660, 8.52771747113);
static const Atom In98("In", 49, 98, 0.000000, 97.942240, 6.23558827362);
static const Atom In99("In", 49, 99, 0.000000, 98.934610, 6.29876847794);
static const Atom In100("In", 49, 100, 0.000000, 99.931150, 6.36221416938);
static const Atom In101("In", 49, 101, 0.000000, 100.926560, 6.42558791827);
static const Atom In102("In", 49, 102, 0.000000, 101.924710, 6.48913611193);
static const Atom In103("In", 49, 103, 0.000000, 102.919914, 6.55249674563);
static const Atom In104("In", 49, 104, 0.000000, 103.918340, 6.6160625111);
static const Atom In105("In", 49, 105, 0.000000, 104.914673, 6.67949502369);
static const Atom In106("In", 49, 106, 0.000000, 105.913461, 6.74308383625);
static const Atom In107("In", 49, 107, 0.000000, 106.910292, 6.80654805449);
static const Atom In108("In", 49, 108, 0.000000, 107.909720, 6.87017761327);
static const Atom In109("In", 49, 109, 0.000000, 108.907154, 6.93368022209);
static const Atom In110("In", 49, 110, 0.000000, 109.907169, 6.9973471528);
static const Atom In111("In", 49, 111, 0.000000, 110.905111, 7.06088210394);
static const Atom In112("In", 49, 112, 0.000000, 111.905533, 7.1245749467);
static const Atom In113("In", 49, 113, 4.290000, 112.904061, 7.1881472061);
static const Atom In114("In", 49, 114, 0.000000, 113.904917, 7.25186767989);
static const Atom In115("In", 49, 115, 95.710000, 114.903878, 7.31546750666);
static const Atom In116("In", 49, 116, 0.000000, 115.905260, 7.37922146876);
static const Atom In117("In", 49, 117, 0.000000, 116.904516, 7.44284007699);
static const Atom In118("In", 49, 118, 0.000000, 117.906355, 7.50662313444);
static const Atom In119("In", 49, 119, 0.000000, 118.905846, 7.57025670418);
static const Atom In120("In", 49, 120, 0.000000, 119.907960, 7.63405726977);
static const Atom In121("In", 49, 121, 0.000000, 120.907849, 7.69771617856);
static const Atom In122("In", 49, 122, 0.000000, 121.910280, 7.76153692627);
static const Atom In123("In", 49, 123, 0.000000, 122.910439, 7.82521302487);
static const Atom In124("In", 49, 124, 0.000000, 123.913180, 7.88905350903);
static const Atom In125("In", 49, 125, 0.000000, 124.913600, 7.95274622446);
static const Atom In126("In", 49, 126, 0.000000, 125.916460, 8.01659428487);
static const Atom In127("In", 49, 127, 0.000000, 126.917340, 8.08031628664);
static const Atom Sn("Sn", 50, 0, 0.000000, 118.710000, 7.31);
static const Atom Sn128("Sn", 50, 128, 0.000000, 127.910535, 7.8765564051);
static const Atom Sn129("Sn", 50, 129, 0.000000, 128.913440, 7.93831392806);
static const Atom Sn130("Sn", 50, 130, 0.000000, 129.913850, 7.99991781232);
static const Atom Sn131("Sn", 50, 131, 0.000000, 130.916920, 8.06168549575);
static const Atom Sn132("Sn", 50, 132, 0.000000, 131.917744, 8.12331487356);
static const Atom Sn133("Sn", 50, 133, 0.000000, 132.923810, 8.18526704658);
static const Atom Sn134("Sn", 50, 134, 0.000000, 133.928460, 8.24713202426);
static const Atom Sn135("Sn", 50, 135, 0.000000, 134.934730, 8.30909675933);
static const Atom Sn136("Sn", 50, 136, 0.000000, 135.939340, 8.37095927386);
static const Atom Sn137("Sn", 50, 137, 0.000000, 136.945790, 8.43293509308);
static const Atom Sn100("Sn", 50, 100, 0.000000, 99.938950, 6.15410432567);
static const Atom Sn101("Sn", 50, 101, 0.000000, 100.936060, 6.21550500042);
static const Atom Sn102("Sn", 50, 102, 0.000000, 101.930490, 6.27674064443);
static const Atom Sn103("Sn", 50, 103, 0.000000, 102.928130, 6.33817395586);
static const Atom Sn104("Sn", 50, 104, 0.000000, 103.923190, 6.39944839441);
static const Atom Sn105("Sn", 50, 105, 0.000000, 104.921390, 6.46091618987);
static const Atom Sn106("Sn", 50, 106, 0.000000, 105.916880, 6.52221710724);
static const Atom Sn107("Sn", 50, 107, 0.000000, 106.915670, 6.5837212341);
static const Atom Sn108("Sn", 50, 108, 0.000000, 107.911970, 6.64507203016);
static const Atom Sn109("Sn", 50, 109, 0.000000, 108.911287, 6.70660860896);
static const Atom Sn110("Sn", 50, 110, 0.000000, 109.907853, 6.76797578494);
static const Atom Sn111("Sn", 50, 111, 0.000000, 110.907735, 6.82954715567);
static const Atom Sn112("Sn", 50, 112, 0.970000, 111.904821, 6.89094635254);
static const Atom Sn113("Sn", 50, 113, 0.000000, 112.905173, 6.95254666523);
static const Atom Sn114("Sn", 50, 114, 0.660000, 113.902782, 7.01397806773);
static const Atom Sn115("Sn", 50, 115, 0.340000, 114.903346, 7.07559143509);
static const Atom Sn116("Sn", 50, 116, 14.540000, 115.901744, 7.13707142313);
static const Atom Sn117("Sn", 50, 117, 7.680000, 116.902954, 7.1987245703);
static const Atom Sn118("Sn", 50, 118, 24.220000, 117.901606, 7.26022019931);
static const Atom Sn119("Sn", 50, 119, 8.590000, 118.903309, 7.32190370474);
static const Atom Sn120("Sn", 50, 120, 32.580000, 119.902197, 7.38341384168);
static const Atom Sn121("Sn", 50, 121, 0.000000, 120.904237, 7.44511811759);
static const Atom Sn122("Sn", 50, 122, 4.630000, 121.903440, 7.50664768875);
static const Atom Sn123("Sn", 50, 123, 0.000000, 122.905722, 7.56836683589);
static const Atom Sn124("Sn", 50, 124, 5.790000, 123.905275, 7.62991792878);
static const Atom Sn125("Sn", 50, 125, 0.000000, 124.907785, 7.69165114665);
static const Atom Sn126("Sn", 50, 126, 0.000000, 125.907654, 7.75322172302);
static const Atom Sn127("Sn", 50, 127, 0.000000, 126.910351, 7.81496643762);
static const Atom Sb("Sb", 51, 0, 0.000000, 121.760000, 6.691);
static const Atom Sb128("Sb", 51, 128, 0.000000, 127.909167, 7.02891127133);
static const Atom Sb129("Sb", 51, 129, 0.000000, 128.909150, 7.08386270245);
static const Atom Sb130("Sb", 51, 130, 0.000000, 129.911546, 7.13894673362);
static const Atom Sb131("Sb", 51, 131, 0.000000, 130.911950, 7.19392129969);
static const Atom Sb132("Sb", 51, 132, 0.000000, 131.914413, 7.24900901267);
static const Atom Sb133("Sb", 51, 133, 0.000000, 132.915240, 7.30400682359);
static const Atom Sb134("Sb", 51, 134, 0.000000, 133.920550, 7.35925098596);
static const Atom Sb135("Sb", 51, 135, 0.000000, 134.925170, 7.41445723119);
static const Atom Sb136("Sb", 51, 136, 0.000000, 135.930660, 7.46971128499);
static const Atom Sb137("Sb", 51, 137, 0.000000, 136.935310, 7.52491917879);
static const Atom Sb138("Sb", 51, 138, 0.000000, 137.940960, 7.58018202497);
static const Atom Sb139("Sb", 51, 139, 0.000000, 138.945710, 7.63539541401);
static const Atom Sb103("Sb", 51, 103, 0.000000, 102.940120, 5.65680307917);
static const Atom Sb104("Sb", 51, 104, 0.000000, 103.936290, 5.71154497692);
static const Atom Sb105("Sb", 51, 105, 0.000000, 104.931530, 5.76623576897);
static const Atom Sb106("Sb", 51, 106, 0.000000, 105.928760, 5.82103591623);
static const Atom Sb107("Sb", 51, 107, 0.000000, 106.924150, 5.87573495113);
static const Atom Sb108("Sb", 51, 108, 0.000000, 107.922160, 5.93057796124);
static const Atom Sb109("Sb", 51, 109, 0.000000, 108.918136, 5.98530919823);
static const Atom Sb110("Sb", 51, 110, 0.000000, 109.916760, 6.04018594908);
static const Atom Sb111("Sb", 51, 111, 0.000000, 110.913210, 6.09494323349);
static const Atom Sb112("Sb", 51, 112, 0.000000, 111.912395, 6.14985081262);
static const Atom Sb113("Sb", 51, 113, 0.000000, 112.909378, 6.20463738665);
static const Atom Sb114("Sb", 51, 114, 0.000000, 113.909100, 6.2595744752);
static const Atom Sb115("Sb", 51, 115, 0.000000, 114.906599, 6.31438940464);
static const Atom Sb116("Sb", 51, 116, 0.000000, 115.906797, 6.36935265052);
static const Atom Sb117("Sb", 51, 117, 0.000000, 116.904840, 6.42419747405);
static const Atom Sb118("Sb", 51, 118, 0.000000, 117.905532, 6.47918786639);
static const Atom Sb119("Sb", 51, 119, 0.000000, 118.903946, 6.53405307725);
static const Atom Sb120("Sb", 51, 120, 0.000000, 119.905074, 6.58906742883);
static const Atom Sb121("Sb", 51, 121, 57.210000, 120.903818, 6.64395077397);
static const Atom Sb122("Sb", 51, 122, 0.000000, 121.905175, 6.69897773161);
static const Atom Sb123("Sb", 51, 123, 42.790000, 122.904216, 6.75387735914);
static const Atom Sb124("Sb", 51, 124, 0.000000, 123.905937, 6.80892434143);
static const Atom Sb125("Sb", 51, 125, 0.000000, 124.905248, 6.86383881708);
static const Atom Sb126("Sb", 51, 126, 0.000000, 125.907250, 6.91890119703);
static const Atom Sb127("Sb", 51, 127, 0.000000, 126.906915, 6.97383515329);
static const Atom Te("Te", 52, 0, 0.000000, 127.600000, 6.24);
static const Atom Te128("Te", 52, 128, 31.740000, 127.904461, 6.25488902144);
static const Atom Te129("Te", 52, 129, 0.000000, 128.906596, 6.30389623072);
static const Atom Te130("Te", 52, 130, 34.080000, 129.906223, 6.3527808015);
static const Atom Te131("Te", 52, 131, 0.000000, 130.908522, 6.4017960553);
static const Atom Te132("Te", 52, 132, 0.000000, 131.908524, 6.45069897931);
static const Atom Te133("Te", 52, 133, 0.000000, 132.910940, 6.49971994984);
static const Atom Te134("Te", 52, 134, 0.000000, 133.911540, 6.54865211285);
static const Atom Te135("Te", 52, 135, 0.000000, 134.916450, 6.59779504702);
static const Atom Te136("Te", 52, 136, 0.000000, 135.920100, 6.64687636364);
static const Atom Te137("Te", 52, 137, 0.000000, 136.925320, 6.69603445768);
static const Atom Te138("Te", 52, 138, 0.000000, 137.929220, 6.745128);
static const Atom Te139("Te", 52, 139, 0.000000, 138.934730, 6.79430027586);
static const Atom Te140("Te", 52, 140, 0.000000, 139.938700, 6.84339724138);
static const Atom Te141("Te", 52, 141, 0.000000, 140.944390, 6.89257831975);
static const Atom Te142("Te", 52, 142, 0.000000, 141.948500, 6.94168213166);
static const Atom Te106("Te", 52, 106, 0.000000, 105.937700, 5.18065241379);
static const Atom Te107("Te", 52, 107, 0.000000, 106.935040, 5.22942515361);
static const Atom Te108("Te", 52, 108, 0.000000, 107.929490, 5.27805656426);
static const Atom Te109("Te", 52, 109, 0.000000, 108.927460, 5.32686011285);
static const Atom Te110("Te", 52, 110, 0.000000, 109.922410, 5.37551597492);
static const Atom Te111("Te", 52, 111, 0.000000, 110.921120, 5.4243557116);
static const Atom Te112("Te", 52, 112, 0.000000, 111.917060, 5.47305998746);
static const Atom Te113("Te", 52, 113, 0.000000, 112.915930, 5.52190754859);
static const Atom Te114("Te", 52, 114, 0.000000, 113.912060, 5.57062111599);
static const Atom Te115("Te", 52, 115, 0.000000, 114.911580, 5.61950046395);
static const Atom Te116("Te", 52, 116, 0.000000, 115.908420, 5.66824875235);
static const Atom Te117("Te", 52, 117, 0.000000, 116.908634, 5.71716203887);
static const Atom Te118("Te", 52, 118, 0.000000, 117.905825, 5.76592749216);
static const Atom Te119("Te", 52, 119, 0.000000, 118.906408, 5.81485882382);
static const Atom Te120("Te", 52, 120, 0.090000, 119.904020, 5.8636448652);
static const Atom Te121("Te", 52, 121, 0.000000, 120.904930, 5.91259218809);
static const Atom Te122("Te", 52, 122, 2.550000, 121.903047, 5.96140293028);
static const Atom Te123("Te", 52, 123, 0.890000, 122.904273, 6.01036570157);
static const Atom Te124("Te", 52, 124, 4.740000, 123.902820, 6.05919744263);
static const Atom Te125("Te", 52, 125, 7.070000, 124.904425, 6.10817876276);
static const Atom Te126("Te", 52, 126, 18.840000, 125.903306, 6.15702685204);
static const Atom Te127("Te", 52, 127, 0.000000, 126.905217, 6.2060231511);
static const Atom I("I", 53, 0, 0.000000, 126.904470, 4.93);
static const Atom I128("I", 53, 128, 0.000000, 127.905805, 4.96889998162);
static const Atom I129("I", 53, 129, 0.000000, 128.904987, 5.00771632323);
static const Atom I130("I", 53, 130, 0.000000, 129.906674, 5.04662997939);
static const Atom I131("I", 53, 131, 0.000000, 130.906124, 5.08545674007);
static const Atom I132("I", 53, 132, 0.000000, 131.907995, 5.1243775365);
static const Atom I133("I", 53, 133, 0.000000, 132.907806, 5.16321831359);
static const Atom I134("I", 53, 134, 0.000000, 133.909877, 5.20214688742);
static const Atom I135("I", 53, 135, 0.000000, 134.910050, 5.24100172752);
static const Atom I136("I", 53, 136, 0.000000, 135.914660, 5.28002893673);
static const Atom I137("I", 53, 137, 0.000000, 136.917873, 5.31900187511);
static const Atom I138("I", 53, 138, 0.000000, 137.922380, 5.35802508296);
static const Atom I139("I", 53, 139, 0.000000, 138.926090, 5.39701732886);
static const Atom I140("I", 53, 140, 0.000000, 139.931210, 5.43606435061);
static const Atom I141("I", 53, 141, 0.000000, 140.934830, 5.47505310018);
static const Atom I142("I", 53, 142, 0.000000, 141.940180, 5.51410905699);
static const Atom I143("I", 53, 143, 0.000000, 142.944070, 5.55310829555);
static const Atom I144("I", 53, 144, 0.000000, 143.949610, 5.59217163351);
static const Atom I108("I", 53, 108, 0.000000, 107.943290, 4.19339381584);
static const Atom I109("I", 53, 109, 0.000000, 108.938190, 4.2320438098);
static const Atom I110("I", 53, 110, 0.000000, 109.935210, 4.27077616179);
static const Atom I111("I", 53, 111, 0.000000, 110.930280, 4.30943275993);
static const Atom I112("I", 53, 112, 0.000000, 111.927970, 4.34819114015);
static const Atom I113("I", 53, 113, 0.000000, 112.923640, 4.38687104717);
static const Atom I114("I", 53, 114, 0.000000, 113.921850, 4.42564962842);
static const Atom I115("I", 53, 115, 0.000000, 114.917920, 4.46434507468);
static const Atom I116("I", 53, 116, 0.000000, 115.916740, 4.50314735328);
static const Atom I117("I", 53, 117, 0.000000, 116.913650, 4.54187543197);
static const Atom I118("I", 53, 118, 0.000000, 117.913380, 4.58071306235);
static const Atom I119("I", 53, 119, 0.000000, 118.910180, 4.61943686775);
static const Atom I120("I", 53, 120, 0.000000, 119.910048, 4.65827985917);
static const Atom I121("I", 53, 121, 0.000000, 120.907366, 4.69702378789);
static const Atom I122("I", 53, 122, 0.000000, 121.907592, 4.73588068695);
static const Atom I123("I", 53, 123, 0.000000, 122.905598, 4.77465134317);
static const Atom I124("I", 53, 124, 0.000000, 123.906211, 4.81352329198);
static const Atom I125("I", 53, 125, 0.000000, 124.904624, 4.85230974774);
static const Atom I126("I", 53, 126, 0.000000, 125.905619, 4.89119651711);
static const Atom I127("I", 53, 127, 100.000000, 126.904468, 4.9299999223);
static const Atom Xe("Xe", 54, 0, 0.000000, 131.293000, 3.52);
static const Atom Xe128("Xe", 54, 128, 1.920000, 127.903530, 3.42912742498);
static const Atom Xe129("Xe", 54, 129, 26.440000, 128.904779, 3.45597117775);
static const Atom Xe130("Xe", 54, 130, 4.080000, 129.903508, 3.48274734988);
static const Atom Xe131("Xe", 54, 131, 21.180000, 130.905082, 3.5095998133);
static const Atom Xe132("Xe", 54, 132, 26.890000, 131.904155, 3.53638521353);
static const Atom Xe133("Xe", 54, 133, 0.000000, 132.905906, 3.56324243577);
static const Atom Xe134("Xe", 54, 134, 10.440000, 133.905394, 3.59003898639);
static const Atom Xe135("Xe", 54, 135, 0.000000, 134.907207, 3.61689784406);
static const Atom Xe136("Xe", 54, 136, 8.870000, 135.907220, 3.64370845666);
static const Atom Xe137("Xe", 54, 137, 0.000000, 136.911563, 3.6706351577);
static const Atom Xe138("Xe", 54, 138, 0.000000, 137.913990, 3.69751049028);
static const Atom Xe139("Xe", 54, 139, 0.000000, 138.918787, 3.72444936318);
static const Atom Xe140("Xe", 54, 140, 0.000000, 139.921640, 3.75133611693);
static const Atom Xe141("Xe", 54, 141, 0.000000, 140.926650, 3.77828070042);
static const Atom Xe142("Xe", 54, 142, 0.000000, 141.929700, 3.80517273579);
static const Atom Xe143("Xe", 54, 143, 0.000000, 142.934890, 3.83212214513);
static const Atom Xe144("Xe", 54, 144, 0.000000, 143.938230, 3.85902195547);
static const Atom Xe145("Xe", 54, 145, 0.000000, 144.943670, 3.88597806738);
static const Atom Xe146("Xe", 54, 146, 0.000000, 145.947300, 3.9128856527);
static const Atom Xe147("Xe", 54, 147, 0.000000, 146.953010, 3.93984900337);
static const Atom Xe110("Xe", 54, 110, 0.000000, 109.944480, 2.94764054138);
static const Atom Xe111("Xe", 54, 111, 0.000000, 110.941630, 2.9743743962);
static const Atom Xe112("Xe", 54, 112, 0.000000, 111.935670, 3.00102487109);
static const Atom Xe113("Xe", 54, 113, 0.000000, 112.933380, 3.02777373965);
static const Atom Xe114("Xe", 54, 114, 0.000000, 113.928150, 3.05444378604);
static const Atom Xe115("Xe", 54, 115, 0.000000, 114.926540, 3.08121088558);
static const Atom Xe116("Xe", 54, 116, 0.000000, 115.921740, 3.10789246037);
static const Atom Xe117("Xe", 54, 117, 0.000000, 116.920560, 3.13467108833);
static const Atom Xe118("Xe", 54, 118, 0.000000, 117.916570, 3.16137437944);
static const Atom Xe119("Xe", 54, 119, 0.000000, 118.915550, 3.18815729704);
static const Atom Xe120("Xe", 54, 120, 0.000000, 119.912150, 3.21487640621);
static const Atom Xe121("Xe", 54, 121, 0.000000, 120.911386, 3.24166618723);
static const Atom Xe122("Xe", 54, 122, 0.000000, 121.908550, 3.26840041739);
static const Atom Xe123("Xe", 54, 123, 0.000000, 122.908471, 3.29520856344);
static const Atom Xe124("Xe", 54, 124, 0.090000, 123.905896, 3.32194978572);
static const Atom Xe125("Xe", 54, 125, 0.000000, 124.906398, 3.34877351926);
static const Atom Xe126("Xe", 54, 126, 0.090000, 125.904269, 3.37552669891);
static const Atom Xe127("Xe", 54, 127, 0.000000, 126.905180, 3.40236138713);
static const Atom Cs("Cs", 55, 0, 0.000000, 132.905450, 1.873);
static const Atom Cs128("Cs", 55, 128, 0.000000, 127.907748, 1.8025687585);
static const Atom Cs129("Cs", 55, 129, 0.000000, 128.906063, 1.81663773757);
static const Atom Cs130("Cs", 55, 130, 0.000000, 129.906706, 1.83073952451);
static const Atom Cs131("Cs", 55, 131, 0.000000, 130.905460, 1.84481469029);
static const Atom Cs132("Cs", 55, 132, 0.000000, 131.906430, 1.85892108555);
static const Atom Cs133("Cs", 55, 133, 100.000000, 132.905447, 1.87299995772);
static const Atom Cs134("Cs", 55, 134, 0.000000, 133.906713, 1.88711052443);
static const Atom Cs135("Cs", 55, 135, 0.000000, 134.905972, 1.90119280704);
static const Atom Cs136("Cs", 55, 136, 0.000000, 135.907306, 1.91530433205);
static const Atom Cs137("Cs", 55, 137, 0.000000, 136.907084, 1.92939392878);
static const Atom Cs138("Cs", 55, 138, 0.000000, 137.911011, 1.94354199623);
static const Atom Cs139("Cs", 55, 139, 0.000000, 138.913358, 1.95766779717);
static const Atom Cs140("Cs", 55, 140, 0.000000, 139.917277, 1.97181575188);
static const Atom Cs141("Cs", 55, 141, 0.000000, 140.920044, 1.98594747177);
static const Atom Cs142("Cs", 55, 142, 0.000000, 141.924292, 2.00010006298);
static const Atom Cs143("Cs", 55, 143, 0.000000, 142.927330, 2.014235602);
static const Atom Cs144("Cs", 55, 144, 0.000000, 143.932030, 2.02839456313);
static const Atom Cs145("Cs", 55, 145, 0.000000, 144.935390, 2.04253464);
static const Atom Cs146("Cs", 55, 146, 0.000000, 145.940160, 2.05669458762);
static const Atom Cs147("Cs", 55, 147, 0.000000, 146.943860, 2.07083945602);
static const Atom Cs148("Cs", 55, 148, 0.000000, 147.948900, 2.08500320867);
static const Atom Cs149("Cs", 55, 149, 0.000000, 148.952720, 2.0991497682);
static const Atom Cs150("Cs", 55, 150, 0.000000, 149.957970, 2.11331648032);
static const Atom Cs151("Cs", 55, 151, 0.000000, 150.962000, 2.12746599933);
static const Atom Cs112("Cs", 55, 112, 0.000000, 111.950330, 1.57768524985);
static const Atom Cs113("Cs", 55, 113, 0.000000, 112.944540, 1.59169637829);
static const Atom Cs114("Cs", 55, 114, 0.000000, 113.941420, 1.6057451343);
static const Atom Cs115("Cs", 55, 115, 0.000000, 114.935940, 1.61976063149);
static const Atom Cs116("Cs", 55, 116, 0.000000, 115.932910, 1.63381065585);
static const Atom Cs117("Cs", 55, 117, 0.000000, 116.928640, 1.64784320523);
static const Atom Cs118("Cs", 55, 118, 0.000000, 117.926555, 1.66190654721);
static const Atom Cs119("Cs", 55, 119, 0.000000, 118.922371, 1.67594030857);
static const Atom Cs120("Cs", 55, 120, 0.000000, 119.920678, 1.6900091749);
static const Atom Cs121("Cs", 55, 121, 0.000000, 120.917184, 1.70405266023);
static const Atom Cs122("Cs", 55, 122, 0.000000, 121.916122, 1.71813041908);
static const Atom Cs123("Cs", 55, 123, 0.000000, 122.912990, 1.73217900598);
static const Atom Cs124("Cs", 55, 124, 0.000000, 123.912246, 1.74626124631);
static const Atom Cs125("Cs", 55, 125, 0.000000, 124.909725, 1.76031844386);
static const Atom Cs126("Cs", 55, 126, 0.000000, 125.909448, 1.7744072655);
static const Atom Cs127("Cs", 55, 127, 0.000000, 126.907418, 1.78847138258);
static const Atom Ba("Ba", 56, 0, 0.000000, 137.327000, 3.5);
static const Atom Ba128("Ba", 56, 128, 0.000000, 127.908309, 3.25994947461);
static const Atom Ba129("Ba", 56, 129, 0.000000, 128.908674, 3.28544538947);
static const Atom Ba130("Ba", 56, 130, 0.106000, 129.906310, 3.31087175137);
static const Atom Ba131("Ba", 56, 131, 0.000000, 130.906931, 3.3363741908);
static const Atom Ba132("Ba", 56, 132, 0.101000, 131.905056, 3.36181301565);
static const Atom Ba133("Ba", 56, 133, 0.000000, 132.906002, 3.38732373823);
static const Atom Ba134("Ba", 56, 134, 2.417000, 133.904503, 3.41277214605);
static const Atom Ba135("Ba", 56, 135, 6.592000, 134.905683, 3.43828883249);
static const Atom Ba136("Ba", 56, 136, 7.854000, 135.904570, 3.46374707814);
static const Atom Ba137("Ba", 56, 137, 11.232000, 136.905821, 3.48926557414);
static const Atom Ba138("Ba", 56, 138, 71.698000, 137.905241, 3.51473740415);
static const Atom Ba139("Ba", 56, 139, 0.000000, 138.908835, 3.54031561528);
static const Atom Ba140("Ba", 56, 140, 0.000000, 139.910599, 3.56584718591);
static const Atom Ba141("Ba", 56, 141, 0.000000, 140.914406, 3.59143082569);
static const Atom Ba142("Ba", 56, 142, 0.000000, 141.916448, 3.6169694816);
static const Atom Ba143("Ba", 56, 143, 0.000000, 142.920617, 3.64256234754);
static const Atom Ba144("Ba", 56, 144, 0.000000, 143.922940, 3.66810816518);
static const Atom Ba145("Ba", 56, 145, 0.000000, 144.926920, 3.69369621415);
static const Atom Ba146("Ba", 56, 146, 0.000000, 145.930110, 3.71926412869);
static const Atom Ba147("Ba", 56, 147, 0.000000, 146.933990, 3.74484962899);
static const Atom Ba148("Ba", 56, 148, 0.000000, 147.937680, 3.77043028683);
static const Atom Ba149("Ba", 56, 149, 0.000000, 148.942460, 3.79603872509);
static const Atom Ba150("Ba", 56, 150, 0.000000, 149.945620, 3.82160587503);
static const Atom Ba151("Ba", 56, 151, 0.000000, 150.950700, 3.84722195927);
static const Atom Ba152("Ba", 56, 152, 0.000000, 151.954160, 3.87279675519);
static const Atom Ba153("Ba", 56, 153, 0.000000, 152.959610, 3.89842226947);
static const Atom Ba114("Ba", 56, 114, 0.000000, 113.950940, 2.90422342292);
static const Atom Ba115("Ba", 56, 115, 0.000000, 114.947710, 2.92962771341);
static const Atom Ba116("Ba", 56, 116, 0.000000, 115.941680, 2.95496064139);
static const Atom Ba117("Ba", 56, 117, 0.000000, 116.938860, 2.98037538139);
static const Atom Ba118("Ba", 56, 118, 0.000000, 117.933440, 3.0057238562);
static const Atom Ba119("Ba", 56, 119, 0.000000, 118.931050, 3.03114955544);
static const Atom Ba120("Ba", 56, 120, 0.000000, 119.926050, 3.05650873463);
static const Atom Ba121("Ba", 56, 121, 0.000000, 120.924490, 3.08195558776);
static const Atom Ba122("Ba", 56, 122, 0.000000, 121.920260, 3.10733439163);
static const Atom Ba123("Ba", 56, 123, 0.000000, 122.918850, 3.13278506776);
static const Atom Ba124("Ba", 56, 124, 0.000000, 123.915088, 3.15817579937);
static const Atom Ba125("Ba", 56, 125, 0.000000, 124.914620, 3.18365048388);
static const Atom Ba126("Ba", 56, 126, 0.000000, 125.911244, 3.20905105333);
static const Atom Ba127("Ba", 56, 127, 0.000000, 126.911120, 3.23453450523);
static const Atom La("La", 57, 0, 0.000000, 138.905500, 6.145);
static const Atom La128("La", 57, 128, 0.000000, 127.915450, 5.6588143756);
static const Atom La129("La", 57, 129, 0.000000, 128.912670, 5.70293010104);
static const Atom La130("La", 57, 130, 0.000000, 129.912320, 5.74715332654);
static const Atom La131("La", 57, 131, 0.000000, 130.910110, 5.79129426805);
static const Atom La132("La", 57, 132, 0.000000, 131.910110, 5.8355329771);
static const Atom La133("La", 57, 133, 0.000000, 132.908400, 5.87969603795);
static const Atom La134("La", 57, 134, 0.000000, 133.908490, 5.92393872849);
static const Atom La135("La", 57, 135, 0.000000, 134.906971, 5.96811023894);
static const Atom La136("La", 57, 136, 0.000000, 135.907650, 6.01237898607);
static const Atom La137("La", 57, 137, 0.000000, 136.906470, 6.05656549345);
static const Atom La138("La", 57, 138, 0.090000, 137.907107, 6.10083238256);
static const Atom La139("La", 57, 139, 99.910000, 138.906348, 6.14503751443);
static const Atom La140("La", 57, 140, 0.000000, 139.909473, 6.18941446944);
static const Atom La141("La", 57, 141, 0.000000, 140.910957, 6.23371882874);
static const Atom La142("La", 57, 142, 0.000000, 141.914074, 6.27809542984);
static const Atom La143("La", 57, 143, 0.000000, 142.916059, 6.32242195273);
static const Atom La144("La", 57, 144, 0.000000, 143.919590, 6.36681686866);
static const Atom La145("La", 57, 145, 0.000000, 144.921640, 6.41114626707);
static const Atom La146("La", 57, 146, 0.000000, 145.925700, 6.45556458528);
static const Atom La147("La", 57, 147, 0.000000, 146.927820, 6.49989708039);
static const Atom La148("La", 57, 148, 0.000000, 147.932190, 6.5443291126);
static const Atom La149("La", 57, 149, 0.000000, 148.934370, 6.58866426203);
static const Atom La150("La", 57, 150, 0.000000, 149.938570, 6.63308877366);
static const Atom La151("La", 57, 151, 0.000000, 150.941560, 6.67745975645);
static const Atom La152("La", 57, 152, 0.000000, 151.946110, 6.72189975163);
static const Atom La153("La", 57, 153, 0.000000, 152.949450, 6.76628621797);
static const Atom La154("La", 57, 154, 0.000000, 153.954400, 6.81074390863);
static const Atom La155("La", 57, 155, 0.000000, 154.958130, 6.85514762806);
static const Atom La117("La", 57, 117, 0.000000, 116.950010, 5.17371746583);
static const Atom La118("La", 57, 118, 0.000000, 117.946570, 5.21780399372);
static const Atom La119("La", 57, 119, 0.000000, 118.940990, 5.26179585078);
static const Atom La120("La", 57, 120, 0.000000, 119.938070, 5.3059053828);
static const Atom La121("La", 57, 121, 0.000000, 120.933010, 5.34992024398);
static const Atom La122("La", 57, 122, 0.000000, 121.930710, 5.394057204);
static const Atom La123("La", 57, 123, 0.000000, 122.926240, 5.43809816602);
static const Atom La124("La", 57, 124, 0.000000, 123.924530, 5.48226122688);
static const Atom La125("La", 57, 125, 0.000000, 124.920670, 5.52632917451);
static const Atom La126("La", 57, 126, 0.000000, 125.919370, 5.57051037324);
static const Atom La127("La", 57, 127, 0.000000, 126.916160, 5.61460707603);
static const Atom Ce("Ce", 58, 0, 0.000000, 140.116000, 6.77);
static const Atom Ce128("Ce", 58, 128, 0.000000, 127.918870, 6.18066994419);
static const Atom Ce129("Ce", 58, 129, 0.000000, 128.918090, 6.22894936553);
static const Atom Ce130("Ce", 58, 130, 0.000000, 129.914690, 6.27710219604);
static const Atom Ce131("Ce", 58, 131, 0.000000, 130.914420, 6.3254062591);
static const Atom Ce132("Ce", 58, 132, 0.000000, 131.911490, 6.37358179865);
static const Atom Ce133("Ce", 58, 133, 0.000000, 132.911550, 6.42190180636);
static const Atom Ce134("Ce", 58, 134, 0.000000, 133.909030, 6.47009715593);
static const Atom Ce135("Ce", 58, 135, 0.000000, 134.909146, 6.51841986939);
static const Atom Ce136("Ce", 58, 136, 0.185000, 135.907140, 6.56664005396);
static const Atom Ce137("Ce", 58, 137, 0.000000, 136.907780, 6.61498808559);
static const Atom Ce138("Ce", 58, 138, 0.251000, 137.905986, 6.66321851337);
static const Atom Ce139("Ce", 58, 139, 0.000000, 138.906647, 6.71156755966);
static const Atom Ce140("Ce", 58, 140, 88.450000, 139.905434, 6.75982605969);
static const Atom Ce141("Ce", 58, 141, 0.000000, 140.908271, 6.80828024401);
static const Atom Ce142("Ce", 58, 142, 11.114000, 141.909240, 6.85664417197);
static const Atom Ce143("Ce", 58, 143, 0.000000, 142.912381, 6.90511304469);
static const Atom Ce144("Ce", 58, 144, 0.000000, 143.913643, 6.95349112956);
static const Atom Ce145("Ce", 58, 145, 0.000000, 144.917230, 7.00198155171);
static const Atom Ce146("Ce", 58, 146, 0.000000, 145.918690, 7.05036920337);
static const Atom Ce147("Ce", 58, 147, 0.000000, 146.922510, 7.09887088341);
static const Atom Ce148("Ce", 58, 148, 0.000000, 147.924390, 7.14727882826);
static const Atom Ce149("Ce", 58, 149, 0.000000, 148.928290, 7.19578437366);
static const Atom Ce150("Ce", 58, 150, 0.000000, 149.930230, 7.24419521753);
static const Atom Ce151("Ce", 58, 151, 0.000000, 150.934040, 7.2926964144);
static const Atom Ce152("Ce", 58, 152, 0.000000, 151.936380, 7.34112658512);
static const Atom Ce153("Ce", 58, 153, 0.000000, 152.940580, 7.38964662565);
static const Atom Ce154("Ce", 58, 154, 0.000000, 153.943320, 7.43809612321);
static const Atom Ce155("Ce", 58, 155, 0.000000, 154.948040, 7.48664128865);
static const Atom Ce156("Ce", 58, 156, 0.000000, 155.951260, 7.53511397842);
static const Atom Ce157("Ce", 58, 157, 0.000000, 156.956340, 7.58367653801);
static const Atom Ce119("Ce", 58, 119, 0.000000, 118.952760, 5.74745343287);
static const Atom Ce120("Ce", 58, 120, 0.000000, 119.946640, 5.79547484085);
static const Atom Ce121("Ce", 58, 121, 0.000000, 120.943670, 5.84364844771);
static const Atom Ce122("Ce", 58, 122, 0.000000, 121.938010, 5.89169208156);
static const Atom Ce123("Ce", 58, 123, 0.000000, 122.935510, 5.93988839747);
static const Atom Ce124("Ce", 58, 124, 0.000000, 123.930520, 5.98796440378);
static const Atom Ce125("Ce", 58, 125, 0.000000, 124.928540, 6.03618584459);
static const Atom Ce126("Ce", 58, 126, 0.000000, 125.924100, 6.0842884253);
static const Atom Ce127("Ce", 58, 127, 0.000000, 126.922750, 6.13254030589);
static const Atom Pr("Pr", 59, 0, 0.000000, 140.907650, 6.773);
static const Atom Pr128("Pr", 59, 128, 0.000000, 127.928800, 6.14914635508);
static const Atom Pr129("Pr", 59, 129, 0.000000, 128.924860, 6.19702391446);
static const Atom Pr130("Pr", 59, 130, 0.000000, 129.923380, 6.24501971852);
static const Atom Pr131("Pr", 59, 131, 0.000000, 130.920060, 6.2929270794);
static const Atom Pr132("Pr", 59, 132, 0.000000, 131.919120, 6.34094883961);
static const Atom Pr133("Pr", 59, 133, 0.000000, 132.916200, 6.38887542727);
static const Atom Pr134("Pr", 59, 134, 0.000000, 133.915670, 6.43691689493);
static const Atom Pr135("Pr", 59, 135, 0.000000, 134.913140, 6.4848622287);
static const Atom Pr136("Pr", 59, 136, 0.000000, 135.912650, 6.53290561903);
static const Atom Pr137("Pr", 59, 137, 0.000000, 136.910680, 6.58087787029);
static const Atom Pr138("Pr", 59, 138, 0.000000, 137.910749, 6.62894813005);
static const Atom Pr139("Pr", 59, 139, 0.000000, 138.908932, 6.67692773555);
static const Atom Pr140("Pr", 59, 140, 0.000000, 139.909071, 6.72500135999);
static const Atom Pr141("Pr", 59, 141, 100.000000, 140.907648, 6.77299990387);
static const Atom Pr142("Pr", 59, 142, 0.000000, 141.910040, 6.82118182313);
static const Atom Pr143("Pr", 59, 143, 0.000000, 142.910812, 6.86928587395);
static const Atom Pr144("Pr", 59, 144, 0.000000, 143.913301, 6.9174724557);
static const Atom Pr145("Pr", 59, 145, 0.000000, 144.914507, 6.96559736757);
static const Atom Pr146("Pr", 59, 146, 0.000000, 145.917590, 7.0138125011);
static const Atom Pr147("Pr", 59, 147, 0.000000, 146.918980, 7.06194625728);
static const Atom Pr148("Pr", 59, 148, 0.000000, 147.922180, 7.11016701464);
static const Atom Pr149("Pr", 59, 149, 0.000000, 148.923791, 7.15831139362);
static const Atom Pr150("Pr", 59, 150, 0.000000, 149.927000, 7.20653258358);
static const Atom Pr151("Pr", 59, 151, 0.000000, 150.928230, 7.25465864905);
static const Atom Pr152("Pr", 59, 152, 0.000000, 151.931600, 7.30288757779);
static const Atom Pr153("Pr", 59, 153, 0.000000, 152.933650, 7.35105305816);
static const Atom Pr154("Pr", 59, 154, 0.000000, 153.937390, 7.39929977166);
static const Atom Pr155("Pr", 59, 155, 0.000000, 154.939990, 7.44749168885);
static const Atom Pr156("Pr", 59, 156, 0.000000, 155.944120, 7.49575714846);
static const Atom Pr157("Pr", 59, 157, 0.000000, 156.947170, 7.54397069577);
static const Atom Pr158("Pr", 59, 158, 0.000000, 157.951780, 7.59225922752);
static const Atom Pr159("Pr", 59, 159, 0.000000, 158.955230, 7.64049200161);
static const Atom Pr121("Pr", 59, 121, 0.000000, 120.955360, 5.81395441113);
static const Atom Pr122("Pr", 59, 122, 0.000000, 121.951650, 5.86184302591);
static const Atom Pr123("Pr", 59, 123, 0.000000, 122.945960, 5.90963646814);
static const Atom Pr124("Pr", 59, 124, 0.000000, 123.942960, 5.95755921045);
static const Atom Pr125("Pr", 59, 125, 0.000000, 124.937830, 6.00537957017);
static const Atom Pr126("Pr", 59, 126, 0.000000, 125.935310, 6.0533253846);
static const Atom Pr127("Pr", 59, 127, 0.000000, 126.930830, 6.10117698784);
static const Atom Nd("Nd", 60, 0, 0.000000, 144.240000, 7.008);
static const Atom Nd128("Nd", 60, 128, 0.000000, 127.935390, 6.21582926456);
static const Atom Nd129("Nd", 60, 129, 0.000000, 128.933250, 6.2643109817);
static const Atom Nd130("Nd", 60, 130, 0.000000, 129.928780, 6.31267949418);
static const Atom Nd131("Nd", 60, 131, 0.000000, 130.927100, 6.36118356073);
static const Atom Nd132("Nd", 60, 132, 0.000000, 131.923120, 6.4095758802);
static const Atom Nd133("Nd", 60, 133, 0.000000, 132.922210, 6.45811735774);
static const Atom Nd134("Nd", 60, 134, 0.000000, 133.918650, 6.50653008319);
static const Atom Nd135("Nd", 60, 135, 0.000000, 134.918240, 6.55509585358);
static const Atom Nd136("Nd", 60, 136, 0.000000, 135.915020, 6.60352509817);
static const Atom Nd137("Nd", 60, 137, 0.000000, 136.914640, 6.65209232612);
static const Atom Nd138("Nd", 60, 138, 0.000000, 137.911930, 6.70054634942);
static const Atom Nd139("Nd", 60, 139, 0.000000, 138.911920, 6.74913155408);
static const Atom Nd140("Nd", 60, 140, 0.000000, 139.909310, 6.79759043594);
static const Atom Nd141("Nd", 60, 141, 0.000000, 140.909605, 6.84619045923);
static const Atom Nd142("Nd", 60, 142, 27.200000, 141.907719, 6.89468451714);
static const Atom Nd143("Nd", 60, 143, 12.200000, 142.909810, 6.94337180033);
static const Atom Nd144("Nd", 60, 144, 23.800000, 143.910083, 6.99197075474);
static const Atom Nd145("Nd", 60, 145, 8.300000, 144.912569, 7.04067722928);
static const Atom Nd146("Nd", 60, 146, 17.200000, 145.913112, 7.08928930183);
static const Atom Nd147("Nd", 60, 147, 0.000000, 146.916096, 7.13801997205);
static const Atom Nd148("Nd", 60, 148, 5.700000, 147.916889, 7.18664419101);
static const Atom Nd149("Nd", 60, 149, 0.000000, 148.920144, 7.23538802795);
static const Atom Nd150("Nd", 60, 150, 5.600000, 149.920887, 7.28400981764);
static const Atom Nd151("Nd", 60, 151, 0.000000, 150.923825, 7.33273825291);
static const Atom Nd152("Nd", 60, 152, 0.000000, 151.924680, 7.38136548419);
static const Atom Nd153("Nd", 60, 153, 0.000000, 152.927695, 7.43009766057);
static const Atom Nd154("Nd", 60, 154, 0.000000, 153.929480, 7.47877007654);
static const Atom Nd155("Nd", 60, 155, 0.000000, 154.932630, 7.52750881198);
static const Atom Nd156("Nd", 60, 156, 0.000000, 155.935200, 7.57621936772);
static const Atom Nd157("Nd", 60, 157, 0.000000, 156.939270, 7.625002802);
static const Atom Nd158("Nd", 60, 158, 0.000000, 157.941870, 7.67371481531);
static const Atom Nd159("Nd", 60, 159, 0.000000, 158.946390, 7.72252011314);
static const Atom Nd160("Nd", 60, 160, 0.000000, 159.949390, 7.77125156073);
static const Atom Nd161("Nd", 60, 161, 0.000000, 160.954330, 7.82007726456);
static const Atom Nd126("Nd", 60, 126, 0.000000, 125.943070, 6.11903102163);
static const Atom Nd127("Nd", 60, 127, 0.000000, 126.940500, 6.16749184692);
static const Atom Pm("Pm", 61, 0, 0.000000, 145.000000, 7.264);
static const Atom Pm128("Pm", 61, 128, 0.000000, 127.948260, 6.4097666251);
static const Atom Pm129("Pm", 61, 129, 0.000000, 128.943160, 6.45960768441);
static const Atom Pm130("Pm", 61, 130, 0.000000, 129.940450, 6.50956847448);
static const Atom Pm131("Pm", 61, 131, 0.000000, 130.935800, 6.55943207724);
static const Atom Pm132("Pm", 61, 132, 0.000000, 131.933750, 6.60942593103);
static const Atom Pm133("Pm", 61, 133, 0.000000, 132.929720, 6.65932059366);
static const Atom Pm134("Pm", 61, 134, 0.000000, 133.928490, 6.70935552662);
static const Atom Pm135("Pm", 61, 135, 0.000000, 134.924620, 6.75925820469);
static const Atom Pm136("Pm", 61, 136, 0.000000, 135.923450, 6.80929614345);
static const Atom Pm137("Pm", 61, 137, 0.000000, 136.920710, 6.85925543062);
static const Atom Pm138("Pm", 61, 138, 0.000000, 137.919450, 6.90928886069);
static const Atom Pm139("Pm", 61, 139, 0.000000, 138.916760, 6.95925065269);
static const Atom Pm140("Pm", 61, 140, 0.000000, 139.915800, 7.00929911172);
static const Atom Pm141("Pm", 61, 141, 0.000000, 140.913607, 7.05928580171);
static const Atom Pm142("Pm", 61, 142, 0.000000, 141.912950, 7.10934944);
static const Atom Pm143("Pm", 61, 143, 0.000000, 142.910928, 7.1593446965);
static const Atom Pm144("Pm", 61, 144, 0.000000, 143.912586, 7.2095243083);
static const Atom Pm145("Pm", 61, 145, 0.000000, 144.912744, 7.25962877528);
static const Atom Pm146("Pm", 61, 146, 0.000000, 145.914692, 7.30982291509);
static const Atom Pm147("Pm", 61, 147, 0.000000, 146.915134, 7.35994160949);
static const Atom Pm148("Pm", 61, 148, 0.000000, 147.917468, 7.41015508657);
static const Atom Pm149("Pm", 61, 149, 0.000000, 148.918329, 7.46029477142);
static const Atom Pm150("Pm", 61, 150, 0.000000, 149.920979, 7.51052407901);
static const Atom Pm151("Pm", 61, 151, 0.000000, 150.921203, 7.56063185236);
static const Atom Pm152("Pm", 61, 152, 0.000000, 151.923490, 7.6108429749);
static const Atom Pm153("Pm", 61, 153, 0.000000, 152.924113, 7.66097073677);
static const Atom Pm154("Pm", 61, 154, 0.000000, 153.926550, 7.71118937379);
static const Atom Pm155("Pm", 61, 155, 0.000000, 154.928100, 7.76136357517);
static const Atom Pm156("Pm", 61, 156, 0.000000, 155.931060, 7.81160841269);
static const Atom Pm157("Pm", 61, 157, 0.000000, 156.933200, 7.86181217103);
static const Atom Pm158("Pm", 61, 158, 0.000000, 157.936690, 7.91208355972);
static const Atom Pm159("Pm", 61, 159, 0.000000, 158.939130, 7.96230234703);
static const Atom Pm160("Pm", 61, 160, 0.000000, 159.942990, 8.01259227145);
static const Atom Pm161("Pm", 61, 161, 0.000000, 160.945860, 8.06283260028);
static const Atom Pm162("Pm", 61, 162, 0.000000, 161.950290, 8.11315107972);
static const Atom Pm163("Pm", 61, 163, 0.000000, 162.953520, 8.16340944331);
static const Atom Sm("Sm", 62, 0, 0.000000, 150.360000, 7.52);
static const Atom Sm130("Sm", 62, 130, 0.000000, 129.948630, 6.49916);
static const Atom Sm131("Sm", 62, 131, 0.000000, 130.945890, 6.54903626496);
static const Atom Sm132("Sm", 62, 132, 0.000000, 131.940820, 6.59879599894);
static const Atom Sm133("Sm", 62, 133, 0.000000, 132.938730, 6.64870477255);
static const Atom Sm134("Sm", 62, 134, 0.000000, 133.934020, 6.69848251131);
static const Atom Sm135("Sm", 62, 135, 0.000000, 134.932350, 6.7484122905);
static const Atom Sm136("Sm", 62, 136, 0.000000, 135.928300, 6.79822303804);
static const Atom Sm137("Sm", 62, 137, 0.000000, 136.927050, 6.84817382283);
static const Atom Sm138("Sm", 62, 138, 0.000000, 137.923540, 6.89801157755);
static const Atom Sm139("Sm", 62, 139, 0.000000, 138.922302, 6.94796296249);
static const Atom Sm140("Sm", 62, 140, 0.000000, 139.918991, 6.99781066986);
static const Atom Sm141("Sm", 62, 141, 0.000000, 140.918469, 7.04779786433);
static const Atom Sm142("Sm", 62, 142, 0.000000, 141.915193, 7.09764732216);
static const Atom Sm143("Sm", 62, 143, 0.000000, 142.914624, 7.147632166);
static const Atom Sm144("Sm", 62, 144, 3.070000, 143.911995, 7.19751398244);
static const Atom Sm145("Sm", 62, 145, 0.000000, 144.913406, 7.24759785262);
static const Atom Sm146("Sm", 62, 146, 0.000000, 145.913037, 7.29759269912);
static const Atom Sm147("Sm", 62, 147, 14.990000, 146.914893, 7.34769882522);
static const Atom Sm148("Sm", 62, 148, 11.240000, 147.914818, 7.39770837563);
static const Atom Sm149("Sm", 62, 149, 13.820000, 148.917180, 7.44783980846);
static const Atom Sm150("Sm", 62, 150, 7.380000, 149.917271, 7.49785766108);
static const Atom Sm151("Sm", 62, 151, 0.000000, 150.919928, 7.54800384783);
static const Atom Sm152("Sm", 62, 152, 26.750000, 151.919728, 7.59800714658);
static const Atom Sm153("Sm", 62, 153, 0.000000, 152.922094, 7.64813877946);
static const Atom Sm154("Sm", 62, 154, 22.750000, 153.922205, 7.69815763235);
static const Atom Sm155("Sm", 62, 155, 0.000000, 154.924636, 7.74829251609);
static const Atom Sm156("Sm", 62, 156, 0.000000, 155.925526, 7.79835032934);
static const Atom Sm157("Sm", 62, 157, 0.000000, 156.928350, 7.84850486832);
static const Atom Sm158("Sm", 62, 158, 0.000000, 157.929990, 7.89860019154);
static const Atom Sm159("Sm", 62, 159, 0.000000, 158.933200, 7.94877403565);
static const Atom Sm160("Sm", 62, 160, 0.000000, 159.935140, 7.99888436286);
static const Atom Sm161("Sm", 62, 161, 0.000000, 160.938830, 8.04908221335);
static const Atom Sm162("Sm", 62, 162, 0.000000, 161.941220, 8.09921504655);
static const Atom Sm163("Sm", 62, 163, 0.000000, 162.945360, 8.14943540303);
static const Atom Sm164("Sm", 62, 164, 0.000000, 163.948280, 8.19959474328);
static const Atom Sm165("Sm", 62, 165, 0.000000, 164.952980, 8.24984310721);
static const Atom Eu("Eu", 63, 0, 0.000000, 151.964000, 5.244);
static const Atom Eu132("Eu", 63, 132, 0.000000, 131.954160, 4.55349697981);
static const Atom Eu133("Eu", 63, 133, 0.000000, 132.948900, 4.58782363981);
static const Atom Eu134("Eu", 63, 134, 0.000000, 133.946320, 4.62224278171);
static const Atom Eu135("Eu", 63, 135, 0.000000, 134.941720, 4.6565922171);
static const Atom Eu136("Eu", 63, 136, 0.000000, 135.939500, 4.69102378195);
static const Atom Eu137("Eu", 63, 137, 0.000000, 136.935210, 4.72538391487);
static const Atom Eu138("Eu", 63, 138, 0.000000, 137.933450, 4.75983135348);
static const Atom Eu139("Eu", 63, 139, 0.000000, 138.929840, 4.79421495196);
static const Atom Eu140("Eu", 63, 140, 0.000000, 139.928080, 4.82866239057);
static const Atom Eu141("Eu", 63, 141, 0.000000, 140.924890, 4.86306048248);
static const Atom Eu142("Eu", 63, 142, 0.000000, 141.923400, 4.89751723829);
static const Atom Eu143("Eu", 63, 143, 0.000000, 142.920287, 4.93191798734);
static const Atom Eu144("Eu", 63, 144, 0.000000, 143.918774, 4.96637394946);
static const Atom Eu145("Eu", 63, 145, 0.000000, 144.916261, 5.00079540341);
static const Atom Eu146("Eu", 63, 146, 0.000000, 145.917200, 5.03533597957);
static const Atom Eu147("Eu", 63, 147, 0.000000, 146.916741, 5.06982831331);
static const Atom Eu148("Eu", 63, 148, 0.000000, 147.918154, 5.10438524635);
static const Atom Eu149("Eu", 63, 149, 0.000000, 148.917926, 5.13888555147);
static const Atom Eu150("Eu", 63, 150, 0.000000, 149.919698, 5.17345487294);
static const Atom Eu151("Eu", 63, 151, 47.810000, 150.919846, 5.20796815314);
static const Atom Eu152("Eu", 63, 152, 0.000000, 151.921740, 5.24254168461);
static const Atom Eu153("Eu", 63, 153, 52.190000, 152.921226, 5.2770321204);
static const Atom Eu154("Eu", 63, 154, 0.000000, 153.922975, 5.31160064818);
static const Atom Eu155("Eu", 63, 155, 0.000000, 154.922889, 5.34610585347);
static const Atom Eu156("Eu", 63, 156, 0.000000, 155.924751, 5.38067828067);
static const Atom Eu157("Eu", 63, 157, 0.000000, 156.925419, 5.41520950512);
static const Atom Eu158("Eu", 63, 158, 0.000000, 157.927840, 5.44980122239);
static const Atom Eu159("Eu", 63, 159, 0.000000, 158.929084, 5.48435232355);
static const Atom Eu160("Eu", 63, 160, 0.000000, 159.931970, 5.51896008713);
static const Atom Eu161("Eu", 63, 161, 0.000000, 160.933680, 5.55352726909);
static const Atom Eu162("Eu", 63, 162, 0.000000, 161.937040, 5.58815138954);
static const Atom Eu163("Eu", 63, 163, 0.000000, 162.939210, 5.62273444526);
static const Atom Eu164("Eu", 63, 164, 0.000000, 163.942990, 5.65737305915);
static const Atom Eu165("Eu", 63, 165, 0.000000, 164.945720, 5.69197543945);
static const Atom Eu166("Eu", 63, 166, 0.000000, 165.949970, 5.72663027217);
static const Atom Eu167("Eu", 63, 167, 0.000000, 166.953050, 5.76124473033);
static const Atom Gd("Gd", 64, 0, 0.000000, 157.250000, 7.901);
static const Atom Gd136("Gd", 64, 136, 0.000000, 135.947070, 6.83063783828);
static const Atom Gd137("Gd", 64, 137, 0.000000, 136.944650, 6.88076107886);
static const Atom Gd138("Gd", 64, 138, 0.000000, 137.939970, 6.9307707661);
static const Atom Gd139("Gd", 64, 139, 0.000000, 138.938080, 6.98092063644);
static const Atom Gd140("Gd", 64, 140, 0.000000, 139.933950, 7.03095795835);
static const Atom Gd141("Gd", 64, 141, 0.000000, 140.932210, 7.08111536541);
static const Atom Gd142("Gd", 64, 142, 0.000000, 141.928230, 7.13116022404);
static const Atom Gd143("Gd", 64, 143, 0.000000, 142.926740, 7.18133019231);
static const Atom Gd144("Gd", 64, 144, 0.000000, 143.922790, 7.23137655828);
static const Atom Gd145("Gd", 64, 145, 0.000000, 144.921690, 7.28156612203);
static const Atom Gd146("Gd", 64, 146, 0.000000, 145.918305, 7.33164087634);
static const Atom Gd147("Gd", 64, 147, 0.000000, 146.919089, 7.38192510136);
static const Atom Gd148("Gd", 64, 148, 0.000000, 147.918110, 7.43212074474);
static const Atom Gd149("Gd", 64, 149, 0.000000, 148.919336, 7.48242717797);
static const Atom Gd150("Gd", 64, 150, 0.000000, 149.918655, 7.53263779431);
static const Atom Gd151("Gd", 64, 151, 0.000000, 150.920344, 7.5829674909);
static const Atom Gd152("Gd", 64, 152, 0.200000, 151.919788, 7.63318438784);
static const Atom Gd153("Gd", 64, 153, 0.000000, 152.921746, 7.68352760029);
static const Atom Gd154("Gd", 64, 154, 2.180000, 153.920862, 7.73372801693);
static const Atom Gd155("Gd", 64, 155, 14.800000, 154.922619, 7.78406113017);
static const Atom Gd156("Gd", 64, 156, 20.470000, 155.922120, 7.83428089107);
static const Atom Gd157("Gd", 64, 157, 15.650000, 156.923957, 7.88461802389);
static const Atom Gd158("Gd", 64, 158, 24.840000, 157.924101, 7.93487009222);
static const Atom Gd159("Gd", 64, 159, 0.000000, 158.926385, 7.98522968448);
static const Atom Gd160("Gd", 64, 160, 21.860000, 159.927051, 8.03550798061);
static const Atom Gd161("Gd", 64, 161, 0.000000, 160.929666, 8.08588420392);
static const Atom Gd162("Gd", 64, 162, 0.000000, 161.930981, 8.13619510894);
static const Atom Gd163("Gd", 64, 163, 0.000000, 162.933990, 8.18659112871);
static const Atom Gd164("Gd", 64, 164, 0.000000, 163.935860, 8.23692991962);
static const Atom Gd165("Gd", 64, 165, 0.000000, 164.939380, 8.2873516145);
static const Atom Gd166("Gd", 64, 166, 0.000000, 165.941600, 8.3377079911);
static const Atom Gd167("Gd", 64, 167, 0.000000, 166.945570, 8.38815229615);
static const Atom Gd168("Gd", 64, 168, 0.000000, 167.948360, 8.43853731231);
static const Atom Gd169("Gd", 64, 169, 0.000000, 168.952870, 8.48900874957);
static const Atom Tb("Tb", 65, 0, 0.000000, 158.925340, 8.23);
static const Atom Tb138("Tb", 65, 138, 0.000000, 137.952870, 7.14393387549);
static const Atom Tb139("Tb", 65, 139, 0.000000, 138.948030, 7.19546855712);
static const Atom Tb140("Tb", 65, 140, 0.000000, 139.945540, 7.24712493426);
static const Atom Tb141("Tb", 65, 141, 0.000000, 140.941160, 7.29868343714);
static const Atom Tb142("Tb", 65, 142, 0.000000, 141.938860, 7.35034965349);
static const Atom Tb143("Tb", 65, 143, 0.000000, 142.934750, 7.40192213841);
static const Atom Tb144("Tb", 65, 144, 0.000000, 143.932530, 7.45359249758);
static const Atom Tb145("Tb", 65, 145, 0.000000, 144.928880, 7.50518880375);
static const Atom Tb146("Tb", 65, 146, 0.000000, 145.927180, 7.55688609129);
static const Atom Tb147("Tb", 65, 147, 0.000000, 146.924037, 7.60850865262);
static const Atom Tb148("Tb", 65, 148, 0.000000, 147.924300, 7.66030759475);
static const Atom Tb149("Tb", 65, 149, 0.000000, 148.923242, 7.71203812847);
static const Atom Tb150("Tb", 65, 150, 0.000000, 149.923654, 7.76384478661);
static const Atom Tb151("Tb", 65, 151, 0.000000, 150.923098, 7.81560131657);
static const Atom Tb152("Tb", 65, 152, 0.000000, 151.924070, 7.86743697449);
static const Atom Tb153("Tb", 65, 153, 0.000000, 152.923431, 7.91918920627);
static const Atom Tb154("Tb", 65, 154, 0.000000, 153.924690, 7.97103972658);
static const Atom Tb155("Tb", 65, 155, 0.000000, 154.923500, 8.02276342464);
static const Atom Tb156("Tb", 65, 156, 0.000000, 155.924744, 8.07461316817);
static const Atom Tb157("Tb", 65, 157, 0.000000, 156.924021, 8.12636104997);
static const Atom Tb158("Tb", 65, 158, 0.000000, 157.925410, 8.17821830238);
static const Atom Tb159("Tb", 65, 159, 100.000000, 158.925343, 8.23000015536);
static const Atom Tb160("Tb", 65, 160, 0.000000, 159.927164, 8.28187977902);
static const Atom Tb161("Tb", 65, 161, 0.000000, 160.927566, 8.33368591931);
static const Atom Tb162("Tb", 65, 162, 0.000000, 161.929480, 8.38557035901);
static const Atom Tb163("Tb", 65, 163, 0.000000, 162.930644, 8.43741595972);
static const Atom Tb164("Tb", 65, 164, 0.000000, 163.933350, 8.4893414134);
static const Atom Tb165("Tb", 65, 165, 0.000000, 164.934880, 8.54120596753);
static const Atom Tb166("Tb", 65, 166, 0.000000, 165.938050, 8.5931554496);
static const Atom Tb167("Tb", 65, 167, 0.000000, 166.940050, 8.64504434283);
static const Atom Tb168("Tb", 65, 168, 0.000000, 167.943640, 8.69701557473);
static const Atom Tb169("Tb", 65, 169, 0.000000, 168.946220, 8.74893450346);
static const Atom Tb170("Tb", 65, 170, 0.000000, 169.950250, 8.8009285209);
static const Atom Tb171("Tb", 65, 171, 0.000000, 170.953300, 8.85287178873);
static const Atom Dy("Dy", 66, 0, 0.000000, 162.500000, 8.551);
static const Atom Dy140("Dy", 66, 140, 0.000000, 139.953790, 7.36458374332);
static const Atom Dy141("Dy", 66, 141, 0.000000, 140.951190, 7.41706846578);
static const Atom Dy142("Dy", 66, 142, 0.000000, 141.946270, 7.46943110628);
static const Atom Dy143("Dy", 66, 143, 0.000000, 142.943830, 7.52192424818);
static const Atom Dy144("Dy", 66, 144, 0.000000, 143.939070, 7.57429530812);
static const Atom Dy145("Dy", 66, 145, 0.000000, 144.936950, 7.62680528892);
static const Atom Dy146("Dy", 66, 146, 0.000000, 145.932720, 7.67920423828);
static const Atom Dy147("Dy", 66, 147, 0.000000, 146.930880, 7.73172895311);
static const Atom Dy148("Dy", 66, 148, 0.000000, 147.927180, 7.78415579188);
static const Atom Dy149("Dy", 66, 149, 0.000000, 148.927334, 7.83678543406);
static const Atom Dy150("Dy", 66, 150, 0.000000, 149.925580, 7.88931467434);
static const Atom Dy151("Dy", 66, 151, 0.000000, 150.926180, 7.94196778572);
static const Atom Dy152("Dy", 66, 152, 0.000000, 151.924714, 7.99451218101);
static const Atom Dy153("Dy", 66, 153, 0.000000, 152.925761, 8.04718881422);
static const Atom Dy154("Dy", 66, 154, 0.000000, 153.924422, 8.09973989244);
static const Atom Dy155("Dy", 66, 155, 0.000000, 154.925749, 8.15243125969);
static const Atom Dy156("Dy", 66, 156, 0.060000, 155.924278, 8.20497539186);
static const Atom Dy157("Dy", 66, 157, 0.000000, 156.925461, 8.25765918161);
static const Atom Dy158("Dy", 66, 158, 0.100000, 157.924405, 8.31022515172);
static const Atom Dy159("Dy", 66, 159, 0.000000, 158.925736, 8.36291672945);
static const Atom Dy160("Dy", 66, 160, 2.340000, 159.925194, 8.41550974704);
static const Atom Dy161("Dy", 66, 161, 18.910000, 160.926930, 8.46822263649);
static const Atom Dy162("Dy", 66, 162, 25.510000, 161.926795, 8.52083707105);
static const Atom Dy163("Dy", 66, 163, 24.900000, 162.928728, 8.57356032694);
static const Atom Dy164("Dy", 66, 164, 28.180000, 163.929171, 8.62620517674);
static const Atom Dy165("Dy", 66, 165, 0.000000, 164.931700, 8.67895979508);
static const Atom Dy166("Dy", 66, 166, 0.000000, 165.932803, 8.7316393751);
static const Atom Dy167("Dy", 66, 167, 0.000000, 166.935650, 8.78441072708);
static const Atom Dy168("Dy", 66, 168, 0.000000, 167.937230, 8.83711540757);
static const Atom Dy169("Dy", 66, 169, 0.000000, 168.940300, 8.88989849415);
static const Atom Dy170("Dy", 66, 170, 0.000000, 169.942670, 8.94264474566);
static const Atom Dy171("Dy", 66, 171, 0.000000, 170.946480, 8.99546677218);
static const Atom Dy172("Dy", 66, 172, 0.000000, 171.949110, 9.04822670529);
static const Atom Dy173("Dy", 66, 173, 0.000000, 172.953440, 9.10107609502);
static const Atom Ho("Ho", 67, 0, 0.000000, 164.930320, 8.795);
static const Atom Ho142("Ho", 67, 142, 0.000000, 141.959860, 7.57008759032);
static const Atom Ho143("Ho", 67, 143, 0.000000, 142.954690, 7.62313744707);
static const Atom Ho144("Ho", 67, 144, 0.000000, 143.951640, 7.67630035399);
static const Atom Ho145("Ho", 67, 145, 0.000000, 144.946880, 7.72937207422);
static const Atom Ho146("Ho", 67, 146, 0.000000, 145.944100, 7.78254937903);
static const Atom Ho147("Ho", 67, 147, 0.000000, 146.939840, 7.83564776204);
static const Atom Ho148("Ho", 67, 148, 0.000000, 147.937270, 7.88883626522);
static const Atom Ho149("Ho", 67, 149, 0.000000, 148.933790, 7.94197624215);
static const Atom Ho150("Ho", 67, 150, 0.000000, 149.933350, 7.99527832875);
static const Atom Ho151("Ho", 67, 151, 0.000000, 150.931681, 8.04851487825);
static const Atom Ho152("Ho", 67, 152, 0.000000, 151.931740, 8.1018435743);
static const Atom Ho153("Ho", 67, 153, 0.000000, 152.930195, 8.15508673617);
static const Atom Ho154("Ho", 67, 154, 0.000000, 153.930596, 8.20843366956);
static const Atom Ho155("Ho", 67, 155, 0.000000, 154.929079, 8.26167832455);
static const Atom Ho156("Ho", 67, 156, 0.000000, 155.929710, 8.31503752282);
static const Atom Ho157("Ho", 67, 157, 0.000000, 156.928190, 8.36828201782);
static const Atom Ho158("Ho", 67, 158, 0.000000, 157.928950, 8.42164809509);
static const Atom Ho159("Ho", 67, 159, 0.000000, 158.927709, 8.47490746792);
static const Atom Ho160("Ho", 67, 160, 0.000000, 159.928726, 8.52828724985);
static const Atom Ho161("Ho", 67, 161, 0.000000, 160.927852, 8.58156619317);
static const Atom Ho162("Ho", 67, 162, 0.000000, 161.929092, 8.63495786669);
static const Atom Ho163("Ho", 67, 163, 0.000000, 162.928730, 8.68826411269);
static const Atom Ho164("Ho", 67, 164, 0.000000, 163.930231, 8.74166970418);
static const Atom Ho165("Ho", 67, 165, 100.000000, 164.930319, 8.79499994667);
static const Atom Ho166("Ho", 67, 166, 0.000000, 165.932281, 8.84843012125);
static const Atom Ho167("Ho", 67, 167, 0.000000, 166.933126, 8.90180073118);
static const Atom Ho168("Ho", 67, 168, 0.000000, 167.935500, 8.95525287588);
static const Atom Ho169("Ho", 67, 169, 0.000000, 168.936868, 9.00865137508);
static const Atom Ho170("Ho", 67, 170, 0.000000, 169.939610, 9.06212314358);
static const Atom Ho171("Ho", 67, 171, 0.000000, 170.941460, 9.11554734569);
static const Atom Ho172("Ho", 67, 172, 0.000000, 171.944820, 9.16905206938);
static const Atom Ho173("Ho", 67, 173, 0.000000, 172.947290, 9.22250933334);
static const Atom Ho174("Ho", 67, 174, 0.000000, 173.951150, 9.2760407198);
static const Atom Ho175("Ho", 67, 175, 0.000000, 174.954050, 9.32952091374);
static const Atom Er("Er", 68, 0, 0.000000, 167.259000, 9.066);
static const Atom Er144("Er", 68, 144, 0.000000, 143.960590, 7.80314786612);
static const Atom Er145("Er", 68, 145, 0.000000, 144.957460, 7.85718157086);
static const Atom Er146("Er", 68, 146, 0.000000, 145.952120, 7.91109548616);
static const Atom Er147("Er", 68, 147, 0.000000, 146.949310, 7.96514653597);
static const Atom Er148("Er", 68, 148, 0.000000, 147.944440, 8.01908592686);
static const Atom Er149("Er", 68, 149, 0.000000, 148.942170, 8.07316624648);
static const Atom Er150("Er", 68, 150, 0.000000, 149.937760, 8.12713057091);
static const Atom Er151("Er", 68, 151, 0.000000, 150.937460, 8.18131767116);
static const Atom Er152("Er", 68, 152, 0.000000, 151.935080, 8.23539202841);
static const Atom Er153("Er", 68, 153, 0.000000, 152.935093, 8.28959609431);
static const Atom Er154("Er", 68, 154, 0.000000, 153.932777, 8.34367392058);
static const Atom Er155("Er", 68, 155, 0.000000, 154.933200, 8.39790020985);
static const Atom Er156("Er", 68, 156, 0.000000, 155.931020, 8.45198540778);
static const Atom Er157("Er", 68, 157, 0.000000, 156.931950, 8.50623917816);
static const Atom Er158("Er", 68, 158, 0.000000, 157.929910, 8.56033196456);
static const Atom Er159("Er", 68, 159, 0.000000, 158.930681, 8.6145771166);
static const Atom Er160("Er", 68, 160, 0.000000, 159.929080, 8.66869369828);
static const Atom Er161("Er", 68, 161, 0.000000, 160.930001, 8.72294698083);
static const Atom Er162("Er", 68, 162, 0.140000, 161.928775, 8.77708388876);
static const Atom Er163("Er", 68, 163, 0.000000, 162.930029, 8.83135522103);
static const Atom Er164("Er", 68, 164, 1.610000, 163.929197, 8.88551348509);
static const Atom Er165("Er", 68, 165, 0.000000, 164.930723, 8.93979956067);
static const Atom Er166("Er", 68, 166, 33.610000, 165.930290, 8.99397945187);
static const Atom Er167("Er", 68, 167, 22.930000, 166.932045, 9.04827794002);
static const Atom Er168("Er", 68, 168, 26.780000, 167.932368, 9.10249880896);
static const Atom Er169("Er", 68, 169, 0.000000, 168.934588, 9.15682250168);
static const Atom Er170("Er", 68, 170, 14.930000, 169.935460, 9.21107312826);
static const Atom Er171("Er", 68, 171, 0.000000, 170.938026, 9.26541557534);
static const Atom Er172("Er", 68, 172, 0.000000, 171.939352, 9.31969081025);
static const Atom Er173("Er", 68, 173, 0.000000, 172.942400, 9.37405938335);
static const Atom Er174("Er", 68, 174, 0.000000, 173.944340, 9.42836789913);
static const Atom Er175("Er", 68, 175, 0.000000, 174.947930, 9.48276585045);
static const Atom Er176("Er", 68, 176, 0.000000, 175.950290, 9.53709713163);
static const Atom Er177("Er", 68, 177, 0.000000, 176.954370, 9.5915216426);
static const Atom Tm("Tm", 69, 0, 0.000000, 168.934210, 9.321);
static const Atom Tm146("Tm", 69, 146, 0.000000, 145.966500, 8.05374912814);
static const Atom Tm147("Tm", 69, 147, 0.000000, 146.961080, 8.10862540323);
static const Atom Tm148("Tm", 69, 148, 0.000000, 147.957550, 8.16360595968);
static const Atom Tm149("Tm", 69, 149, 0.000000, 148.952650, 8.21851092594);
static const Atom Tm150("Tm", 69, 150, 0.000000, 149.949670, 8.27352182882);
static const Atom Tm151("Tm", 69, 151, 0.000000, 150.945430, 8.3284632108);
static const Atom Tm152("Tm", 69, 152, 0.000000, 151.944300, 8.38357618803);
static const Atom Tm153("Tm", 69, 153, 0.000000, 152.942028, 8.43862615505);
static const Atom Tm154("Tm", 69, 154, 0.000000, 153.941420, 8.4937679338);
static const Atom Tm155("Tm", 69, 155, 0.000000, 154.939192, 8.54882032853);
static const Atom Tm156("Tm", 69, 156, 0.000000, 155.939010, 8.60398561197);
static const Atom Tm157("Tm", 69, 157, 0.000000, 156.936760, 8.65903679284);
static const Atom Tm158("Tm", 69, 158, 0.000000, 157.937000, 8.71422536027);
static const Atom Tm159("Tm", 69, 159, 0.000000, 158.934810, 8.76927985167);
static const Atom Tm160("Tm", 69, 160, 0.000000, 159.935090, 8.82447062611);
static const Atom Tm161("Tm", 69, 161, 0.000000, 160.933400, 8.87955270516);
static const Atom Tm162("Tm", 69, 162, 0.000000, 161.933970, 8.93475948045);
static const Atom Tm163("Tm", 69, 163, 0.000000, 162.932648, 8.98986186402);
static const Atom Tm164("Tm", 69, 164, 0.000000, 163.933451, 9.04508149516);
static const Atom Tm165("Tm", 69, 165, 0.000000, 164.932432, 9.10020059686);
static const Atom Tm166("Tm", 69, 166, 0.000000, 165.933553, 9.15543777375);
static const Atom Tm167("Tm", 69, 167, 0.000000, 166.932849, 9.21057425568);
static const Atom Tm168("Tm", 69, 168, 0.000000, 167.934170, 9.26582246763);
static const Atom Tm169("Tm", 69, 169, 100.000000, 168.934211, 9.32100005518);
static const Atom Tm170("Tm", 69, 170, 0.000000, 169.935798, 9.37626294377);
static const Atom Tm171("Tm", 69, 171, 0.000000, 170.936426, 9.43147291923);
static const Atom Tm172("Tm", 69, 172, 0.000000, 171.938396, 9.48675693997);
static const Atom Tm173("Tm", 69, 173, 0.000000, 172.939600, 9.54199869642);
static const Atom Tm174("Tm", 69, 174, 0.000000, 173.942160, 9.5973152706);
static const Atom Tm175("Tm", 69, 175, 0.000000, 174.943830, 9.65258273875);
static const Atom Tm176("Tm", 69, 176, 0.000000, 175.946990, 9.70793241813);
static const Atom Tm177("Tm", 69, 177, 0.000000, 176.949040, 9.7632208529);
static const Atom Tm178("Tm", 69, 178, 0.000000, 177.952640, 9.81859480942);
static const Atom Tm179("Tm", 69, 179, 0.000000, 178.955340, 9.87391910815);
static const Atom Yb("Yb", 70, 0, 0.000000, 173.040000, 6.966);
static const Atom Yb148("Yb", 70, 148, 0.000000, 147.966760, 5.95663690569);
static const Atom Yb149("Yb", 70, 149, 0.000000, 148.963480, 5.99676145215);
static const Atom Yb150("Yb", 70, 150, 0.000000, 149.957990, 6.03679703155);
static const Atom Yb151("Yb", 70, 151, 0.000000, 150.955250, 6.07694331657);
static const Atom Yb152("Yb", 70, 152, 0.000000, 151.950170, 6.11699540118);
static const Atom Yb153("Yb", 70, 153, 0.000000, 152.949210, 6.15721334293);
static const Atom Yb154("Yb", 70, 154, 0.000000, 153.946240, 6.19735036893);
static const Atom Yb155("Yb", 70, 155, 0.000000, 154.945790, 6.23758884154);
static const Atom Yb156("Yb", 70, 156, 0.000000, 155.942850, 6.27772707524);
static const Atom Yb157("Yb", 70, 157, 0.000000, 156.942660, 6.31797601456);
static const Atom Yb158("Yb", 70, 158, 0.000000, 157.939858, 6.35811980368);
static const Atom Yb159("Yb", 70, 159, 0.000000, 158.940150, 6.39838814667);
static const Atom Yb160("Yb", 70, 160, 0.000000, 159.937560, 6.43854047018);
static const Atom Yb161("Yb", 70, 161, 0.000000, 160.937850, 6.47880873266);
static const Atom Yb162("Yb", 70, 162, 0.000000, 161.935750, 6.5189807819);
static const Atom Yb163("Yb", 70, 163, 0.000000, 162.936270, 6.5592583034);
static const Atom Yb164("Yb", 70, 164, 0.000000, 163.934520, 6.59944444244);
static const Atom Yb165("Yb", 70, 165, 0.000000, 164.935398, 6.6397363758);
static const Atom Yb166("Yb", 70, 166, 0.000000, 165.933880, 6.67993185437);
static const Atom Yb167("Yb", 70, 167, 0.000000, 166.934947, 6.72023139622);
static const Atom Yb168("Yb", 70, 168, 0.130000, 167.933894, 6.76044559411);
static const Atom Yb169("Yb", 70, 169, 0.000000, 168.935187, 6.80075423395);
static const Atom Yb170("Yb", 70, 170, 3.040000, 169.934759, 6.8409935922);
static const Atom Yb171("Yb", 70, 171, 14.280000, 170.936322, 6.88131310132);
static const Atom Yb172("Yb", 70, 172, 21.830000, 171.936378, 6.92157193168);
static const Atom Yb173("Yb", 70, 173, 16.130000, 172.938207, 6.96190215308);
static const Atom Yb174("Yb", 70, 174, 31.830000, 173.938858, 7.00218496027);
static const Atom Yb175("Yb", 70, 175, 0.000000, 174.941272, 7.04253874385);
static const Atom Yb176("Yb", 70, 176, 12.760000, 175.942568, 7.08284748433);
static const Atom Yb177("Yb", 70, 177, 0.000000, 176.945257, 7.12321232236);
static const Atom Yb178("Yb", 70, 178, 0.000000, 177.946643, 7.16352470607);
static const Atom Yb179("Yb", 70, 179, 0.000000, 178.950170, 7.20392327913);
static const Atom Yb180("Yb", 70, 180, 0.000000, 179.952330, 7.24426682143);
static const Atom Yb181("Yb", 70, 181, 0.000000, 180.956150, 7.28467718967);
static const Atom Lu("Lu", 71, 0, 0.000000, 174.967000, 9.841);
static const Atom Lu150("Lu", 71, 150, 0.000000, 149.972670, 8.43519661119);
static const Atom Lu151("Lu", 71, 151, 0.000000, 150.967150, 8.49113103128);
static const Atom Lu152("Lu", 71, 152, 0.000000, 151.963610, 8.54717681626);
static const Atom Lu153("Lu", 71, 153, 0.000000, 152.958690, 8.60314498328);
static const Atom Lu154("Lu", 71, 154, 0.000000, 153.957100, 8.6593004458);
static const Atom Lu155("Lu", 71, 155, 0.000000, 154.954230, 8.71538391485);
static const Atom Lu156("Lu", 71, 156, 0.000000, 155.952910, 8.77155456349);
static const Atom Lu157("Lu", 71, 157, 0.000000, 156.950102, 8.82764151973);
static const Atom Lu158("Lu", 71, 158, 0.000000, 157.949170, 8.88383399138);
static const Atom Lu159("Lu", 71, 159, 0.000000, 158.946620, 8.9399354588);
static const Atom Lu160("Lu", 71, 160, 0.000000, 159.946020, 8.99614660376);
static const Atom Lu161("Lu", 71, 161, 0.000000, 160.943540, 9.05225200832);
static const Atom Lu162("Lu", 71, 162, 0.000000, 161.943220, 9.10847890185);
static const Atom Lu163("Lu", 71, 163, 0.000000, 162.941200, 9.16461017906);
static const Atom Lu164("Lu", 71, 164, 0.000000, 163.941220, 9.22085619585);
static const Atom Lu165("Lu", 71, 165, 0.000000, 164.939610, 9.27701053347);
static const Atom Lu166("Lu", 71, 166, 0.000000, 165.939760, 9.3332638621);
static const Atom Lu167("Lu", 71, 167, 0.000000, 166.938310, 9.3894271989);
static const Atom Lu168("Lu", 71, 168, 0.000000, 167.938700, 9.4456940263);
static const Atom Lu169("Lu", 71, 169, 0.000000, 168.937649, 9.50187980481);
static const Atom Lu170("Lu", 71, 170, 0.000000, 169.938472, 9.55817098625);
static const Atom Lu171("Lu", 71, 171, 0.000000, 170.937910, 9.61438426852);
static const Atom Lu172("Lu", 71, 172, 0.000000, 171.939082, 9.67069507943);
static const Atom Lu173("Lu", 71, 173, 0.000000, 172.938927, 9.72693125336);
static const Atom Lu174("Lu", 71, 174, 0.000000, 173.940334, 9.7832552537);
static const Atom Lu175("Lu", 71, 175, 97.410000, 174.940768, 9.83952457837);
static const Atom Lu176("Lu", 71, 176, 2.590000, 175.942682, 9.89587715111);
static const Atom Lu177("Lu", 71, 177, 0.000000, 176.943755, 9.95218237128);
static const Atom Lu178("Lu", 71, 178, 0.000000, 177.945951, 10.008550777);
static const Atom Lu179("Lu", 71, 179, 0.000000, 178.947324, 10.0648728931);
static const Atom Lu180("Lu", 71, 180, 0.000000, 179.949880, 10.1212615469);
static const Atom Lu181("Lu", 71, 181, 0.000000, 180.951970, 10.1776239906);
static const Atom Lu182("Lu", 71, 182, 0.000000, 181.955210, 10.234051116);
static const Atom Lu183("Lu", 71, 183, 0.000000, 182.957570, 10.2904287458);
static const Atom Lu184("Lu", 71, 184, 0.000000, 183.961170, 10.3468761193);
static const Atom Hf("Hf", 72, 0, 0.000000, 178.490000, 13.31);
static const Atom Hf154("Hf", 72, 154, 0.000000, 153.964250, 11.4811147263);
static const Atom Hf155("Hf", 72, 155, 0.000000, 154.962760, 11.5555736209);
static const Atom Hf156("Hf", 72, 156, 0.000000, 155.959250, 11.6298818841);
static const Atom Hf157("Hf", 72, 157, 0.000000, 156.958130, 11.7043683697);
static const Atom Hf158("Hf", 72, 158, 0.000000, 157.954650, 11.77867887);
static const Atom Hf159("Hf", 72, 159, 0.000000, 158.954000, 11.8532004034);
static const Atom Hf160("Hf", 72, 160, 0.000000, 159.950710, 11.927525072);
static const Atom Hf161("Hf", 72, 161, 0.000000, 160.950330, 12.0020667393);
static const Atom Hf162("Hf", 72, 162, 0.000000, 161.947203, 12.0764035628);
static const Atom Hf163("Hf", 72, 163, 0.000000, 162.947060, 12.1509629032);
static const Atom Hf164("Hf", 72, 164, 0.000000, 163.944420, 12.2253360424);
static const Atom Hf165("Hf", 72, 165, 0.000000, 164.944540, 12.2999149947);
static const Atom Hf166("Hf", 72, 166, 0.000000, 165.942250, 12.3743142333);
static const Atom Hf167("Hf", 72, 167, 0.000000, 166.942600, 12.4489103367);
static const Atom Hf168("Hf", 72, 168, 0.000000, 167.940630, 12.5233334377);
static const Atom Hf169("Hf", 72, 169, 0.000000, 168.941160, 12.5979429638);
static const Atom Hf170("Hf", 72, 170, 0.000000, 169.939650, 12.672400367);
static const Atom Hf171("Hf", 72, 171, 0.000000, 170.940490, 12.7470330097);
static const Atom Hf172("Hf", 72, 172, 0.000000, 171.939460, 12.8215262065);
static const Atom Hf173("Hf", 72, 173, 0.000000, 172.940650, 12.8961849487);
static const Atom Hf174("Hf", 72, 174, 0.160000, 173.940040, 12.970709465);
static const Atom Hf175("Hf", 72, 175, 0.000000, 174.941503, 13.0453885648);
static const Atom Hf176("Hf", 72, 176, 5.260000, 175.941402, 13.1199510222);
static const Atom Hf177("Hf", 72, 177, 18.600000, 176.943220, 13.1946566093);
static const Atom Hf178("Hf", 72, 178, 27.280000, 177.943698, 13.2692622353);
static const Atom Hf179("Hf", 72, 179, 13.620000, 178.945815, 13.3439901338);
static const Atom Hf180("Hf", 72, 180, 35.080000, 179.946549, 13.4186148497);
static const Atom Hf181("Hf", 72, 181, 0.000000, 180.949099, 13.4933750295);
static const Atom Hf182("Hf", 72, 182, 0.000000, 181.950553, 13.5680534508);
static const Atom Hf183("Hf", 72, 183, 0.000000, 182.953530, 13.6428454496);
static const Atom Hf184("Hf", 72, 184, 0.000000, 183.955450, 13.7175586279);
static const Atom Hf185("Hf", 72, 185, 0.000000, 184.958780, 13.79237695);
static const Atom Hf186("Hf", 72, 186, 0.000000, 185.960920, 13.8671065337);
static const Atom Ta("Ta", 73, 0, 0.000000, 180.947900, 16.654);
static const Atom Ta156("Ta", 73, 156, 0.000000, 155.971690, 14.3552510157);
static const Atom Ta157("Ta", 73, 157, 0.000000, 156.968150, 14.4469627451);
static const Atom Ta158("Ta", 73, 158, 0.000000, 157.966370, 14.5388364606);
static const Atom Ta159("Ta", 73, 159, 0.000000, 158.962910, 14.630555553);
static const Atom Ta160("Ta", 73, 160, 0.000000, 159.961360, 14.7224504371);
static const Atom Ta161("Ta", 73, 161, 0.000000, 160.958370, 14.8142127871);
static const Atom Ta162("Ta", 73, 162, 0.000000, 161.957150, 14.9061380436);
static const Atom Ta163("Ta", 73, 163, 0.000000, 162.954320, 14.9979151197);
static const Atom Ta164("Ta", 73, 164, 0.000000, 163.953570, 15.0898836338);
static const Atom Ta165("Ta", 73, 165, 0.000000, 164.950820, 15.1816680729);
static const Atom Ta166("Ta", 73, 166, 0.000000, 165.950470, 15.273673402);
static const Atom Ta167("Ta", 73, 167, 0.000000, 166.947970, 15.3654808505);
static const Atom Ta168("Ta", 73, 168, 0.000000, 167.947790, 15.457501826);
static const Atom Ta169("Ta", 73, 169, 0.000000, 168.945920, 15.5493672581);
static const Atom Ta170("Ta", 73, 170, 0.000000, 169.946090, 15.6414204468);
static const Atom Ta171("Ta", 73, 171, 0.000000, 170.944460, 15.7333079679);
static const Atom Ta172("Ta", 73, 172, 0.000000, 171.944740, 15.8253712807);
static const Atom Ta173("Ta", 73, 173, 0.000000, 172.943540, 15.9172983779);
static const Atom Ta174("Ta", 73, 174, 0.000000, 173.944170, 16.0093939039);
static const Atom Ta175("Ta", 73, 175, 0.000000, 174.943650, 16.1013835867);
static const Atom Ta176("Ta", 73, 176, 0.000000, 175.944740, 16.1935214499);
static const Atom Ta177("Ta", 73, 177, 0.000000, 176.944472, 16.2855343261);
static const Atom Ta178("Ta", 73, 178, 0.000000, 177.945750, 16.3776894924);
static const Atom Ta179("Ta", 73, 179, 0.000000, 178.945934, 16.4697439696);
static const Atom Ta180("Ta", 73, 180, 0.012000, 179.947466, 16.5619225134);
static const Atom Ta181("Ta", 73, 181, 99.988000, 180.947996, 16.6540088356);
static const Atom Ta182("Ta", 73, 182, 0.000000, 181.950152, 16.7462448108);
static const Atom Ta183("Ta", 73, 183, 0.000000, 182.951373, 16.838394731);
static const Atom Ta184("Ta", 73, 184, 0.000000, 183.954009, 16.9306748842);
static const Atom Ta185("Ta", 73, 185, 0.000000, 184.955559, 17.0228550847);
static const Atom Ta186("Ta", 73, 186, 0.000000, 185.958550, 17.1151679113);
static const Atom Ta187("Ta", 73, 187, 0.000000, 186.960410, 17.2073766434);
static const Atom Ta188("Ta", 73, 188, 0.000000, 187.963710, 17.2997179096);
static const Atom W("W", 74, 0, 0.000000, 183.840000, 19.3);
static const Atom W158("W", 74, 158, 0.000000, 157.973940, 16.584513936);
static const Atom W159("W", 74, 159, 0.000000, 158.972280, 16.6893222585);
static const Atom W160("W", 74, 160, 0.000000, 159.968370, 16.7938943701);
static const Atom W161("W", 74, 161, 0.000000, 160.967090, 16.8987425859);
static const Atom W162("W", 74, 162, 0.000000, 161.963340, 17.0033314948);
static const Atom W163("W", 74, 163, 0.000000, 162.962530, 17.1082290524);
static const Atom W164("W", 74, 164, 0.000000, 163.958980, 17.2128389578);
static const Atom W165("W", 74, 165, 0.000000, 164.958340, 17.3177543625);
static const Atom W166("W", 74, 166, 0.000000, 165.955020, 17.4223884138);
static const Atom W167("W", 74, 167, 0.000000, 166.954670, 17.5273342635);
static const Atom W168("W", 74, 168, 0.000000, 167.951860, 17.632021856);
static const Atom W169("W", 74, 169, 0.000000, 168.951760, 17.7369939513);
static const Atom W170("W", 74, 170, 0.000000, 169.949290, 17.8417172378);
static const Atom W171("W", 74, 171, 0.000000, 170.949460, 17.9467176784);
static const Atom W172("W", 74, 172, 0.000000, 171.947420, 18.0514861075);
static const Atom W173("W", 74, 173, 0.000000, 172.947830, 18.1565117439);
static const Atom W174("W", 74, 174, 0.000000, 173.946160, 18.2613190165);
static const Atom W175("W", 74, 175, 0.000000, 174.946770, 18.3663656495);
static const Atom W176("W", 74, 176, 0.000000, 175.945590, 18.4712243636);
static const Atom W177("W", 74, 177, 0.000000, 176.946620, 18.5763150892);
static const Atom W178("W", 74, 178, 0.000000, 177.945850, 18.6812168462);
static const Atom W179("W", 74, 179, 0.000000, 178.947072, 18.7863277285);
static const Atom W180("W", 74, 180, 0.120000, 179.946706, 18.8912718984);
static const Atom W181("W", 74, 181, 0.000000, 180.948198, 18.996411126);
static const Atom W182("W", 74, 182, 26.500000, 181.948206, 19.1013945594);
static const Atom W183("W", 74, 183, 14.310000, 182.950224, 19.2065890603);
static const Atom W184("W", 74, 184, 30.640000, 183.950933, 19.3116459921);
static const Atom W185("W", 74, 185, 0.000000, 184.953421, 19.4168897823);
static const Atom W186("W", 74, 186, 28.430000, 185.954362, 19.5219712065);
static const Atom W187("W", 74, 187, 0.000000, 186.957158, 19.6272473314);
static const Atom W188("W", 74, 188, 0.000000, 187.958487, 19.7323694468);
static const Atom W189("W", 74, 189, 0.000000, 188.961910, 19.8377113958);
static const Atom W190("W", 74, 190, 0.000000, 189.963180, 19.9428273172);
static const Atom Re("Re", 75, 0, 0.000000, 186.207000, 21.02);
static const Atom Re160("Re", 75, 160, 0.000000, 159.981490, 18.0595300918);
static const Atom Re161("Re", 75, 161, 0.000000, 160.977660, 18.1719828642);
static const Atom Re162("Re", 75, 162, 0.000000, 161.975710, 18.2846478607);
static const Atom Re163("Re", 75, 163, 0.000000, 162.971970, 18.3971107928);
static const Atom Re164("Re", 75, 164, 0.000000, 163.970320, 18.5098096548);
static const Atom Re165("Re", 75, 165, 0.000000, 164.967050, 18.622325643);
static const Atom Re166("Re", 75, 166, 0.000000, 165.965800, 18.735069659);
static const Atom Re167("Re", 75, 167, 0.000000, 166.962560, 18.8475890337);
static const Atom Re168("Re", 75, 168, 0.000000, 167.961610, 18.9603669153);
static const Atom Re169("Re", 75, 169, 0.000000, 168.958830, 19.0729382171);
static const Atom Re170("Re", 75, 170, 0.000000, 169.958160, 19.1857477066);
static const Atom Re171("Re", 75, 171, 0.000000, 170.955550, 19.2983381989);
static const Atom Re172("Re", 75, 172, 0.000000, 171.955290, 19.4111939712);
static const Atom Re173("Re", 75, 173, 0.000000, 172.953060, 19.5238273599);
static const Atom Re174("Re", 75, 174, 0.000000, 173.953110, 19.6367181266);
static const Atom Re175("Re", 75, 175, 0.000000, 174.951390, 19.7494090867);
static const Atom Re176("Re", 75, 176, 0.000000, 175.951570, 19.8623145285);
static const Atom Re177("Re", 75, 177, 0.000000, 176.950270, 19.9750529003);
static const Atom Re178("Re", 75, 178, 0.000000, 177.950850, 20.0880034961);
static const Atom Re179("Re", 75, 179, 0.000000, 178.949980, 20.2007904085);
static const Atom Re180("Re", 75, 180, 0.000000, 179.950790, 20.3137669679);
static const Atom Re181("Re", 75, 181, 0.000000, 180.950065, 20.4265702487);
static const Atom Re182("Re", 75, 182, 0.000000, 181.951210, 20.5395846246);
static const Atom Re183("Re", 75, 183, 0.000000, 182.950821, 20.6524258348);
static const Atom Re184("Re", 75, 184, 0.000000, 183.952524, 20.7655032006);
static const Atom Re185("Re", 75, 185, 37.400000, 184.952956, 20.8784370556);
static const Atom Re186("Re", 75, 186, 0.000000, 185.954987, 20.9915514816);
static const Atom Re187("Re", 75, 187, 62.600000, 186.955751, 21.1045228258);
static const Atom Re188("Re", 75, 188, 0.000000, 187.958112, 21.2176745264);
static const Atom Re189("Re", 75, 189, 0.000000, 188.959228, 21.3306855948);
static const Atom Re190("Re", 75, 190, 0.000000, 189.961820, 21.4438633156);
static const Atom Re191("Re", 75, 191, 0.000000, 190.963124, 21.5568956402);
static const Atom Re192("Re", 75, 192, 0.000000, 191.965960, 21.6701009049);
static const Atom Os("Os", 76, 0, 0.000000, 190.230000, 22.57);
static const Atom Os162("Os", 76, 162, 0.000000, 161.983820, 19.2187079714);
static const Atom Os163("Os", 76, 163, 0.000000, 162.982050, 19.337143818);
static const Atom Os164("Os", 76, 164, 0.000000, 163.977930, 19.4553008469);
static const Atom Os165("Os", 76, 165, 0.000000, 164.976480, 19.5737746601);
static const Atom Os166("Os", 76, 166, 0.000000, 165.972530, 19.6919518588);
static const Atom Os167("Os", 76, 167, 0.000000, 166.971550, 19.8104814356);
static const Atom Os168("Os", 76, 168, 0.000000, 167.967830, 19.9286859228);
static const Atom Os169("Os", 76, 169, 0.000000, 168.967080, 20.0472427882);
static const Atom Os170("Os", 76, 170, 0.000000, 169.963570, 20.165472191);
static const Atom Os171("Os", 76, 171, 0.000000, 170.963040, 20.2840551585);
static const Atom Os172("Os", 76, 172, 0.000000, 171.960080, 20.4023498165);
static const Atom Os173("Os", 76, 173, 0.000000, 172.959790, 20.520961259);
static const Atom Os174("Os", 76, 174, 0.000000, 173.957120, 20.6392903243);
static const Atom Os175("Os", 76, 175, 0.000000, 174.957080, 20.7579314283);
static const Atom Os176("Os", 76, 176, 0.000000, 175.954950, 20.8763245624);
static const Atom Os177("Os", 76, 177, 0.000000, 176.955050, 20.9949822767);
static const Atom Os178("Os", 76, 178, 0.000000, 177.953350, 21.1134264285);
static const Atom Os179("Os", 76, 179, 0.000000, 178.953950, 21.2321434658);
static const Atom Os180("Os", 76, 180, 0.000000, 179.952350, 21.3505994822);
static const Atom Os181("Os", 76, 181, 0.000000, 180.953270, 21.4693544861);
static const Atom Os182("Os", 76, 182, 0.000000, 181.952186, 21.5878717238);
static const Atom Os183("Os", 76, 183, 0.000000, 182.953110, 21.7066272023);
static const Atom Os184("Os", 76, 184, 0.020000, 183.952491, 21.8251996103);
static const Atom Os185("Os", 76, 185, 0.000000, 184.954043, 21.9440295984);
static const Atom Os186("Os", 76, 186, 1.590000, 185.953838, 22.0626511258);
static const Atom Os187("Os", 76, 187, 1.960000, 186.955748, 22.1815235773);
static const Atom Os188("Os", 76, 188, 13.240000, 187.955836, 22.3001798797);
static const Atom Os189("Os", 76, 189, 16.150000, 188.958145, 22.4190996709);
static const Atom Os190("Os", 76, 190, 26.260000, 189.958445, 22.5377811263);
static const Atom Os191("Os", 76, 191, 0.000000, 190.960928, 22.6567215737);
static const Atom Os192("Os", 76, 192, 40.780000, 191.961479, 22.7754327973);
static const Atom Os193("Os", 76, 193, 0.000000, 192.964148, 22.8943953128);
static const Atom Os194("Os", 76, 194, 0.000000, 193.965179, 23.0131634865);
static const Atom Os195("Os", 76, 195, 0.000000, 194.968120, 23.1321582737);
static const Atom Os196("Os", 76, 196, 0.000000, 195.969620, 23.2509820922);
static const Atom Ir("Ir", 77, 0, 0.000000, 192.217000, 22.42);
static const Atom Ir165("Ir", 77, 165, 0.000000, 164.987580, 19.2439874912);
static const Atom Ir166("Ir", 77, 166, 0.000000, 165.985510, 19.3603850554);
static const Atom Ir167("Ir", 77, 167, 0.000000, 166.981540, 19.4765610055);
static const Atom Ir168("Ir", 77, 168, 0.000000, 167.979970, 19.5930168892);
static const Atom Ir169("Ir", 77, 169, 0.000000, 168.976390, 19.7092383286);
static const Atom Ir170("Ir", 77, 170, 0.000000, 169.975030, 19.8257187065);
static const Atom Ir171("Ir", 77, 171, 0.000000, 170.971780, 19.9419786366);
static const Atom Ir172("Ir", 77, 172, 0.000000, 171.970640, 20.0584846751);
static const Atom Ir173("Ir", 77, 173, 0.000000, 172.967710, 20.1747819298);
static const Atom Ir174("Ir", 77, 174, 0.000000, 173.966800, 20.2913147953);
static const Atom Ir175("Ir", 77, 175, 0.000000, 174.964280, 20.4076598719);
static const Atom Ir176("Ir", 77, 176, 0.000000, 175.963510, 20.5242090668);
static const Atom Ir177("Ir", 77, 177, 0.000000, 176.961170, 20.6405751385);
static const Atom Ir178("Ir", 77, 178, 0.000000, 177.961080, 20.757203648);
static const Atom Ir179("Ir", 77, 179, 0.000000, 178.959150, 20.8736175416);
static const Atom Ir180("Ir", 77, 180, 0.000000, 179.959250, 20.9902682125);
static const Atom Ir181("Ir", 77, 181, 0.000000, 180.957640, 21.1067194306);
static const Atom Ir182("Ir", 77, 182, 0.000000, 181.958130, 21.2234155907);
static const Atom Ir183("Ir", 77, 183, 0.000000, 182.956810, 21.3399006342);
static const Atom Ir184("Ir", 77, 184, 0.000000, 183.957390, 21.4566072918);
static const Atom Ir185("Ir", 77, 185, 0.000000, 184.956590, 21.5731529875);
static const Atom Ir186("Ir", 77, 186, 0.000000, 185.957951, 21.6899507402);
static const Atom Ir187("Ir", 77, 187, 0.000000, 186.957361, 21.8065209301);
static const Atom Ir188("Ir", 77, 188, 0.000000, 187.958852, 21.9233338458);
static const Atom Ir189("Ir", 77, 189, 0.000000, 188.958716, 22.0399569899);
static const Atom Ir190("Ir", 77, 190, 0.000000, 189.960590, 22.1568145783);
static const Atom Ir191("Ir", 77, 191, 37.300000, 190.960591, 22.2734537019);
static const Atom Ir192("Ir", 77, 192, 0.000000, 191.962602, 22.3903272699);
static const Atom Ir193("Ir", 77, 193, 62.700000, 192.962924, 22.5070038346);
static const Atom Ir194("Ir", 77, 194, 0.000000, 193.965076, 22.6238938487);
static const Atom Ir195("Ir", 77, 195, 0.000000, 194.965977, 22.7406379474);
static const Atom Ir196("Ir", 77, 196, 0.000000, 195.968380, 22.8575572379);
static const Atom Ir197("Ir", 77, 197, 0.000000, 196.969636, 22.9743427435);
static const Atom Ir198("Ir", 77, 198, 0.000000, 197.972280, 23.091290144);
static const Atom Ir199("Ir", 77, 199, 0.000000, 198.973790, 23.2081052758);
static const Atom Pt("Pt", 78, 0, 0.000000, 195.078000, 21.45);
static const Atom Pt168("Pt", 78, 168, 0.000000, 167.988040, 18.4712958816);
static const Atom Pt169("Pt", 78, 169, 0.000000, 168.986420, 18.5810737705);
static const Atom Pt170("Pt", 78, 170, 0.000000, 169.982330, 18.690580068);
static const Atom Pt171("Pt", 78, 171, 0.000000, 170.981250, 18.8004173331);
static const Atom Pt172("Pt", 78, 172, 0.000000, 171.977380, 18.9099478209);
static const Atom Pt173("Pt", 78, 173, 0.000000, 172.976500, 19.0198070772);
static const Atom Pt174("Pt", 78, 174, 0.000000, 173.972811, 19.129357467);
static const Atom Pt175("Pt", 78, 175, 0.000000, 174.972280, 19.239255098);
static const Atom Pt176("Pt", 78, 176, 0.000000, 175.969000, 19.3488504598);
static const Atom Pt177("Pt", 78, 177, 0.000000, 176.968450, 19.4587460016);
static const Atom Pt178("Pt", 78, 178, 0.000000, 177.965710, 19.5684007397);
static const Atom Pt179("Pt", 78, 179, 0.000000, 178.965480, 19.6783314674);
static const Atom Pt180("Pt", 78, 180, 0.000000, 179.963220, 19.7880389844);
static const Atom Pt181("Pt", 78, 181, 0.000000, 180.963180, 19.8979906038);
static const Atom Pt182("Pt", 78, 182, 0.000000, 181.961270, 20.0077366054);
static const Atom Pt183("Pt", 78, 183, 0.000000, 182.961730, 20.1177432027);
static const Atom Pt184("Pt", 78, 184, 0.000000, 183.959900, 20.2274980008);
static const Atom Pt185("Pt", 78, 185, 0.000000, 184.960750, 20.337547481);
static const Atom Pt186("Pt", 78, 186, 0.000000, 185.959430, 20.4473583567);
static const Atom Pt187("Pt", 78, 187, 0.000000, 186.960560, 20.5574386246);
static const Atom Pt188("Pt", 78, 188, 0.000000, 187.959396, 20.6672666533);
static const Atom Pt189("Pt", 78, 189, 0.000000, 188.960832, 20.7773805678);
static const Atom Pt190("Pt", 78, 190, 0.014000, 189.959930, 20.887237405);
static const Atom Pt191("Pt", 78, 191, 0.000000, 190.961685, 20.9973863954);
static const Atom Pt192("Pt", 78, 192, 0.782000, 191.961035, 21.1072709416);
static const Atom Pt193("Pt", 78, 193, 0.000000, 192.962985, 21.2174413735);
static const Atom Pt194("Pt", 78, 194, 32.967000, 193.962664, 21.3273620952);
static const Atom Pt195("Pt", 78, 195, 33.832000, 194.964774, 21.43755012);
static const Atom Pt196("Pt", 78, 196, 25.242000, 195.964935, 21.5475238405);
static const Atom Pt197("Pt", 78, 197, 0.000000, 196.967323, 21.657742433);
static const Atom Pt198("Pt", 78, 198, 7.163000, 197.967876, 21.7677592563);
static const Atom Pt199("Pt", 78, 199, 0.000000, 198.970576, 21.8780121551);
static const Atom Pt200("Pt", 78, 200, 0.000000, 199.971424, 21.9880614154);
static const Atom Pt201("Pt", 78, 201, 0.000000, 200.974500, 22.0983556577);
static const Atom Pt202("Pt", 78, 202, 0.000000, 201.975740, 22.2084480208);
static const Atom Au("Au", 79, 0, 0.000000, 196.966550, 19.3);
static const Atom Au171("Au", 79, 171, 0.000000, 170.991770, 16.754830508);
static const Atom Au172("Au", 79, 172, 0.000000, 171.990110, 16.8526540319);
static const Atom Au173("Au", 79, 173, 0.000000, 172.986400, 16.950276684);
static const Atom Au174("Au", 79, 174, 0.000000, 173.984920, 17.0481178454);
static const Atom Au175("Au", 79, 175, 0.000000, 174.981550, 17.1457738129);
static const Atom Au176("Au", 79, 176, 0.000000, 175.980270, 17.2436345715);
static const Atom Au177("Au", 79, 177, 0.000000, 176.977220, 17.3413218945);
static const Atom Au178("Au", 79, 178, 0.000000, 177.975980, 17.4391865725);
static const Atom Au179("Au", 79, 179, 0.000000, 178.973410, 17.536920929);
static const Atom Au180("Au", 79, 180, 0.000000, 179.972400, 17.6348081438);
static const Atom Au181("Au", 79, 181, 0.000000, 180.969950, 17.7325542586);
static const Atom Au182("Au", 79, 182, 0.000000, 181.969620, 17.830508104);
static const Atom Au183("Au", 79, 183, 0.000000, 182.967620, 17.9282983126);
static const Atom Au184("Au", 79, 184, 0.000000, 183.967470, 18.0262697956);
static const Atom Au185("Au", 79, 185, 0.000000, 184.965810, 18.1240933194);
static const Atom Au186("Au", 79, 186, 0.000000, 185.966000, 18.2220981177);
static const Atom Au187("Au", 79, 187, 0.000000, 186.964560, 18.3199431985);
static const Atom Au188("Au", 79, 188, 0.000000, 187.965090, 18.4179813121);
static const Atom Au189("Au", 79, 189, 0.000000, 188.963890, 18.5158499095);
static const Atom Au190("Au", 79, 190, 0.000000, 189.964699, 18.6139153613);
static const Atom Au191("Au", 79, 191, 0.000000, 190.963650, 18.7117987547);
static const Atom Au192("Au", 79, 192, 0.000000, 191.964810, 18.8098985995);
static const Atom Au193("Au", 79, 193, 0.000000, 192.964132, 18.9078183458);
static const Atom Au194("Au", 79, 194, 0.000000, 193.965339, 19.005922796);
static const Atom Au195("Au", 79, 195, 0.000000, 194.965018, 19.1038775234);
static const Atom Au196("Au", 79, 196, 0.000000, 195.966551, 19.2020139171);
static const Atom Au197("Au", 79, 197, 100.000000, 196.966552, 19.300000196);
static const Atom Au198("Au", 79, 198, 0.000000, 197.968225, 19.3981503078);
static const Atom Au199("Au", 79, 199, 0.000000, 198.968748, 19.4961877354);
static const Atom Au200("Au", 79, 200, 0.000000, 199.970720, 19.5943671451);
static const Atom Au201("Au", 79, 201, 0.000000, 200.971641, 19.6924435713);
static const Atom Au202("Au", 79, 202, 0.000000, 201.973790, 19.7906403245);
static const Atom Au203("Au", 79, 203, 0.000000, 202.975137, 19.8887584927);
static const Atom Au204("Au", 79, 204, 0.000000, 203.977710, 19.9869967921);
static const Atom Au205("Au", 79, 205, 0.000000, 204.979610, 20.0851691467);
static const Atom Hg("Hg", 80, 0, 0.000000, 200.590000, 13.546);
static const Atom Hg175("Hg", 80, 175, 0.000000, 174.991410, 11.8173071432);
static const Atom Hg176("Hg", 80, 176, 0.000000, 175.987410, 11.8845678043);
static const Atom Hg177("Hg", 80, 177, 0.000000, 176.986340, 11.9520263305);
static const Atom Hg178("Hg", 80, 178, 0.000000, 177.982476, 12.0192961758);
static const Atom Hg179("Hg", 80, 179, 0.000000, 178.981780, 12.0867799585);
static const Atom Hg180("Hg", 80, 180, 0.000000, 179.978320, 12.1540770862);
static const Atom Hg181("Hg", 80, 181, 0.000000, 180.977810, 12.2215734297);
static const Atom Hg182("Hg", 80, 182, 0.000000, 181.974750, 12.2888975697);
static const Atom Hg183("Hg", 80, 183, 0.000000, 182.974560, 12.356415523);
static const Atom Hg184("Hg", 80, 184, 0.000000, 183.971900, 12.4237666753);
static const Atom Hg185("Hg", 80, 185, 0.000000, 184.971980, 12.491302862);
static const Atom Hg186("Hg", 80, 186, 0.000000, 185.969460, 12.5586634686);
static const Atom Hg187("Hg", 80, 187, 0.000000, 186.969790, 12.6262165379);
static const Atom Hg188("Hg", 80, 188, 0.000000, 187.967560, 12.6935967285);
static const Atom Hg189("Hg", 80, 189, 0.000000, 188.968130, 12.7611660052);
static const Atom Hg190("Hg", 80, 190, 0.000000, 189.966280, 12.8285718574);
static const Atom Hg191("Hg", 80, 191, 0.000000, 190.967060, 12.8961553156);
static const Atom Hg192("Hg", 80, 192, 0.000000, 191.965570, 12.9635854789);
static const Atom Hg193("Hg", 80, 193, 0.000000, 192.966644, 13.0311887912);
static const Atom Hg194("Hg", 80, 194, 0.000000, 193.965382, 13.0986343515);
static const Atom Hg195("Hg", 80, 195, 0.000000, 194.966640, 13.1662500894);
static const Atom Hg196("Hg", 80, 196, 0.150000, 195.965815, 13.2337251607);
static const Atom Hg197("Hg", 80, 197, 0.000000, 196.967195, 13.3013491374);
static const Atom Hg198("Hg", 80, 198, 9.970000, 197.966752, 13.3688500054);
static const Atom Hg199("Hg", 80, 199, 16.870000, 198.968262, 13.4364827611);
static const Atom Hg200("Hg", 80, 200, 23.100000, 199.968309, 13.5040167192);
static const Atom Hg201("Hg", 80, 201, 13.180000, 200.970285, 13.5716809443);
static const Atom Hg202("Hg", 80, 202, 29.860000, 201.970626, 13.6392347564);
static const Atom Hg203("Hg", 80, 203, 0.000000, 202.972857, 13.7069162018);
static const Atom Hg204("Hg", 80, 204, 6.870000, 203.973476, 13.7744887876);
static const Atom Hg205("Hg", 80, 205, 0.000000, 204.976056, 13.8421938012);
static const Atom Hg206("Hg", 80, 206, 0.000000, 205.977499, 13.9098220323);
static const Atom Hg207("Hg", 80, 207, 0.000000, 206.982580, 13.9776959404);
static const Atom Hg208("Hg", 80, 208, 0.000000, 207.985940, 14.045453628);
static const Atom Tl("Tl", 81, 0, 0.000000, 204.383300, 11.85);
static const Atom Tl177("Tl", 81, 177, 0.000000, 176.996880, 10.2621546281);
static const Atom Tl178("Tl", 81, 178, 0.000000, 177.995230, 10.320038259);
static const Atom Tl179("Tl", 81, 179, 0.000000, 178.991470, 10.3777995536);
static const Atom Tl180("Tl", 81, 180, 0.000000, 179.990190, 10.4357046368);
static const Atom Tl181("Tl", 81, 181, 0.000000, 180.986900, 10.4934931817);
static const Atom Tl182("Tl", 81, 182, 0.000000, 181.985610, 10.5513976851);
static const Atom Tl183("Tl", 81, 183, 0.000000, 182.982700, 10.6092082621);
static const Atom Tl184("Tl", 81, 184, 0.000000, 183.981760, 10.6671330583);
static const Atom Tl185("Tl", 81, 185, 0.000000, 184.979100, 10.7249581301);
static const Atom Tl186("Tl", 81, 186, 0.000000, 185.978550, 10.7829055383);
static const Atom Tl187("Tl", 81, 187, 0.000000, 186.976170, 10.8407468443);
static const Atom Tl188("Tl", 81, 188, 0.000000, 187.975920, 10.8987116462);
static const Atom Tl189("Tl", 81, 189, 0.000000, 188.973690, 10.9565616491);
static const Atom Tl190("Tl", 81, 190, 0.000000, 189.973790, 11.0145467438);
static const Atom Tl191("Tl", 81, 191, 0.000000, 190.971890, 11.0724158799);
static const Atom Tl192("Tl", 81, 192, 0.000000, 191.972140, 11.1304096714);
static const Atom Tl193("Tl", 81, 193, 0.000000, 192.970550, 11.1882967811);
static const Atom Tl194("Tl", 81, 194, 0.000000, 193.971050, 11.2463050675);
static const Atom Tl195("Tl", 81, 195, 0.000000, 194.969650, 11.3042031932);
static const Atom Tl196("Tl", 81, 196, 0.000000, 195.970520, 11.3622329319);
static const Atom Tl197("Tl", 81, 197, 0.000000, 196.969540, 11.420155409);
static const Atom Tl198("Tl", 81, 198, 0.000000, 197.970470, 11.4781886265);
static const Atom Tl199("Tl", 81, 199, 0.000000, 198.969810, 11.5361296569);
static const Atom Tl200("Tl", 81, 200, 0.000000, 199.970945, 11.5941747601);
static const Atom Tl201("Tl", 81, 201, 0.000000, 200.970804, 11.6521458818);
static const Atom Tl202("Tl", 81, 202, 0.000000, 201.972091, 11.7101997979);
static const Atom Tl203("Tl", 81, 203, 29.524000, 202.972329, 11.7681928937);
static const Atom Tl204("Tl", 81, 204, 0.000000, 203.973849, 11.826260319);
static const Atom Tl205("Tl", 81, 205, 70.476000, 204.974412, 11.8842722581);
static const Atom Tl206("Tl", 81, 206, 0.000000, 205.976095, 11.942349134);
static const Atom Tl207("Tl", 81, 207, 0.000000, 206.977408, 12.0004045575);
static const Atom Tl208("Tl", 81, 208, 0.000000, 207.982005, 12.0586503851);
static const Atom Tl209("Tl", 81, 209, 0.000000, 208.985349, 12.1168235646);
static const Atom Tl210("Tl", 81, 210, 0.000000, 209.990066, 12.1750763497);
static const Atom Pb("Pb", 82, 0, 0.000000, 207.200000, 11.35);
static const Atom Pb181("Pb", 82, 181, 0.000000, 180.996710, 9.91463638272);
static const Atom Pb182("Pb", 82, 182, 0.000000, 181.992676, 9.96919340058);
static const Atom Pb183("Pb", 82, 183, 0.000000, 182.991930, 10.0239305285);
static const Atom Pb184("Pb", 82, 184, 0.000000, 183.988200, 10.0785041988);
static const Atom Pb185("Pb", 82, 185, 0.000000, 184.987580, 10.1332482288);
static const Atom Pb186("Pb", 82, 186, 0.000000, 185.984300, 10.1878465492);
static const Atom Pb187("Pb", 82, 187, 0.000000, 186.984030, 10.2426097514);
static const Atom Pb188("Pb", 82, 188, 0.000000, 187.981060, 10.2972250531);
static const Atom Pb189("Pb", 82, 189, 0.000000, 188.980880, 10.3519931853);
static const Atom Pb190("Pb", 82, 190, 0.000000, 189.978180, 10.406623277);
static const Atom Pb191("Pb", 82, 191, 0.000000, 190.978200, 10.4614023649);
static const Atom Pb192("Pb", 82, 192, 0.000000, 191.975760, 10.5160466988);
static const Atom Pb193("Pb", 82, 193, 0.000000, 192.976080, 10.5708422201);
static const Atom Pb194("Pb", 82, 194, 0.000000, 193.973970, 10.6255046308);
static const Atom Pb195("Pb", 82, 195, 0.000000, 194.974470, 10.6803100121);
static const Atom Pb196("Pb", 82, 196, 0.000000, 195.972710, 10.7349915951);
static const Atom Pb197("Pb", 82, 197, 0.000000, 196.973380, 10.7898062886);
static const Atom Pb198("Pb", 82, 198, 0.000000, 197.971980, 10.8445075917);
static const Atom Pb199("Pb", 82, 199, 0.000000, 198.972910, 10.8993365275);
static const Atom Pb200("Pb", 82, 200, 0.000000, 199.971816, 10.9540545927);
static const Atom Pb201("Pb", 82, 201, 0.000000, 200.972850, 11.0088892254);
static const Atom Pb202("Pb", 82, 202, 0.000000, 201.972144, 11.0636285444);
static const Atom Pb203("Pb", 82, 203, 0.000000, 202.973375, 11.1184739684);
static const Atom Pb204("Pb", 82, 204, 1.400000, 203.973029, 11.1732330075);
static const Atom Pb205("Pb", 82, 205, 0.000000, 204.974467, 11.2280897705);
static const Atom Pb206("Pb", 82, 206, 24.100000, 205.974449, 11.2828667768);
static const Atom Pb207("Pb", 82, 207, 22.100000, 206.975881, 11.3377232111);
static const Atom Pb208("Pb", 82, 208, 52.400000, 207.976636, 11.3925425608);
static const Atom Pb209("Pb", 82, 209, 0.000000, 208.981075, 11.4475637126);
static const Atom Pb210("Pb", 82, 210, 0.000000, 209.984173, 11.5025114071);
static const Atom Pb211("Pb", 82, 211, 0.000000, 210.988731, 11.5575390775);
static const Atom Pb212("Pb", 82, 212, 0.000000, 211.991887, 11.6124899765);
static const Atom Pb213("Pb", 82, 213, 0.000000, 212.996500, 11.6675206322);
static const Atom Pb214("Pb", 82, 214, 0.000000, 213.999798, 11.7224792878);
static const Atom Bi("Bi", 83, 0, 0.000000, 208.980380, 9.747);
static const Atom Bi185("Bi", 83, 185, 0.000000, 184.997710, 8.62843047453);
static const Atom Bi186("Bi", 83, 186, 0.000000, 185.996480, 8.67501384848);
static const Atom Bi187("Bi", 83, 187, 0.000000, 186.993460, 8.7215137355);
static const Atom Bi188("Bi", 83, 188, 0.000000, 187.992170, 8.76809431101);
static const Atom Bi189("Bi", 83, 189, 0.000000, 188.989510, 8.8146109887);
static const Atom Bi190("Bi", 83, 190, 0.000000, 189.988520, 8.86120555643);
static const Atom Bi191("Bi", 83, 191, 0.000000, 190.986050, 8.90773109586);
static const Atom Bi192("Bi", 83, 192, 0.000000, 191.985370, 8.95434012222);
static const Atom Bi193("Bi", 83, 193, 0.000000, 192.983060, 9.00087312417);
static const Atom Bi194("Bi", 83, 194, 0.000000, 193.982750, 9.0474994076);
static const Atom Bi195("Bi", 83, 195, 0.000000, 194.980750, 9.09404686818);
static const Atom Bi196("Bi", 83, 196, 0.000000, 195.980610, 9.14068108054);
static const Atom Bi197("Bi", 83, 197, 0.000000, 196.978930, 9.18724346616);
static const Atom Bi198("Bi", 83, 198, 0.000000, 197.979020, 9.23388840589);
static const Atom Bi199("Bi", 83, 199, 0.000000, 198.977580, 9.28046198528);
static const Atom Bi200("Bi", 83, 200, 0.000000, 199.978140, 9.32712884616);
static const Atom Bi201("Bi", 83, 201, 0.000000, 200.976970, 9.37371501856);
static const Atom Bi202("Bi", 83, 202, 0.000000, 201.977670, 9.42038840914);
static const Atom Bi203("Bi", 83, 203, 0.000000, 202.976868, 9.46699174533);
static const Atom Bi204("Bi", 83, 204, 0.000000, 203.977805, 9.51367618977);
static const Atom Bi205("Bi", 83, 205, 0.000000, 204.977375, 9.56029687631);
static const Atom Bi206("Bi", 83, 206, 0.000000, 205.978483, 9.60698929632);
static const Atom Bi207("Bi", 83, 207, 0.000000, 206.978455, 9.65362873244);
static const Atom Bi208("Bi", 83, 208, 0.000000, 207.979727, 9.70032880153);
static const Atom Bi209("Bi", 83, 209, 100.000000, 208.980383, 9.74700013992);
static const Atom Bi210("Bi", 83, 210, 0.000000, 209.984105, 9.79381447883);
static const Atom Bi211("Bi", 83, 211, 0.000000, 210.987258, 9.84060227915);
static const Atom Bi212("Bi", 83, 212, 0.000000, 211.991272, 9.88743023715);
static const Atom Bi213("Bi", 83, 213, 0.000000, 212.994375, 9.93421570544);
static const Atom Bi214("Bi", 83, 214, 0.000000, 213.998699, 9.98105812207);
static const Atom Bi215("Bi", 83, 215, 0.000000, 215.001830, 10.0278448963);
static const Atom Bi216("Bi", 83, 216, 0.000000, 216.006200, 10.0746894584);
static const Atom Po("Po", 84, 0, 0.000000, 209.000000, 9.32);
static const Atom Po190("Po", 84, 190, 0.000000, 189.995110, 8.47250921148);
static const Atom Po191("Po", 84, 191, 0.000000, 190.994650, 8.517082);
static const Atom Po192("Po", 84, 192, 0.000000, 191.991520, 8.5615357244);
static const Atom Po193("Po", 84, 193, 0.000000, 192.991100, 8.60611029665);
static const Atom Po194("Po", 84, 194, 0.000000, 193.988280, 8.65057784498);
static const Atom Po195("Po", 84, 195, 0.000000, 194.988050, 8.69516088995);
static const Atom Po196("Po", 84, 196, 0.000000, 195.985510, 8.7396409244);
static const Atom Po197("Po", 84, 197, 0.000000, 196.985570, 8.78423690144);
static const Atom Po198("Po", 84, 198, 0.000000, 197.983340, 8.82873075981);
static const Atom Po199("Po", 84, 199, 0.000000, 198.983600, 8.8733356555);
static const Atom Po200("Po", 84, 200, 0.000000, 199.981740, 8.9178460134);
static const Atom Po201("Po", 84, 201, 0.000000, 200.982210, 8.96246027368);
static const Atom Po202("Po", 84, 202, 0.000000, 201.980700, 9.00698623923);
static const Atom Po203("Po", 84, 203, 0.000000, 202.981410, 9.05161120191);
static const Atom Po204("Po", 84, 204, 0.000000, 203.980307, 9.09615531694);
static const Atom Po205("Po", 84, 205, 0.000000, 204.981170, 9.14078710239);
static const Atom Po206("Po", 84, 206, 0.000000, 205.980465, 9.18534896555);
static const Atom Po207("Po", 84, 207, 0.000000, 206.981578, 9.22999189933);
static const Atom Po208("Po", 84, 208, 0.000000, 207.981231, 9.27456972689);
static const Atom Po209("Po", 84, 209, 0.000000, 208.982416, 9.31921587139);
static const Atom Po210("Po", 84, 210, 0.000000, 209.982857, 9.36382883847);
static const Atom Po211("Po", 84, 211, 0.000000, 210.986637, 9.40859070258);
static const Atom Po212("Po", 84, 212, 0.000000, 211.988852, 9.45328277818);
static const Atom Po213("Po", 84, 213, 0.000000, 212.992843, 9.49805405148);
static const Atom Po214("Po", 84, 214, 0.000000, 213.995186, 9.54275183502);
static const Atom Po215("Po", 84, 215, 0.000000, 214.999415, 9.58753372153);
static const Atom Po216("Po", 84, 216, 0.000000, 216.001905, 9.63223806921);
static const Atom Po217("Po", 84, 217, 0.000000, 217.006250, 9.67702511962);
static const Atom Po218("Po", 84, 218, 0.000000, 218.008966, 9.72173952754);
static const Atom At("At", 85, 0, 0.000000, 210.000000, NAN);
static const Atom At193("At", 85, 193, 0.000000, 193.000190, NAN);
static const Atom At194("At", 85, 194, 0.000000, 193.998970, NAN);
static const Atom At195("At", 85, 195, 0.000000, 194.996550, NAN);
static const Atom At196("At", 85, 196, 0.000000, 195.995700, NAN);
static const Atom At197("At", 85, 197, 0.000000, 196.993290, NAN);
static const Atom At198("At", 85, 198, 0.000000, 197.992750, NAN);
static const Atom At199("At", 85, 199, 0.000000, 198.990630, NAN);
static const Atom At200("At", 85, 200, 0.000000, 199.990290, NAN);
static const Atom At201("At", 85, 201, 0.000000, 200.988490, NAN);
static const Atom At202("At", 85, 202, 0.000000, 201.988450, NAN);
static const Atom At203("At", 85, 203, 0.000000, 202.986850, NAN);
static const Atom At204("At", 85, 204, 0.000000, 203.987260, NAN);
static const Atom At205("At", 85, 205, 0.000000, 204.986040, NAN);
static const Atom At206("At", 85, 206, 0.000000, 205.986600, NAN);
static const Atom At207("At", 85, 207, 0.000000, 206.985776, NAN);
static const Atom At208("At", 85, 208, 0.000000, 207.986583, NAN);
static const Atom At209("At", 85, 209, 0.000000, 208.986159, NAN);
static const Atom At210("At", 85, 210, 0.000000, 209.987131, NAN);
static const Atom At211("At", 85, 211, 0.000000, 210.987481, NAN);
static const Atom At212("At", 85, 212, 0.000000, 211.990735, NAN);
static const Atom At213("At", 85, 213, 0.000000, 212.992921, NAN);
static const Atom At214("At", 85, 214, 0.000000, 213.996356, NAN);
static const Atom At215("At", 85, 215, 0.000000, 214.998641, NAN);
static const Atom At216("At", 85, 216, 0.000000, 216.002409, NAN);
static const Atom At217("At", 85, 217, 0.000000, 217.004710, NAN);
static const Atom At218("At", 85, 218, 0.000000, 218.008681, NAN);
static const Atom At219("At", 85, 219, 0.000000, 219.011300, NAN);
static const Atom At220("At", 85, 220, 0.000000, 220.015300, NAN);
static const Atom At221("At", 85, 221, 0.000000, 221.018140, NAN);
static const Atom At222("At", 85, 222, 0.000000, 222.022330, NAN);
static const Atom At223("At", 85, 223, 0.000000, 223.025340, NAN);
static const Atom Rn("Rn", 86, 0, 0.000000, 222.000000, NAN);
static const Atom Rn196("Rn", 86, 196, 0.000000, 196.002310, NAN);
static const Atom Rn197("Rn", 86, 197, 0.000000, 197.001660, NAN);
static const Atom Rn198("Rn", 86, 198, 0.000000, 197.998780, NAN);
static const Atom Rn199("Rn", 86, 199, 0.000000, 198.998310, NAN);
static const Atom Rn200("Rn", 86, 200, 0.000000, 199.995680, NAN);
static const Atom Rn201("Rn", 86, 201, 0.000000, 200.995540, NAN);
static const Atom Rn202("Rn", 86, 202, 0.000000, 201.993220, NAN);
static const Atom Rn203("Rn", 86, 203, 0.000000, 202.993320, NAN);
static const Atom Rn204("Rn", 86, 204, 0.000000, 203.991370, NAN);
static const Atom Rn205("Rn", 86, 205, 0.000000, 204.991670, NAN);
static const Atom Rn206("Rn", 86, 206, 0.000000, 205.990160, NAN);
static const Atom Rn207("Rn", 86, 207, 0.000000, 206.990730, NAN);
static const Atom Rn208("Rn", 86, 208, 0.000000, 207.989631, NAN);
static const Atom Rn209("Rn", 86, 209, 0.000000, 208.990380, NAN);
static const Atom Rn210("Rn", 86, 210, 0.000000, 209.989680, NAN);
static const Atom Rn211("Rn", 86, 211, 0.000000, 210.990585, NAN);
static const Atom Rn212("Rn", 86, 212, 0.000000, 211.990689, NAN);
static const Atom Rn213("Rn", 86, 213, 0.000000, 212.993868, NAN);
static const Atom Rn214("Rn", 86, 214, 0.000000, 213.995346, NAN);
static const Atom Rn215("Rn", 86, 215, 0.000000, 214.998729, NAN);
static const Atom Rn216("Rn", 86, 216, 0.000000, 216.000258, NAN);
static const Atom Rn217("Rn", 86, 217, 0.000000, 217.003915, NAN);
static const Atom Rn218("Rn", 86, 218, 0.000000, 218.005586, NAN);
static const Atom Rn219("Rn", 86, 219, 0.000000, 219.009475, NAN);
static const Atom Rn220("Rn", 86, 220, 0.000000, 220.011384, NAN);
static const Atom Rn221("Rn", 86, 221, 0.000000, 221.015460, NAN);
static const Atom Rn222("Rn", 86, 222, 0.000000, 222.017571, NAN);
static const Atom Rn223("Rn", 86, 223, 0.000000, 223.021790, NAN);
static const Atom Rn224("Rn", 86, 224, 0.000000, 224.024090, NAN);
static const Atom Rn225("Rn", 86, 225, 0.000000, 225.028440, NAN);
static const Atom Rn226("Rn", 86, 226, 0.000000, 226.030890, NAN);
static const Atom Rn227("Rn", 86, 227, 0.000000, 227.035410, NAN);
static const Atom Rn228("Rn", 86, 228, 0.000000, 228.038080, NAN);
static const Atom Fr("Fr", 87, 0, 0.000000, 223.000000, NAN);
static const Atom Fr200("Fr", 87, 200, 0.000000, 200.006500, NAN);
static const Atom Fr201("Fr", 87, 201, 0.000000, 201.003990, NAN);
static const Atom Fr202("Fr", 87, 202, 0.000000, 202.003290, NAN);
static const Atom Fr203("Fr", 87, 203, 0.000000, 203.001050, NAN);
static const Atom Fr204("Fr", 87, 204, 0.000000, 204.000590, NAN);
static const Atom Fr205("Fr", 87, 205, 0.000000, 204.998660, NAN);
static const Atom Fr206("Fr", 87, 206, 0.000000, 205.998490, NAN);
static const Atom Fr207("Fr", 87, 207, 0.000000, 206.996860, NAN);
static const Atom Fr208("Fr", 87, 208, 0.000000, 207.997130, NAN);
static const Atom Fr209("Fr", 87, 209, 0.000000, 208.995920, NAN);
static const Atom Fr210("Fr", 87, 210, 0.000000, 209.996398, NAN);
static const Atom Fr211("Fr", 87, 211, 0.000000, 210.995529, NAN);
static const Atom Fr212("Fr", 87, 212, 0.000000, 211.996195, NAN);
static const Atom Fr213("Fr", 87, 213, 0.000000, 212.996175, NAN);
static const Atom Fr214("Fr", 87, 214, 0.000000, 213.998955, NAN);
static const Atom Fr215("Fr", 87, 215, 0.000000, 215.000326, NAN);
static const Atom Fr216("Fr", 87, 216, 0.000000, 216.003188, NAN);
static const Atom Fr217("Fr", 87, 217, 0.000000, 217.004616, NAN);
static const Atom Fr218("Fr", 87, 218, 0.000000, 218.007563, NAN);
static const Atom Fr219("Fr", 87, 219, 0.000000, 219.009241, NAN);
static const Atom Fr220("Fr", 87, 220, 0.000000, 220.012313, NAN);
static const Atom Fr221("Fr", 87, 221, 0.000000, 221.014246, NAN);
static const Atom Fr222("Fr", 87, 222, 0.000000, 222.017544, NAN);
static const Atom Fr223("Fr", 87, 223, 0.000000, 223.019731, NAN);
static const Atom Fr224("Fr", 87, 224, 0.000000, 224.023240, NAN);
static const Atom Fr225("Fr", 87, 225, 0.000000, 225.025607, NAN);
static const Atom Fr226("Fr", 87, 226, 0.000000, 226.029340, NAN);
static const Atom Fr227("Fr", 87, 227, 0.000000, 227.031830, NAN);
static const Atom Fr228("Fr", 87, 228, 0.000000, 228.035720, NAN);
static const Atom Fr229("Fr", 87, 229, 0.000000, 229.038430, NAN);
static const Atom Fr230("Fr", 87, 230, 0.000000, 230.042510, NAN);
static const Atom Fr231("Fr", 87, 231, 0.000000, 231.045410, NAN);
static const Atom Fr232("Fr", 87, 232, 0.000000, 232.049650, NAN);
static const Atom Ra("Ra", 88, 0, 0.000000, 226.000000, NAN);
static const Atom Ra203("Ra", 88, 203, 0.000000, 203.009210, NAN);
static const Atom Ra204("Ra", 88, 204, 0.000000, 204.006480, NAN);
static const Atom Ra205("Ra", 88, 205, 0.000000, 205.006190, NAN);
static const Atom Ra206("Ra", 88, 206, 0.000000, 206.003780, NAN);
static const Atom Ra207("Ra", 88, 207, 0.000000, 207.003730, NAN);
static const Atom Ra208("Ra", 88, 208, 0.000000, 208.001780, NAN);
static const Atom Ra209("Ra", 88, 209, 0.000000, 209.001940, NAN);
static const Atom Ra210("Ra", 88, 210, 0.000000, 210.000450, NAN);
static const Atom Ra211("Ra", 88, 211, 0.000000, 211.000890, NAN);
static const Atom Ra212("Ra", 88, 212, 0.000000, 211.999783, NAN);
static const Atom Ra213("Ra", 88, 213, 0.000000, 213.000350, NAN);
static const Atom Ra214("Ra", 88, 214, 0.000000, 214.000091, NAN);
static const Atom Ra215("Ra", 88, 215, 0.000000, 215.002704, NAN);
static const Atom Ra216("Ra", 88, 216, 0.000000, 216.003518, NAN);
static const Atom Ra217("Ra", 88, 217, 0.000000, 217.006306, NAN);
static const Atom Ra218("Ra", 88, 218, 0.000000, 218.007124, NAN);
static const Atom Ra219("Ra", 88, 219, 0.000000, 219.010069, NAN);
static const Atom Ra220("Ra", 88, 220, 0.000000, 220.011015, NAN);
static const Atom Ra221("Ra", 88, 221, 0.000000, 221.013908, NAN);
static const Atom Ra222("Ra", 88, 222, 0.000000, 222.015362, NAN);
static const Atom Ra223("Ra", 88, 223, 0.000000, 223.018497, NAN);
static const Atom Ra224("Ra", 88, 224, 0.000000, 224.020202, NAN);
static const Atom Ra225("Ra", 88, 225, 0.000000, 225.023604, NAN);
static const Atom Ra226("Ra", 88, 226, 0.000000, 226.025403, NAN);
static const Atom Ra227("Ra", 88, 227, 0.000000, 227.029171, NAN);
static const Atom Ra228("Ra", 88, 228, 0.000000, 228.031064, NAN);
static const Atom Ra229("Ra", 88, 229, 0.000000, 229.034820, NAN);
static const Atom Ra230("Ra", 88, 230, 0.000000, 230.037080, NAN);
static const Atom Ra231("Ra", 88, 231, 0.000000, 231.041220, NAN);
static const Atom Ra232("Ra", 88, 232, 0.000000, 232.043690, NAN);
static const Atom Ra233("Ra", 88, 233, 0.000000, 233.048000, NAN);
static const Atom Ra234("Ra", 88, 234, 0.000000, 234.050550, NAN);
static const Atom Ac("Ac", 89, 0, 0.000000, 227.000000, NAN);
static const Atom Ac207("Ac", 89, 207, 0.000000, 207.012090, NAN);
static const Atom Ac208("Ac", 89, 208, 0.000000, 208.011490, NAN);
static const Atom Ac209("Ac", 89, 209, 0.000000, 209.009570, NAN);
static const Atom Ac210("Ac", 89, 210, 0.000000, 210.009260, NAN);
static const Atom Ac211("Ac", 89, 211, 0.000000, 211.007650, NAN);
static const Atom Ac212("Ac", 89, 212, 0.000000, 212.007810, NAN);
static const Atom Ac213("Ac", 89, 213, 0.000000, 213.006570, NAN);
static const Atom Ac214("Ac", 89, 214, 0.000000, 214.006890, NAN);
static const Atom Ac215("Ac", 89, 215, 0.000000, 215.006450, NAN);
static const Atom Ac216("Ac", 89, 216, 0.000000, 216.008721, NAN);
static const Atom Ac217("Ac", 89, 217, 0.000000, 217.009333, NAN);
static const Atom Ac218("Ac", 89, 218, 0.000000, 218.011630, NAN);
static const Atom Ac219("Ac", 89, 219, 0.000000, 219.012400, NAN);
static const Atom Ac220("Ac", 89, 220, 0.000000, 220.014750, NAN);
static const Atom Ac221("Ac", 89, 221, 0.000000, 221.015580, NAN);
static const Atom Ac222("Ac", 89, 222, 0.000000, 222.017829, NAN);
static const Atom Ac223("Ac", 89, 223, 0.000000, 223.019126, NAN);
static const Atom Ac224("Ac", 89, 224, 0.000000, 224.021708, NAN);
static const Atom Ac225("Ac", 89, 225, 0.000000, 225.023221, NAN);
static const Atom Ac226("Ac", 89, 226, 0.000000, 226.026090, NAN);
static const Atom Ac227("Ac", 89, 227, 0.000000, 227.027747, NAN);
static const Atom Ac228("Ac", 89, 228, 0.000000, 228.031015, NAN);
static const Atom Ac229("Ac", 89, 229, 0.000000, 229.032930, NAN);
static const Atom Ac230("Ac", 89, 230, 0.000000, 230.036030, NAN);
static const Atom Ac231("Ac", 89, 231, 0.000000, 231.038550, NAN);
static const Atom Ac232("Ac", 89, 232, 0.000000, 232.042020, NAN);
static const Atom Ac233("Ac", 89, 233, 0.000000, 233.044550, NAN);
static const Atom Ac234("Ac", 89, 234, 0.000000, 234.048420, NAN);
static const Atom Ac235("Ac", 89, 235, 0.000000, 235.051100, NAN);
static const Atom Ac236("Ac", 89, 236, 0.000000, 236.055180, NAN);
static const Atom Th("Th", 90, 0, 0.000000, 232.038100, 11.72);
static const Atom Th210("Th", 90, 210, 0.000000, 210.015030, 10.6076379336);
static const Atom Th211("Th", 90, 211, 0.000000, 211.014860, 10.6581382937);
static const Atom Th212("Th", 90, 212, 0.000000, 212.012920, 10.7085492529);
static const Atom Th213("Th", 90, 213, 0.000000, 213.012960, 10.7590602199);
static const Atom Th214("Th", 90, 214, 0.000000, 214.011450, 10.8094928979);
static const Atom Th215("Th", 90, 215, 0.000000, 215.011730, 10.860015987);
static const Atom Th216("Th", 90, 216, 0.000000, 216.011051, 10.910490638);
static const Atom Th217("Th", 90, 217, 0.000000, 217.013070, 10.9611015622);
static const Atom Th218("Th", 90, 218, 0.000000, 218.013268, 11.0116205096);
static const Atom Th219("Th", 90, 219, 0.000000, 219.015520, 11.0622432023);
static const Atom Th220("Th", 90, 220, 0.000000, 220.015733, 11.1127629073);
static const Atom Th221("Th", 90, 221, 0.000000, 221.018171, 11.1633949947);
static const Atom Th222("Th", 90, 222, 0.000000, 222.018454, 11.2139182353);
static const Atom Th223("Th", 90, 223, 0.000000, 223.020795, 11.2645454234);
static const Atom Th224("Th", 90, 224, 0.000000, 224.021459, 11.3150879079);
static const Atom Th225("Th", 90, 225, 0.000000, 225.023941, 11.3657222177);
static const Atom Th226("Th", 90, 226, 0.000000, 226.024891, 11.4162791478);
static const Atom Th227("Th", 90, 227, 0.000000, 227.027699, 11.4669299235);
static const Atom Th228("Th", 90, 228, 0.000000, 228.028731, 11.5174910105);
static const Atom Th229("Th", 90, 229, 0.000000, 229.031755, 11.568152681);
static const Atom Th230("Th", 90, 230, 0.000000, 230.033127, 11.6187309056);
static const Atom Th231("Th", 90, 231, 0.000000, 231.036297, 11.6693999908);
static const Atom Th232("Th", 90, 232, 100.000000, 232.038050, 11.7199974948);
static const Atom Th233("Th", 90, 233, 0.000000, 233.041577, 11.7706845611);
static const Atom Th234("Th", 90, 234, 0.000000, 234.043595, 11.8212954398);
static const Atom Th235("Th", 90, 235, 0.000000, 235.047500, 11.8720016239);
static const Atom Th236("Th", 90, 236, 0.000000, 236.049710, 11.9226221952);
static const Atom Th237("Th", 90, 237, 0.000000, 237.053890, 11.9733422692);
static const Atom Th238("Th", 90, 238, 0.000000, 238.056240, 12.0239699118);
static const Atom Pa("Pa", 91, 0, 0.000000, 231.035880, 15.37);
static const Atom Pa213("Pa", 91, 213, 0.000000, 213.021180, 14.171545721);
static const Atom Pa214("Pa", 91, 214, 0.000000, 214.020740, 14.2380429126);
static const Atom Pa215("Pa", 91, 215, 0.000000, 215.019100, 14.3044602726);
static const Atom Pa216("Pa", 91, 216, 0.000000, 216.019110, 14.3709874012);
static const Atom Pa217("Pa", 91, 217, 0.000000, 217.018290, 14.4374593128);
static const Atom Pa218("Pa", 91, 218, 0.000000, 218.020010, 14.5041002017);
static const Atom Pa219("Pa", 91, 219, 0.000000, 219.019880, 14.5706180166);
static const Atom Pa220("Pa", 91, 220, 0.000000, 220.021880, 14.6372775328);
static const Atom Pa221("Pa", 91, 221, 0.000000, 221.021860, 14.7038026656);
static const Atom Pa222("Pa", 91, 222, 0.000000, 222.023730, 14.7704535335);
static const Atom Pa223("Pa", 91, 223, 0.000000, 223.023960, 14.8369952979);
static const Atom Pa224("Pa", 91, 224, 0.000000, 224.025610, 14.9036315299);
static const Atom Pa225("Pa", 91, 225, 0.000000, 225.026120, 14.9701919217);
static const Atom Pa226("Pa", 91, 226, 0.000000, 226.027933, 15.0368389975);
static const Atom Pa227("Pa", 91, 227, 0.000000, 227.028793, 15.1034226736);
static const Atom Pa228("Pa", 91, 228, 0.000000, 228.031037, 15.1700984223);
static const Atom Pa229("Pa", 91, 229, 0.000000, 229.032089, 15.2366948715);
static const Atom Pa230("Pa", 91, 230, 0.000000, 230.034533, 15.3033839255);
static const Atom Pa231("Pa", 91, 231, 100.000000, 231.035879, 15.3699999268);
static const Atom Pa232("Pa", 91, 232, 0.000000, 232.038582, 15.4367062178);
static const Atom Pa233("Pa", 91, 233, 0.000000, 233.040240, 15.5033429954);
static const Atom Pa234("Pa", 91, 234, 0.000000, 234.043302, 15.5700731494);
static const Atom Pa235("Pa", 91, 235, 0.000000, 235.045440, 15.6367418463);
static const Atom Pa236("Pa", 91, 236, 0.000000, 236.048680, 15.7034838554);
static const Atom Pa237("Pa", 91, 237, 0.000000, 237.051140, 15.7701739738);
static const Atom Pa238("Pa", 91, 238, 0.000000, 238.054500, 15.8369239661);
static const Atom Pa239("Pa", 91, 239, 0.000000, 239.057130, 15.903625394);
static const Atom Pa240("Pa", 91, 240, 0.000000, 240.060980, 15.9704079842);
static const Atom U("U", 92, 0, 0.000000, 238.028910, 18.95);
static const Atom U218("U", 92, 218, 0.000000, 218.023490, 17.357324938);
static const Atom U219("U", 92, 219, 0.000000, 219.024920, 17.4370509616);
static const Atom U220("U", 92, 220, 0.000000, 220.024710, 17.5166464212);
static const Atom U221("U", 92, 221, 0.000000, 221.026350, 17.5963891634);
static const Atom U222("U", 92, 222, 0.000000, 222.026070, 17.6759790502);
static const Atom U223("U", 92, 223, 0.000000, 223.027720, 17.7557225885);
static const Atom U224("U", 92, 224, 0.000000, 224.027590, 17.8353244171);
static const Atom U225("U", 92, 225, 0.000000, 225.029380, 17.9150791011);
static const Atom U226("U", 92, 226, 0.000000, 226.029340, 17.9946880948);
static const Atom U227("U", 92, 227, 0.000000, 227.031140, 18.0744435749);
static const Atom U228("U", 92, 228, 0.000000, 228.031366, 18.1540737455);
static const Atom U229("U", 92, 229, 0.000000, 229.033496, 18.2338554976);
static const Atom U230("U", 92, 230, 0.000000, 230.033927, 18.3135019887);
static const Atom U231("U", 92, 231, 0.000000, 231.036289, 18.3933022109);
static const Atom U232("U", 92, 232, 0.000000, 232.037146, 18.4729826406);
static const Atom U233("U", 92, 233, 0.000000, 233.039628, 18.5527923923);
static const Atom U234("U", 92, 234, 0.005500, 234.040946, 18.6325094675);
static const Atom U235("U", 92, 235, 0.720000, 235.043923, 18.712358691);
static const Atom U236("U", 92, 236, 0.000000, 236.045562, 18.7921013376);
static const Atom U237("U", 92, 237, 0.000000, 237.048724, 18.8719652575);
static const Atom U238("U", 92, 238, 99.274500, 238.050783, 18.9517413253);
static const Atom U239("U", 92, 239, 0.000000, 239.054288, 19.0316325601);
static const Atom U240("U", 92, 240, 0.000000, 240.056586, 19.111427703);
static const Atom U241("U", 92, 241, 0.000000, 241.060330, 19.1913379492);
static const Atom U242("U", 92, 242, 0.000000, 242.062930, 19.2711571191);
static const Atom Np("Np", 93, 0, 0.000000, 237.000000, 20.25);
static const Atom Np225("Np", 93, 225, 0.000000, 225.033900, 19.2275800633);
static const Atom Np226("Np", 93, 226, 0.000000, 226.035130, 19.3131281962);
static const Atom Np227("Np", 93, 227, 0.000000, 227.034960, 19.3985567089);
static const Atom Np228("Np", 93, 228, 0.000000, 228.036180, 19.4841039873);
static const Atom Np229("Np", 93, 229, 0.000000, 229.036250, 19.5695530063);
static const Atom Np230("Np", 93, 230, 0.000000, 230.037810, 19.6551293354);
static const Atom Np231("Np", 93, 231, 0.000000, 231.038230, 19.7406082595);
static const Atom Np232("Np", 93, 232, 0.000000, 232.040100, 19.8262110759);
static const Atom Np233("Np", 93, 233, 0.000000, 233.040730, 19.911707943);
static const Atom Np234("Np", 93, 234, 0.000000, 234.042889, 19.9973354525);
static const Atom Np235("Np", 93, 235, 0.000000, 235.044056, 20.082878194);
static const Atom Np236("Np", 93, 236, 0.000000, 236.046560, 20.1685351899);
static const Atom Np237("Np", 93, 237, 0.000000, 237.048167, 20.2541155604);
static const Atom Np238("Np", 93, 238, 0.000000, 238.050940, 20.3397955491);
static const Atom Np239("Np", 93, 239, 0.000000, 239.052931, 20.4254086956);
static const Atom Np240("Np", 93, 240, 0.000000, 240.056169, 20.5111283639);
static const Atom Np241("Np", 93, 241, 0.000000, 241.058250, 20.5967492089);
static const Atom Np242("Np", 93, 242, 0.000000, 242.061640, 20.6824818987);
static const Atom Np243("Np", 93, 243, 0.000000, 243.064270, 20.7681496519);
static const Atom Np244("Np", 93, 244, 0.000000, 244.067850, 20.8538985759);
static const Atom Pu("Pu", 94, 0, 0.000000, 244.000000, 19.84);
static const Atom Pu228("Pu", 94, 228, 0.000000, 228.038730, 18.5421655869);
static const Atom Pu229("Pu", 94, 229, 0.000000, 229.040140, 18.6235917115);
static const Atom Pu230("Pu", 94, 230, 0.000000, 230.039646, 18.704863019);
static const Atom Pu231("Pu", 94, 231, 0.000000, 231.041260, 18.7863057311);
static const Atom Pu232("Pu", 94, 232, 0.000000, 232.041179, 18.8676106203);
static const Atom Pu233("Pu", 94, 233, 0.000000, 233.042990, 18.9490693508);
static const Atom Pu234("Pu", 94, 234, 0.000000, 234.043305, 19.0304064393);
static const Atom Pu235("Pu", 94, 235, 0.000000, 235.045282, 19.1118786675);
static const Atom Pu236("Pu", 94, 236, 0.000000, 236.046048, 19.1932524357);
static const Atom Pu237("Pu", 94, 237, 0.000000, 237.048404, 19.2747554565);
static const Atom Pu238("Pu", 94, 238, 0.000000, 238.049553, 19.3561604076);
static const Atom Pu239("Pu", 94, 239, 0.000000, 239.052156, 19.4376835449);
static const Atom Pu240("Pu", 94, 240, 0.000000, 240.053808, 19.5191292656);
static const Atom Pu241("Pu", 94, 241, 0.000000, 241.056845, 19.600687749);
static const Atom Pu242("Pu", 94, 242, 0.000000, 242.058737, 19.682153025);
static const Atom Pu243("Pu", 94, 243, 0.000000, 243.061997, 19.7637295921);
static const Atom Pu244("Pu", 94, 244, 0.000000, 244.064198, 19.8452200341);
static const Atom Pu245("Pu", 94, 245, 0.000000, 245.067739, 19.9268194334);
static const Atom Pu246("Pu", 94, 246, 0.000000, 246.070198, 20.0083308538);
static const Atom Pu247("Pu", 94, 247, 0.000000, 247.074070, 20.0899571672);
static const Atom Am("Am", 95, 0, 0.000000, 243.000000, 13.67);
static const Atom Am231("Am", 95, 231, 0.000000, 231.045560, 12.997501256);
static const Atom Am232("Am", 95, 232, 0.000000, 232.046590, 13.0538143428);
static const Atom Am233("Am", 95, 233, 0.000000, 233.046470, 13.1100627362);
static const Atom Am234("Am", 95, 234, 0.000000, 234.047790, 13.166392137);
static const Atom Am235("Am", 95, 235, 0.000000, 235.048030, 13.2226607823);
static const Atom Am236("Am", 95, 236, 0.000000, 236.049570, 13.2790025593);
static const Atom Am237("Am", 95, 237, 0.000000, 237.049970, 13.3352802053);
static const Atom Am238("Am", 95, 238, 0.000000, 238.051980, 13.3916484222);
static const Atom Am239("Am", 95, 239, 0.000000, 239.053018, 13.4479619591);
static const Atom Am240("Am", 95, 240, 0.000000, 240.055288, 13.5043448023);
static const Atom Am241("Am", 95, 241, 0.000000, 241.056823, 13.5606862924);
static const Atom Am242("Am", 95, 242, 0.000000, 242.059543, 13.617094456);
static const Atom Am243("Am", 95, 243, 0.000000, 243.061373, 13.6734525301);
static const Atom Am244("Am", 95, 244, 0.000000, 244.064279, 13.7298711909);
static const Atom Am245("Am", 95, 245, 0.000000, 245.066445, 13.7862481611);
static const Atom Am246("Am", 95, 246, 0.000000, 246.069768, 13.842690241);
static const Atom Am247("Am", 95, 247, 0.000000, 247.072090, 13.8990760095);
static const Atom Am248("Am", 95, 248, 0.000000, 248.075750, 13.9555370473);
static const Atom Am249("Am", 95, 249, 0.000000, 249.078480, 14.0119457679);
static const Atom Cm("Cm", 96, 0, 0.000000, 247.000000, 13.51);
static const Atom Cm233("Cm", 96, 233, 0.000000, 233.050800, 12.747029587);
static const Atom Cm234("Cm", 96, 234, 0.000000, 234.050240, 12.8016953134);
static const Atom Cm235("Cm", 96, 235, 0.000000, 235.051590, 12.8564655097);
static const Atom Cm236("Cm", 96, 236, 0.000000, 236.051410, 12.9111520206);
static const Atom Cm237("Cm", 96, 237, 0.000000, 237.052890, 12.9659293275);
static const Atom Cm238("Cm", 96, 238, 0.000000, 238.053020, 13.0206327943);
static const Atom Cm239("Cm", 96, 239, 0.000000, 239.054950, 13.0754347146);
static const Atom Cm240("Cm", 96, 240, 0.000000, 240.055519, 13.1301621931);
static const Atom Cm241("Cm", 96, 241, 0.000000, 241.057647, 13.1849749268);
static const Atom Cm242("Cm", 96, 242, 0.000000, 242.058829, 13.239735967);
static const Atom Cm243("Cm", 96, 243, 0.000000, 243.061382, 13.2945719576);
static const Atom Cm244("Cm", 96, 244, 0.000000, 244.062746, 13.3493429252);
static const Atom Cm245("Cm", 96, 245, 0.000000, 245.065486, 13.4041891112);
static const Atom Cm246("Cm", 96, 246, 0.000000, 246.067218, 13.4589802015);
static const Atom Cm247("Cm", 96, 247, 0.000000, 247.070347, 13.5138477246);
static const Atom Cm248("Cm", 96, 248, 0.000000, 248.072342, 13.5686532001);
static const Atom Cm249("Cm", 96, 249, 0.000000, 249.075947, 13.6235467367);
static const Atom Cm250("Cm", 96, 250, 0.000000, 250.078351, 13.678374583);
static const Atom Cm251("Cm", 96, 251, 0.000000, 251.082278, 13.7332857319);
static const Atom Cm252("Cm", 96, 252, 0.000000, 252.084870, 13.7881238611);
static Atom ATOMS[] = {
H, H1, H2, H3, H4, H5, H6, He, He3, He4,
He5, He6, He7, He8, He9, He10, Li, Li4, Li5, Li6,
Li7, Li8, Li9, Li10, Li11, Li12, Be, Be5, Be6, Be7,
Be8, Be9, Be10, Be11, Be12, Be13, Be14, B, B7, B8,
B9, B10, B11, B12, B13, B14, B15, B16, B17, B18,
B19, C, C8, C9, C10, C11, C12, C13, C14, C15,
C16, C17, C18, C19, C20, C21, C22, N, N10, N11,
N12, N13, N14, N15, N16, N17, N18, N19, N20, N21,
N22, N23, N24, O, O12, O13, O14, O15, O16, O17,
O18, O19, O20, O21, O22, O23, O24, O25, O26, F,
F14, F15, F16, F17, F18, F19, F20, F21, F22, F23,
F24, F25, F26, F27, F28, F29, Ne, Ne32, Ne16, Ne17,
Ne18, Ne19, Ne20, Ne21, Ne22, Ne23, Ne24, Ne25, Ne26, Ne27,
Ne28, Ne29, Ne30, Ne31, Na, Na32, Na33, Na34, Na35, Na18,
Na19, Na20, Na21, Na22, Na23, Na24, Na25, Na26, Na27, Na28,
Na29, Na30, Na31, Mg, Mg32, Mg33, Mg34, Mg35, Mg36, Mg37,
Mg20, Mg21, Mg22, Mg23, Mg24, Mg25, Mg26, Mg27, Mg28, Mg29,
Mg30, Mg31, Al, Al32, Al33, Al34, Al35, Al36, Al37, Al38,
Al39, Al21, Al22, Al23, Al24, Al25, Al26, Al27, Al28, Al29,
Al30, Al31, Si, Si32, Si33, Si34, Si35, Si36, Si37, Si38,
Si39, Si40, Si41, Si42, Si22, Si23, Si24, Si25, Si26, Si27,
Si28, Si29, Si30, Si31, P, P24, P25, P26, P27, P28,
P29, P30, P31, P32, P33, P34, P35, P36, P37, P38,
P39, P40, P41, P42, P43, P44, P45, P46, S, S26,
S27, S28, S29, S30, S31, S32, S33, S34, S35, S36,
S37, S38, S39, S40, S41, S42, S43, S44, S45, S46,
S47, S48, S49, Cl, Cl28, Cl29, Cl30, Cl31, Cl32, Cl33,
Cl34, Cl35, Cl36, Cl37, Cl38, Cl39, Cl40, Cl41, Cl42, Cl43,
Cl44, Cl45, Cl46, Cl47, Cl48, Cl49, Cl50, Cl51, Ar, Ar30,
Ar31, Ar32, Ar33, Ar34, Ar35, Ar36, Ar37, Ar38, Ar39, Ar40,
Ar41, Ar42, Ar43, Ar44, Ar45, Ar46, Ar47, Ar48, Ar49, Ar50,
Ar51, Ar52, Ar53, K, K32, K33, K34, K35, K36, K37,
K38, K39, K40, K41, K42, K43, K44, K45, K46, K47,
K48, K49, K50, K51, K52, K53, K54, K55, Ca, Ca34,
Ca35, Ca36, Ca37, Ca38, Ca39, Ca40, Ca41, Ca42, Ca43, Ca44,
Ca45, Ca46, Ca47, Ca48, Ca49, Ca50, Ca51, Ca52, Ca53, Ca54,
Ca55, Ca56, Ca57, Sc, Sc36, Sc37, Sc38, Sc39, Sc40, Sc41,
Sc42, Sc43, Sc44, Sc45, Sc46, Sc47, Sc48, Sc49, Sc50, Sc51,
Sc52, Sc53, Sc54, Sc55, Sc56, Sc57, Sc58, Sc59, Ti, Ti38,
Ti39, Ti40, Ti41, Ti42, Ti43, Ti44, Ti45, Ti46, Ti47, Ti48,
Ti49, Ti50, Ti51, Ti52, Ti53, Ti54, Ti55, Ti56, Ti57, Ti58,
Ti59, Ti60, Ti61, V, V40, V41, V42, V43, V44, V45,
V46, V47, V48, V49, V50, V51, V52, V53, V54, V55,
V56, V57, V58, V59, V60, V61, V62, V63, Cr, Cr42,
Cr43, Cr44, Cr45, Cr46, Cr47, Cr48, Cr49, Cr50, Cr51, Cr52,
Cr53, Cr54, Cr55, Cr56, Cr57, Cr58, Cr59, Cr60, Cr61, Cr62,
Cr63, Cr64, Cr65, Mn, Mn44, Mn45, Mn46, Mn47, Mn48, Mn49,
Mn50, Mn51, Mn52, Mn53, Mn54, Mn55, Mn56, Mn57, Mn58, Mn59,
Mn60, Mn61, Mn62, Mn63, Mn64, Mn65, Mn66, Mn67, Fe, Fe45,
Fe46, Fe47, Fe48, Fe49, Fe50, Fe51, Fe52, Fe53, Fe54, Fe55,
Fe56, Fe57, Fe58, Fe59, Fe60, Fe61, Fe62, Fe63, Fe64, Fe65,
Fe66, Fe67, Fe68, Fe69, Co, Co48, Co49, Co50, Co51, Co52,
Co53, Co54, Co55, Co56, Co57, Co58, Co59, Co60, Co61, Co62,
Co63, Co64, Co65, Co66, Co67, Co68, Co69, Co70, Co71, Co72,
Ni, Ni50, Ni51, Ni52, Ni53, Ni54, Ni55, Ni56, Ni57, Ni58,
Ni59, Ni60, Ni61, Ni62, Ni63, Ni64, Ni65, Ni66, Ni67, Ni68,
Ni69, Ni70, Ni71, Ni72, Ni73, Ni74, Ni75, Ni76, Ni77, Ni78,
Cu, Cu52, Cu53, Cu54, Cu55, Cu56, Cu57, Cu58, Cu59, Cu60,
Cu61, Cu62, Cu63, Cu64, Cu65, Cu66, Cu67, Cu68, Cu69, Cu70,
Cu71, Cu72, Cu73, Cu74, Cu75, Cu76, Cu77, Cu78, Cu79, Cu80,
Zn, Zn54, Zn55, Zn56, Zn57, Zn58, Zn59, Zn60, Zn61, Zn62,
Zn63, Zn64, Zn65, Zn66, Zn67, Zn68, Zn69, Zn70, Zn71, Zn72,
Zn73, Zn74, Zn75, Zn76, Zn77, Zn78, Zn79, Zn80, Zn81, Zn82,
Ga, Ga56, Ga57, Ga58, Ga59, Ga60, Ga61, Ga62, Ga63, Ga64,
Ga65, Ga66, Ga67, Ga68, Ga69, Ga70, Ga71, Ga72, Ga73, Ga74,
Ga75, Ga76, Ga77, Ga78, Ga79, Ga80, Ga81, Ga82, Ga83, Ga84,
Ge, Ge58, Ge59, Ge60, Ge61, Ge62, Ge63, Ge64, Ge65, Ge66,
Ge67, Ge68, Ge69, Ge70, Ge71, Ge72, Ge73, Ge74, Ge75, Ge76,
Ge77, Ge78, Ge79, Ge80, Ge81, Ge82, Ge83, Ge84, Ge85, Ge86,
As, As60, As61, As62, As63, As64, As65, As66, As67, As68,
As69, As70, As71, As72, As73, As74, As75, As76, As77, As78,
As79, As80, As81, As82, As83, As84, As85, As86, As87, As88,
As89, Se, Se65, Se66, Se67, Se68, Se69, Se70, Se71, Se72,
Se73, Se74, Se75, Se76, Se77, Se78, Se79, Se80, Se81, Se82,
Se83, Se84, Se85, Se86, Se87, Se88, Se89, Se90, Se91, Se92,
Br, Br67, Br68, Br69, Br70, Br71, Br72, Br73, Br74, Br75,
Br76, Br77, Br78, Br79, Br80, Br81, Br82, Br83, Br84, Br85,
Br86, Br87, Br88, Br89, Br90, Br91, Br92, Br93, Br94, Kr,
Kr69, Kr70, Kr71, Kr72, Kr73, Kr74, Kr75, Kr76, Kr77, Kr78,
Kr79, Kr80, Kr81, Kr82, Kr83, Kr84, Kr85, Kr86, Kr87, Kr88,
Kr89, Kr90, Kr91, Kr92, Kr93, Kr94, Kr95, Kr96, Kr97, Rb,
Rb71, Rb72, Rb73, Rb74, Rb75, Rb76, Rb77, Rb78, Rb79, Rb80,
Rb81, Rb82, Rb83, Rb84, Rb85, Rb86, Rb87, Rb88, Rb89, Rb90,
Rb91, Rb92, Rb93, Rb94, Rb95, Rb96, Rb97, Rb98, Rb99, Rb100,
Rb101, Rb102, Sr, Sr73, Sr74, Sr75, Sr76, Sr77, Sr78, Sr79,
Sr80, Sr81, Sr82, Sr83, Sr84, Sr85, Sr86, Sr87, Sr88, Sr89,
Sr90, Sr91, Sr92, Sr93, Sr94, Sr95, Sr96, Sr97, Sr98, Sr99,
Sr100, Sr101, Sr102, Sr103, Sr104, Y, Y77, Y78, Y79, Y80,
Y81, Y82, Y83, Y84, Y85, Y86, Y87, Y88, Y89, Y90,
Y91, Y92, Y93, Y94, Y95, Y96, Y97, Y98, Y99, Y100,
Y101, Y102, Y103, Y104, Y105, Y106, Zr, Zr79, Zr80, Zr81,
Zr82, Zr83, Zr84, Zr85, Zr86, Zr87, Zr88, Zr89, Zr90, Zr91,
Zr92, Zr93, Zr94, Zr95, Zr96, Zr97, Zr98, Zr99, Zr100, Zr101,
Zr102, Zr103, Zr104, Zr105, Zr106, Zr107, Zr108, Nb, Nb81, Nb82,
Nb83, Nb84, Nb85, Nb86, Nb87, Nb88, Nb89, Nb90, Nb91, Nb92,
Nb93, Nb94, Nb95, Nb96, Nb97, Nb98, Nb99, Nb100, Nb101, Nb102,
Nb103, Nb104, Nb105, Nb106, Nb107, Nb108, Nb109, Nb110, Mo, Mo83,
Mo84, Mo85, Mo86, Mo87, Mo88, Mo89, Mo90, Mo91, Mo92, Mo93,
Mo94, Mo95, Mo96, Mo97, Mo98, Mo99, Mo100, Mo101, Mo102, Mo103,
Mo104, Mo105, Mo106, Mo107, Mo108, Mo109, Mo110, Mo111, Mo112, Mo113,
Tc, Tc85, Tc86, Tc87, Tc88, Tc89, Tc90, Tc91, Tc92, Tc93,
Tc94, Tc95, Tc96, Tc97, Tc98, Tc99, Tc100, Tc101, Tc102, Tc103,
Tc104, Tc105, Tc106, Tc107, Tc108, Tc109, Tc110, Tc111, Tc112, Tc113,
Tc114, Tc115, Ru, Ru87, Ru88, Ru89, Ru90, Ru91, Ru92, Ru93,
Ru94, Ru95, Ru96, Ru97, Ru98, Ru99, Ru100, Ru101, Ru102, Ru103,
Ru104, Ru105, Ru106, Ru107, Ru108, Ru109, Ru110, Ru111, Ru112, Ru113,
Ru114, Ru115, Ru116, Ru117, Ru118, Rh, Rh89, Rh90, Rh91, Rh92,
Rh93, Rh94, Rh95, Rh96, Rh97, Rh98, Rh99, Rh100, Rh101, Rh102,
Rh103, Rh104, Rh105, Rh106, Rh107, Rh108, Rh109, Rh110, Rh111, Rh112,
Rh113, Rh114, Rh115, Rh116, Rh117, Rh118, Rh119, Rh120, Rh121, Pd,
Pd91, Pd92, Pd93, Pd94, Pd95, Pd96, Pd97, Pd98, Pd99, Pd100,
Pd101, Pd102, Pd103, Pd104, Pd105, Pd106, Pd107, Pd108, Pd109, Pd110,
Pd111, Pd112, Pd113, Pd114, Pd115, Pd116, Pd117, Pd118, Pd119, Pd120,
Pd121, Pd122, Pd123, Ag, Ag94, Ag95, Ag96, Ag97, Ag98, Ag99,
Ag100, Ag101, Ag102, Ag103, Ag104, Ag105, Ag106, Ag107, Ag108, Ag109,
Ag110, Ag111, Ag112, Ag113, Ag114, Ag115, Ag116, Ag117, Ag118, Ag119,
Ag120, Ag121, Ag122, Ag123, Ag124, Ag125, Ag126, Ag127, Cd, Cd128,
Cd129, Cd130, Cd96, Cd97, Cd98, Cd99, Cd100, Cd101, Cd102, Cd103,
Cd104, Cd105, Cd106, Cd107, Cd108, Cd109, Cd110, Cd111, Cd112, Cd113,
Cd114, Cd115, Cd116, Cd117, Cd118, Cd119, Cd120, Cd121, Cd122, Cd123,
Cd124, Cd125, Cd126, Cd127, In, In128, In129, In130, In131, In132,
In133, In134, In98, In99, In100, In101, In102, In103, In104, In105,
In106, In107, In108, In109, In110, In111, In112, In113, In114, In115,
In116, In117, In118, In119, In120, In121, In122, In123, In124, In125,
In126, In127, Sn, Sn128, Sn129, Sn130, Sn131, Sn132, Sn133, Sn134,
Sn135, Sn136, Sn137, Sn100, Sn101, Sn102, Sn103, Sn104, Sn105, Sn106,
Sn107, Sn108, Sn109, Sn110, Sn111, Sn112, Sn113, Sn114, Sn115, Sn116,
Sn117, Sn118, Sn119, Sn120, Sn121, Sn122, Sn123, Sn124, Sn125, Sn126,
Sn127, Sb, Sb128, Sb129, Sb130, Sb131, Sb132, Sb133, Sb134, Sb135,
Sb136, Sb137, Sb138, Sb139, Sb103, Sb104, Sb105, Sb106, Sb107, Sb108,
Sb109, Sb110, Sb111, Sb112, Sb113, Sb114, Sb115, Sb116, Sb117, Sb118,
Sb119, Sb120, Sb121, Sb122, Sb123, Sb124, Sb125, Sb126, Sb127, Te,
Te128, Te129, Te130, Te131, Te132, Te133, Te134, Te135, Te136, Te137,
Te138, Te139, Te140, Te141, Te142, Te106, Te107, Te108, Te109, Te110,
Te111, Te112, Te113, Te114, Te115, Te116, Te117, Te118, Te119, Te120,
Te121, Te122, Te123, Te124, Te125, Te126, Te127, I, I128, I129,
I130, I131, I132, I133, I134, I135, I136, I137, I138, I139,
I140, I141, I142, I143, I144, I108, I109, I110, I111, I112,
I113, I114, I115, I116, I117, I118, I119, I120, I121, I122,
I123, I124, I125, I126, I127, Xe, Xe128, Xe129, Xe130, Xe131,
Xe132, Xe133, Xe134, Xe135, Xe136, Xe137, Xe138, Xe139, Xe140, Xe141,
Xe142, Xe143, Xe144, Xe145, Xe146, Xe147, Xe110, Xe111, Xe112, Xe113,
Xe114, Xe115, Xe116, Xe117, Xe118, Xe119, Xe120, Xe121, Xe122, Xe123,
Xe124, Xe125, Xe126, Xe127, Cs, Cs128, Cs129, Cs130, Cs131, Cs132,
Cs133, Cs134, Cs135, Cs136, Cs137, Cs138, Cs139, Cs140, Cs141, Cs142,
Cs143, Cs144, Cs145, Cs146, Cs147, Cs148, Cs149, Cs150, Cs151, Cs112,
Cs113, Cs114, Cs115, Cs116, Cs117, Cs118, Cs119, Cs120, Cs121, Cs122,
Cs123, Cs124, Cs125, Cs126, Cs127, Ba, Ba128, Ba129, Ba130, Ba131,
Ba132, Ba133, Ba134, Ba135, Ba136, Ba137, Ba138, Ba139, Ba140, Ba141,
Ba142, Ba143, Ba144, Ba145, Ba146, Ba147, Ba148, Ba149, Ba150, Ba151,
Ba152, Ba153, Ba114, Ba115, Ba116, Ba117, Ba118, Ba119, Ba120, Ba121,
Ba122, Ba123, Ba124, Ba125, Ba126, Ba127, La, La128, La129, La130,
La131, La132, La133, La134, La135, La136, La137, La138, La139, La140,
La141, La142, La143, La144, La145, La146, La147, La148, La149, La150,
La151, La152, La153, La154, La155, La117, La118, La119, La120, La121,
La122, La123, La124, La125, La126, La127, Ce, Ce128, Ce129, Ce130,
Ce131, Ce132, Ce133, Ce134, Ce135, Ce136, Ce137, Ce138, Ce139, Ce140,
Ce141, Ce142, Ce143, Ce144, Ce145, Ce146, Ce147, Ce148, Ce149, Ce150,
Ce151, Ce152, Ce153, Ce154, Ce155, Ce156, Ce157, Ce119, Ce120, Ce121,
Ce122, Ce123, Ce124, Ce125, Ce126, Ce127, Pr, Pr128, Pr129, Pr130,
Pr131, Pr132, Pr133, Pr134, Pr135, Pr136, Pr137, Pr138, Pr139, Pr140,
Pr141, Pr142, Pr143, Pr144, Pr145, Pr146, Pr147, Pr148, Pr149, Pr150,
Pr151, Pr152, Pr153, Pr154, Pr155, Pr156, Pr157, Pr158, Pr159, Pr121,
Pr122, Pr123, Pr124, Pr125, Pr126, Pr127, Nd, Nd128, Nd129, Nd130,
Nd131, Nd132, Nd133, Nd134, Nd135, Nd136, Nd137, Nd138, Nd139, Nd140,
Nd141, Nd142, Nd143, Nd144, Nd145, Nd146, Nd147, Nd148, Nd149, Nd150,
Nd151, Nd152, Nd153, Nd154, Nd155, Nd156, Nd157, Nd158, Nd159, Nd160,
Nd161, Nd126, Nd127, Pm, Pm128, Pm129, Pm130, Pm131, Pm132, Pm133,
Pm134, Pm135, Pm136, Pm137, Pm138, Pm139, Pm140, Pm141, Pm142, Pm143,
Pm144, Pm145, Pm146, Pm147, Pm148, Pm149, Pm150, Pm151, Pm152, Pm153,
Pm154, Pm155, Pm156, Pm157, Pm158, Pm159, Pm160, Pm161, Pm162, Pm163,
Sm, Sm130, Sm131, Sm132, Sm133, Sm134, Sm135, Sm136, Sm137, Sm138,
Sm139, Sm140, Sm141, Sm142, Sm143, Sm144, Sm145, Sm146, Sm147, Sm148,
Sm149, Sm150, Sm151, Sm152, Sm153, Sm154, Sm155, Sm156, Sm157, Sm158,
Sm159, Sm160, Sm161, Sm162, Sm163, Sm164, Sm165, Eu, Eu132, Eu133,
Eu134, Eu135, Eu136, Eu137, Eu138, Eu139, Eu140, Eu141, Eu142, Eu143,
Eu144, Eu145, Eu146, Eu147, Eu148, Eu149, Eu150, Eu151, Eu152, Eu153,
Eu154, Eu155, Eu156, Eu157, Eu158, Eu159, Eu160, Eu161, Eu162, Eu163,
Eu164, Eu165, Eu166, Eu167, Gd, Gd136, Gd137, Gd138, Gd139, Gd140,
Gd141, Gd142, Gd143, Gd144, Gd145, Gd146, Gd147, Gd148, Gd149, Gd150,
Gd151, Gd152, Gd153, Gd154, Gd155, Gd156, Gd157, Gd158, Gd159, Gd160,
Gd161, Gd162, Gd163, Gd164, Gd165, Gd166, Gd167, Gd168, Gd169, Tb,
Tb138, Tb139, Tb140, Tb141, Tb142, Tb143, Tb144, Tb145, Tb146, Tb147,
Tb148, Tb149, Tb150, Tb151, Tb152, Tb153, Tb154, Tb155, Tb156, Tb157,
Tb158, Tb159, Tb160, Tb161, Tb162, Tb163, Tb164, Tb165, Tb166, Tb167,
Tb168, Tb169, Tb170, Tb171, Dy, Dy140, Dy141, Dy142, Dy143, Dy144,
Dy145, Dy146, Dy147, Dy148, Dy149, Dy150, Dy151, Dy152, Dy153, Dy154,
Dy155, Dy156, Dy157, Dy158, Dy159, Dy160, Dy161, Dy162, Dy163, Dy164,
Dy165, Dy166, Dy167, Dy168, Dy169, Dy170, Dy171, Dy172, Dy173, Ho,
Ho142, Ho143, Ho144, Ho145, Ho146, Ho147, Ho148, Ho149, Ho150, Ho151,
Ho152, Ho153, Ho154, Ho155, Ho156, Ho157, Ho158, Ho159, Ho160, Ho161,
Ho162, Ho163, Ho164, Ho165, Ho166, Ho167, Ho168, Ho169, Ho170, Ho171,
Ho172, Ho173, Ho174, Ho175, Er, Er144, Er145, Er146, Er147, Er148,
Er149, Er150, Er151, Er152, Er153, Er154, Er155, Er156, Er157, Er158,
Er159, Er160, Er161, Er162, Er163, Er164, Er165, Er166, Er167, Er168,
Er169, Er170, Er171, Er172, Er173, Er174, Er175, Er176, Er177, Tm,
Tm146, Tm147, Tm148, Tm149, Tm150, Tm151, Tm152, Tm153, Tm154, Tm155,
Tm156, Tm157, Tm158, Tm159, Tm160, Tm161, Tm162, Tm163, Tm164, Tm165,
Tm166, Tm167, Tm168, Tm169, Tm170, Tm171, Tm172, Tm173, Tm174, Tm175,
Tm176, Tm177, Tm178, Tm179, Yb, Yb148, Yb149, Yb150, Yb151, Yb152,
Yb153, Yb154, Yb155, Yb156, Yb157, Yb158, Yb159, Yb160, Yb161, Yb162,
Yb163, Yb164, Yb165, Yb166, Yb167, Yb168, Yb169, Yb170, Yb171, Yb172,
Yb173, Yb174, Yb175, Yb176, Yb177, Yb178, Yb179, Yb180, Yb181, Lu,
Lu150, Lu151, Lu152, Lu153, Lu154, Lu155, Lu156, Lu157, Lu158, Lu159,
Lu160, Lu161, Lu162, Lu163, Lu164, Lu165, Lu166, Lu167, Lu168, Lu169,
Lu170, Lu171, Lu172, Lu173, Lu174, Lu175, Lu176, Lu177, Lu178, Lu179,
Lu180, Lu181, Lu182, Lu183, Lu184, Hf, Hf154, Hf155, Hf156, Hf157,
Hf158, Hf159, Hf160, Hf161, Hf162, Hf163, Hf164, Hf165, Hf166, Hf167,
Hf168, Hf169, Hf170, Hf171, Hf172, Hf173, Hf174, Hf175, Hf176, Hf177,
Hf178, Hf179, Hf180, Hf181, Hf182, Hf183, Hf184, Hf185, Hf186, Ta,
Ta156, Ta157, Ta158, Ta159, Ta160, Ta161, Ta162, Ta163, Ta164, Ta165,
Ta166, Ta167, Ta168, Ta169, Ta170, Ta171, Ta172, Ta173, Ta174, Ta175,
Ta176, Ta177, Ta178, Ta179, Ta180, Ta181, Ta182, Ta183, Ta184, Ta185,
Ta186, Ta187, Ta188, W, W158, W159, W160, W161, W162, W163,
W164, W165, W166, W167, W168, W169, W170, W171, W172, W173,
W174, W175, W176, W177, W178, W179, W180, W181, W182, W183,
W184, W185, W186, W187, W188, W189, W190, Re, Re160, Re161,
Re162, Re163, Re164, Re165, Re166, Re167, Re168, Re169, Re170, Re171,
Re172, Re173, Re174, Re175, Re176, Re177, Re178, Re179, Re180, Re181,
Re182, Re183, Re184, Re185, Re186, Re187, Re188, Re189, Re190, Re191,
Re192, Os, Os162, Os163, Os164, Os165, Os166, Os167, Os168, Os169,
Os170, Os171, Os172, Os173, Os174, Os175, Os176, Os177, Os178, Os179,
Os180, Os181, Os182, Os183, Os184, Os185, Os186, Os187, Os188, Os189,
Os190, Os191, Os192, Os193, Os194, Os195, Os196, Ir, Ir165, Ir166,
Ir167, Ir168, Ir169, Ir170, Ir171, Ir172, Ir173, Ir174, Ir175, Ir176,
Ir177, Ir178, Ir179, Ir180, Ir181, Ir182, Ir183, Ir184, Ir185, Ir186,
Ir187, Ir188, Ir189, Ir190, Ir191, Ir192, Ir193, Ir194, Ir195, Ir196,
Ir197, Ir198, Ir199, Pt, Pt168, Pt169, Pt170, Pt171, Pt172, Pt173,
Pt174, Pt175, Pt176, Pt177, Pt178, Pt179, Pt180, Pt181, Pt182, Pt183,
Pt184, Pt185, Pt186, Pt187, Pt188, Pt189, Pt190, Pt191, Pt192, Pt193,
Pt194, Pt195, Pt196, Pt197, Pt198, Pt199, Pt200, Pt201, Pt202, Au,
Au171, Au172, Au173, Au174, Au175, Au176, Au177, Au178, Au179, Au180,
Au181, Au182, Au183, Au184, Au185, Au186, Au187, Au188, Au189, Au190,
Au191, Au192, Au193, Au194, Au195, Au196, Au197, Au198, Au199, Au200,
Au201, Au202, Au203, Au204, Au205, Hg, Hg175, Hg176, Hg177, Hg178,
Hg179, Hg180, Hg181, Hg182, Hg183, Hg184, Hg185, Hg186, Hg187, Hg188,
Hg189, Hg190, Hg191, Hg192, Hg193, Hg194, Hg195, Hg196, Hg197, Hg198,
Hg199, Hg200, Hg201, Hg202, Hg203, Hg204, Hg205, Hg206, Hg207, Hg208,
Tl, Tl177, Tl178, Tl179, Tl180, Tl181, Tl182, Tl183, Tl184, Tl185,
Tl186, Tl187, Tl188, Tl189, Tl190, Tl191, Tl192, Tl193, Tl194, Tl195,
Tl196, Tl197, Tl198, Tl199, Tl200, Tl201, Tl202, Tl203, Tl204, Tl205,
Tl206, Tl207, Tl208, Tl209, Tl210, Pb, Pb181, Pb182, Pb183, Pb184,
Pb185, Pb186, Pb187, Pb188, Pb189, Pb190, Pb191, Pb192, Pb193, Pb194,
Pb195, Pb196, Pb197, Pb198, Pb199, Pb200, Pb201, Pb202, Pb203, Pb204,
Pb205, Pb206, Pb207, Pb208, Pb209, Pb210, Pb211, Pb212, Pb213, Pb214,
Bi, Bi185, Bi186, Bi187, Bi188, Bi189, Bi190, Bi191, Bi192, Bi193,
Bi194, Bi195, Bi196, Bi197, Bi198, Bi199, Bi200, Bi201, Bi202, Bi203,
Bi204, Bi205, Bi206, Bi207, Bi208, Bi209, Bi210, Bi211, Bi212, Bi213,
Bi214, Bi215, Bi216, Po, Po190, Po191, Po192, Po193, Po194, Po195,
Po196, Po197, Po198, Po199, Po200, Po201, Po202, Po203, Po204, Po205,
Po206, Po207, Po208, Po209, Po210, Po211, Po212, Po213, Po214, Po215,
Po216, Po217, Po218, At, At193, At194, At195, At196, At197, At198,
At199, At200, At201, At202, At203, At204, At205, At206, At207, At208,
At209, At210, At211, At212, At213, At214, At215, At216, At217, At218,
At219, At220, At221, At222, At223, Rn, Rn196, Rn197, Rn198, Rn199,
Rn200, Rn201, Rn202, Rn203, Rn204, Rn205, Rn206, Rn207, Rn208, Rn209,
Rn210, Rn211, Rn212, Rn213, Rn214, Rn215, Rn216, Rn217, Rn218, Rn219,
Rn220, Rn221, Rn222, Rn223, Rn224, Rn225, Rn226, Rn227, Rn228, Fr,
Fr200, Fr201, Fr202, Fr203, Fr204, Fr205, Fr206, Fr207, Fr208, Fr209,
Fr210, Fr211, Fr212, Fr213, Fr214, Fr215, Fr216, Fr217, Fr218, Fr219,
Fr220, Fr221, Fr222, Fr223, Fr224, Fr225, Fr226, Fr227, Fr228, Fr229,
Fr230, Fr231, Fr232, Ra, Ra203, Ra204, Ra205, Ra206, Ra207, Ra208,
Ra209, Ra210, Ra211, Ra212, Ra213, Ra214, Ra215, Ra216, Ra217, Ra218,
Ra219, Ra220, Ra221, Ra222, Ra223, Ra224, Ra225, Ra226, Ra227, Ra228,
Ra229, Ra230, Ra231, Ra232, Ra233, Ra234, Ac, Ac207, Ac208, Ac209,
Ac210, Ac211, Ac212, Ac213, Ac214, Ac215, Ac216, Ac217, Ac218, Ac219,
Ac220, Ac221, Ac222, Ac223, Ac224, Ac225, Ac226, Ac227, Ac228, Ac229,
Ac230, Ac231, Ac232, Ac233, Ac234, Ac235, Ac236, Th, Th210, Th211,
Th212, Th213, Th214, Th215, Th216, Th217, Th218, Th219, Th220, Th221,
Th222, Th223, Th224, Th225, Th226, Th227, Th228, Th229, Th230, Th231,
Th232, Th233, Th234, Th235, Th236, Th237, Th238, Pa, Pa213, Pa214,
Pa215, Pa216, Pa217, Pa218, Pa219, Pa220, Pa221, Pa222, Pa223, Pa224,
Pa225, Pa226, Pa227, Pa228, Pa229, Pa230, Pa231, Pa232, Pa233, Pa234,
Pa235, Pa236, Pa237, Pa238, Pa239, Pa240, U, U218, U219, U220,
U221, U222, U223, U224, U225, U226, U227, U228, U229, U230,
U231, U232, U233, U234, U235, U236, U237, U238, U239, U240,
U241, U242, Np, Np225, Np226, Np227, Np228, Np229, Np230, Np231,
Np232, Np233, Np234, Np235, Np236, Np237, Np238, Np239, Np240, Np241,
Np242, Np243, Np244, Pu, Pu228, Pu229, Pu230, Pu231, Pu232, Pu233,
Pu234, Pu235, Pu236, Pu237, Pu238, Pu239, Pu240, Pu241, Pu242, Pu243,
Pu244, Pu245, Pu246, Pu247, Am, Am231, Am232, Am233, Am234, Am235,
Am236, Am237, Am238, Am239, Am240, Am241, Am242, Am243, Am244, Am245,
Am246, Am247, Am248, Am249, Cm, Cm233, Cm234, Cm235, Cm236, Cm237,
Cm238, Cm239, Cm240, Cm241, Cm242, Cm243, Cm244, Cm245, Cm246, Cm247,
Cm248, Cm249, Cm250, Cm251, Cm252
};

/** The total number of atoms in the array. */
static const size_t NUM_ATOMS = 2845;

// ---------- END DO NOT EDIT AREA----------
#endif // DOXYGEN_SHOULD_SKIP_THIS

inline bool AtomIsNaN(const double number)
{
  return (number != number);
}

bool AtomEqualsWithNaN(const double left, const double right)
{
  if (left == right)
    return true;
  if (AtomIsNaN(left) && AtomIsNaN(right))
    return true;
  return false;
}

bool operator==(const Atom& left, const Atom & right)
{
  if (&left == &right)
    return true;

  if (left.z_number != right.z_number)
    return false;
  if (left.a_number != right.a_number)
    return false;
  if (!AtomEqualsWithNaN(left.abundance, right.abundance))
    return false;
  if (!AtomEqualsWithNaN(left.mass, right.mass))
    return false;
  if (!AtomEqualsWithNaN(left.mass_density, right.mass_density))
    return false;
  if (!AtomEqualsWithNaN(left.number_density, right.number_density))
    return false;
  if (left.neutron != right.neutron)
    return false;

  // passes all of the tests
  return true;
}
bool operator!=(const Atom& left, const Atom & right)
{
  return !(left == right);
}

std::ostream& operator<<(std::ostream& out, const Atom &atom)
{
  out << atom.symbol << atom.a_number;
  return out;
}

bool compareAtoms(const Atom &left, const Atom &right)
{
  if (left.z_number == right.z_number) {
    return (left.a_number < right.a_number);
  }
  else {
    return (left.z_number < right.z_number);
  }
}

/**
 * @param z_number Atomic number of the atom to get
 * @param a_number Mass number of the atom to get
 * @return The atom corresponding to the given Z and A
 */
Atom getAtom(const uint16_t z_number, const uint16_t a_number)
{
  Atom temp("junk", z_number, a_number, NAN, NAN, NAN);

  Atom *result = std::lower_bound(&(ATOMS[0]), &(ATOMS[NUM_ATOMS]), temp, compareAtoms);
  if (result == &(ATOMS[NUM_ATOMS]) || result->z_number != z_number
      || result->a_number != a_number)
  {
    std::stringstream msg;
    msg << "Failed to find an atom with z=" << z_number << " and a=" << a_number;
    throw std::runtime_error(msg.str());
  }

  return *result;
}

Atom getAtom(const string& symbol, const uint16_t a_number)
{
  // special cases for aliases
  if (symbol.compare("D") == 0)
    return H2;
  if (symbol.compare("T") == 0)
    return H3;

  // linear search
  for (size_t i = 0; i < NUM_ATOMS; ++i) {
    if (symbol.compare(ATOMS[i].symbol) == 0) {
      if (a_number == ATOMS[i].a_number) {
        return ATOMS[i];
      }
    }
  }

  // fail loudly
  std::stringstream msg;
  msg << "Failed to find an atom with symbol=" << symbol << " and a=" << a_number;
  throw std::runtime_error(msg.str());
}

} // namespace PhysicalConstants
} // namespace Mantid

