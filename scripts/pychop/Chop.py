# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name, too-many-locals, too-many-arguments, unused-variable, unused-argument

"""
Chop.py: a Python port of the functions of CHOP, a program for calculating ToF widths of neutron chopper
spectrometers, by T G Perring. Python implementation by R A Ewings, after a Matlab version by J W Taylor

The theory is based on the formulism developed by Carlile, Taylor, Perring et al., in a series of
technical reports:

[1] RAL-85-052: MARS - a Multi-Angle Rotor Spectrometer for the SNS
    C J Carlile, A D Taylor and W G Williams
    http://purl.org/net/epubs/manifestation/12308488
[2] RALT-028-94: High energy magnetic excitations in hexagonal cobalt
    T G Perring, Ph.D. Thesis, University of Cambridge (1991)
[3] RAL-94-025: The resolution function of the chopper spectrometer HET at ISIS
    T G Perring, Proceedings of ICANS XII (1993)
    http://www.neutronresearch.com/parch/1993/01/199301013280.pdf
"""

import numpy as np
import collections
import warnings

warnings.simplefilter("always", UserWarning)

# ------------------------------------------------------------------------------------------------- #
# Chopper functions
# ------------------------------------------------------------------------------------------------- #


def tchop(freq, Ei, pslit, radius, rho):
    """
    ! Calculates the time width of a Fermi chopper given its parameters, the Ei and frequency.
    """
    p, R, w = tuple([pslit, radius, freq * 2 * np.pi])
    if p == 0.00 and R == 0.00 and rho == 0.00:
        raise ValueError("PyChop: tchop(): slit width, chopper radius or curvature is zero!")
    # Calculate parameter gam:
    veloc = 437.3920 * np.sqrt(Ei)
    gamm = (2.00 * (R**2) / p) * abs(1.00 / rho - 2.00 * w / veloc)
    # Find regime and calculate variance:
    if hasattr(gamm, "__len__"):
        tausqr = np.zeros(len(gamm))
        pre = (p / (2.00 * R * w)) ** 2 / 6.00
        idx = np.where((gamm <= 1.0))
        tausqr[idx] = pre * (1.00 - (gamm[idx] ** 2) ** 2 / 10.00) / (1.00 - (gamm[idx] ** 2) / 6.00)
        idx = np.where((gamm > 1.0) * (gamm < 4.0))
        groot = np.sqrt(gamm[idx])
        # area[idx] = pre * 0.60 * gamm[idx] * ((groot-2.00)**2) * (groot+8.00) / (groot+4.00)
    else:
        if gamm >= 4.00:
            warnings.warn("PyChop: tchop(): No transmission at %5.3f meV at %3d Hz" % (Ei, freq))
            return np.nan
        if gamm <= 1.00:
            gsqr = (1.00 - (gamm**2) ** 2 / 10.00) / (1.00 - (gamm**2) / 6.00)
        else:
            groot = np.sqrt(gamm)
            gsqr = 0.60 * gamm * ((groot - 2.00) ** 2) * (groot + 8.00) / (groot + 4.00)
        tausqr = ((p / (2.00 * R * w)) ** 2 / 6.00) * gsqr
    return tausqr


def achop(Ei, freq, dslat, pslit, radius, rho):
    """
    !   Calculates the integral of the chopper transmission function
    !   P(h, t) over time and distance for any energy. The answer is in m.S
    !   New version 15/1/90
    !
    !    p       slit thickness (m)                              R*8
    !    R       slit package diameter (m)                       R*8
    !    rho     slit radius of curvature (m)                    R*8
    !    w       angular frequency of rotor (rad/sec)            R*8
    !    ei      energy the rotor has been phased for (meV)      R*8
    !    area    intergral                                       R*8
    !    ierr    error indicator                                 integer
    !               =0  no problems
    !               =1  if no transmission  AREA set to zero
    """
    _, p1, R1, rho1, w1 = tuple([dslat, pslit, radius, rho, freq * 2 * np.pi])
    vela = 437.3920 * np.sqrt(Ei)
    gamm = (2.00 * (R1**2) / p1) * abs(1.00 / rho1 - 2.00 * w1 / vela)
    # Find regime and calculate variance:
    if hasattr(gamm, "__len__"):
        area = np.zeros(len(gamm))
        pre = (p1**2) / (2.00 * R1 * w1)
        idx = np.where(gamm <= 1.0)
        area[idx] = pre * (1.0 - (gamm[idx] ** 2) / 6.0)
        idx = np.where((gamm > 1.0) * (gamm < 4.0))
        groot = np.sqrt(gamm[idx])
        area[idx] = pre * groot * ((groot - 2.0) ** 2) * (groot + 4.0) / 6.0
    else:
        if gamm >= 4.00:
            warnings.warn("PyChop: achop(): No transmission at %5.3f meV at %3d Hz" % (Ei, freq), UserWarning)
            return np.nan
        else:
            if gamm <= 1.00:
                f1 = 1.00 - (gamm**2) / 6.00
            else:
                groot = np.sqrt(gamm)
                f1 = groot * ((groot - 2.00) ** 2) * (groot + 4.00) / 6.00
        area = ((p1**2) / (2.00 * R1 * w1)) * f1
    return area


# ------------------------------------------------------------------------------------------------- #
# Moderator functions
# ------------------------------------------------------------------------------------------------- #


def tikeda(S1, S2, B1, B2, Emod, Ei):
    """
    ! Calculates the moderator time width based on the Ikeda-Carpenter distribution
    """
    Ei = np.array(Ei if hasattr(Ei, "__len__") else [Ei])
    sig = np.sqrt((S1 * S1) + ((S2 * S2 * 81.8048) / Ei))
    A = 4.37392e-4 * sig * np.sqrt(Ei)
    tausqr = []
    B = np.array([B1] * len(Ei))
    B[np.where(Ei > 130.0)] = B2
    R = np.exp(-Ei / Emod)
    tausqr = (3.0 / (A**2)) + (R * (2.0 - R)) / (B**2)
    # variance currently in mms**2. Convert to sec**2
    return (tausqr if len(tausqr) > 1 else tausqr[0]) * 1.0e-12


def tchi(delta, Ei):
    """
    ! Calculates the moderator time width based on the Chi^2 distribution
    """
    vel = 437.392 * np.sqrt(Ei)
    tausqr = ((delta / 1.96) / vel) ** 2
    return tausqr


def tchi_2(delta_0, delta_G, Ei):
    """
    ! Calculates the moderator time width based on a modified Chi^2 distribution
    """
    vel = 437.392 * np.sqrt(Ei)
    tausqr = (((delta_0 + delta_G * np.sqrt(Ei)) / 1.96) / vel) ** 2
    return tausqr


def flux_norm(ch_mod):
    """
    ! Returns an empirically determined flux normalisation factor for different ISIS moderators
    """
    phi0 = {"A": 1.0, "AP": 2.8, "H2": 1.8, "CH4": 2.6}
    if ch_mod not in phi0.keys():
        raise ValueError("Moderator %s is not supported in PyChop" % (ch_mod))
    return phi0[ch_mod]


def flux_fun(en_ev, ch_mod):
    """
    ! Calculates the energy dependence of the flux (see Perring et al RAL-85-029)
    !
    !  Entry
    !     en_ev  : eV
    !     ch_mod : 'A'  'AP'  'CH4'  'H2' 'TEST'
    !
    !  Exit
    !     phifun : the functional dependace with energy
    """
    if ch_mod == "A":
        raise ValueError('The "A" Moderator is not supported in PyChop')
    elif ch_mod == "AP":
        ijoin, rj, t, a, w1, w2, w3, w4, w5 = tuple([0, 2.25, 0.032, 0.95, 120.0, 10.0, 0.0, 0.0, 0.0])
    elif ch_mod == "H2":
        ijoin, rj, t, a, w1, w2, w3, w4, w5 = tuple([1, 2.35, 0.0021, 0.95, 15.5, 3.1, 11.0, 0.254, 0.0275])
    elif ch_mod == "CH4":
        ijoin, rj, t, a, w1, w2, w3, w4, w5 = tuple([0, 2.1, 0.011, 0.92, 55.0, 7.0, 0.0, 0.0, 0.0])
    else:
        raise ValueError("Moderator %s is not supported in PyChop" % (ch_mod))
    # Calculation:
    phi_max = rj * (en_ev / (t**2)) * np.exp(-en_ev / t)
    phi_epi = 1.00 / (en_ev) ** a
    expon = np.exp(-w1 / np.sqrt(1000.00 * en_ev) + w2)
    delt1 = expon / (1.00 + expon)
    if ijoin == 1:
        expon = np.exp((w4 - 1.00 / np.sqrt(1000.00 * en_ev)) / w5)
        delt2 = 1.00 + w3 / (1.00 + expon)
    else:
        delt2 = 1.00
    phifun = phi_max + delt1 * delt2 * phi_epi
    return phifun


def flux_calc(Ei, ch_mod, thetam):
    """
    ! Calculates the flux at a given Ei based on the moderator model of Perring et al RAL-85-029
    """
    conv1 = 3.615
    conv2 = 9.104157e-12
    conv = conv1 * conv2
    en_ev = Ei / 1000.00
    phi0 = flux_norm(ch_mod)
    phifun = flux_fun(en_ev, ch_mod)
    flux = conv * (phi0 * np.cos(thetam)) * (np.sqrt(en_ev) * phifun)
    return flux


# ------------------------------------------------------------------------------------------------- #
# Detector functions
# ------------------------------------------------------------------------------------------------- #
def detect2(wd, hd, wf, idet, dd):
    """
    ! Calculates the detector time widths for either a single He tube or several tubes grouped together
    """
    if idet == 1:
        # Davidson type scintillator detector
        raise ValueError("Li detector not supported in Pychop")
    elif idet == 2:
        # He cylindrical detectors binned together
        rad = dd / 2.0
        atms = 10.0
        t2rad = 0.063
        effic, delta, ddsqr, v_dd, v_d = detect_he(wf, rad, atms, t2rad)
        sigd = wd / np.sqrt(12.0)
        sigdz = hd / np.sqrt(12.0)
        sigdd = np.sqrt(v_dd)
    else:
        # He cylindrical detector
        rad = dd / 2.0
        atms = 10.0
        t2rad = 0.063
        effic, delta, ddsqr, v_dd, v_d = detect_he(wf, rad, atms, t2rad)
        ndet = max(int(wd / dd), 1.0)
        space = 2.0 * rad
        v_d = v_d + (space**2) * (ndet**2 - 1) / 12.0
        sigd = np.sqrt(v_d)
        sigdz = hd / np.sqrt(12.0)
        sigdd = np.sqrt(v_dd)
    return delta, sigd, sigdz, sigdd, effic


def detect_he(wvec, rad, atms, t2rad):
    """
    ! Arguments
    ! ----------
    ! Entry:
    !   radius (m), wave-vector(A-1), no.atmospheres He (atms),
    !  ratio wall thickness to radius
    !
    ! Exit:
    !  (The above are unchanged)
    !  effic     efficiency (0< eff <1)
    !  delta     shift in effective position of detector from centre (m)
    !  ddsqr     mean square depth of absorption (w.r.t. centre) (m**2)
    !  v_dd      variance of depth (m**2)
    !  v_d       variance of width (m**2)
    !  ierr   =  0  no problems
    !         =  1  rad< 0, t2rad <0 or >1, atms <0, wvec too small
    !         =  2  error in called routines
    !
    ! Origin of data
    ! --------------
    !  CKL data :
    !   "At 2200 m/s xsect=5327 barns    En=25.415 meV         "
    !   "At 10 atms, rho_atomic=2.688e-4, so sigma=1.4323 cm-1"
    !
    !  These data are not quite consistent, but the errors are small :
    !    2200 m/s = 25.299 meV
    !    5327 barns & 1.4323 cm-1 ==> 10atms ofideal gas at 272.9K
    !   but at what temperature are the tubes "10 atms" ?
    !
    !  Shall use  1.4323 cm-1 @ 3.49416 A-1 with sigma prop. 1/v
    !
    !  This corresponds to a reference energy of 25.299meV, NOT 25.415.
    ! This accounts for a difference of typically 1 pt in 1000 for
    ! energies around a few hundred meV. The approximate quadrature
    ! of CKL is accurate typically to 1 pt in 10000 for efficiency.
    ! The Chebyshev approximation used in this routine is accurate to
    ! 1 pt in 10**12 for all the returned moments. The routine is
    ! typically 5 times faster than CKL.
    """
    sigref = 143.23
    wref = 3.49416
    atmref = 10.0
    const = sigref * wref / atmref
    if rad < 0.0 or t2rad < 0.0 or t2rad > 1.0 or atms < 0.0:
        raise ValueError("Error with detect_he input parameters")
    else:
        reff = rad * (1.0 - t2rad)
        var = 2.0 * (rad * (1.0 - t2rad)) * (const * atms)
        if wvec < (var * 1.0e-18):
            raise ValueError("Error with size of wavevector for input pars")
        else:
            alf = var / wvec
            effic, delta, ddsqr, v_dd, v_d = tube_mts(alf)
            delta = delta * reff
            ddsqr = ddsqr * (reff**2)
            v_dd = v_dd * (reff**2)
            v_d = v_d * (reff**2)
    return effic, delta, ddsqr, v_dd, v_d


def tube_mts(alf):
    """
    !  T.G.Perring 6/4/90
    !
    !  Given ALF (radius in m.f.p.), calculates:
    !    EFF   efficiency
    !    DEL   mean depth of absorption (w.r.t centre)
    !    XSQR  mean of (depth**2) (w.r.t. centre)
    !    VX    variance of depth of absorption
    !    VY    variance of width
    !
    !  The routine approximates the functions by Chebyshev polynomial
    ! expansions over the ranges  0 =< alf =<9  and
    ! 10 =< alf =< (infinity). For  9 =< alf =< 10 a linear combination
    ! of the two approximations is taken.
    !
    !  The routine gives relative and absolute accuracy of the quantities
    ! to better than 10**-12 for all positive ALF.
    !
    !    IERR  returned as  0  ALF .ge. 0.0
    !                       1  ALF .lt. 0.0
    """
    g0 = (32.0 - 3.0 * (np.pi**2)) / 48.0
    g1 = 14.0 / 3.0 - (np.pi**2) / 8.0
    c_eff_f = [
        0.7648360390553052,
        -0.3700950778935237,
        0.1582704090813516,
        -6.0170218669705407e-02,
        2.0465515957968953e-02,
        -6.2690181465706840e-03,
        1.7408667184745830e-03,
        -4.4101378999425122e-04,
        1.0252117967127217e-04,
        -2.1988904738111659e-05,
        4.3729347905629990e-06,
        -8.0998753944849788e-07,
        1.4031240949230472e-07,
        -2.2815971698619819e-08,
        3.4943984983382137e-09,
        -5.0562696807254781e-10,
        6.9315483353094009e-11,
        -9.0261598195695569e-12,
        1.1192324844699897e-12,
        -1.3204992654891612e-13,
        1.4100387524251801e-14,
        -8.6430862467068437e-16,
        -1.1129985821867194e-16,
        -4.5505266221823604e-16,
        3.8885561437496108e-16,
    ]
    c_eff_g = [
        2.033429926215546,
        -2.3123407369310212e-02,
        7.0671915734894875e-03,
        -7.5970017538257162e-04,
        7.4848652541832373e-05,
        4.5642679186460588e-05,
        -2.3097291253000307e-05,
        1.9697221715275770e-06,
        2.4115259271262346e-06,
        -7.1302220919333692e-07,
        -2.5124427621592282e-07,
        1.3246884875139919e-07,
        3.4364196805913849e-08,
        -2.2891359549026546e-08,
        -6.7281240212491156e-09,
        3.8292458615085678e-09,
        1.6451021034313840e-09,
        -5.5868962123284405e-10,
        -4.2052310689211225e-10,
        4.3217612266666094e-11,
        9.9547699528024225e-11,
        1.2882834243832519e-11,
        -1.9103066351000564e-11,
        -7.6805495297094239e-12,
        1.8568853399347773e-12,
    ]
    c_del_f = [
        1.457564928500728,
        -0.2741263150129247,
        1.4102406058428482e-02,
        1.1868136977190956e-02,
        -4.7000120888695418e-03,
        6.7071002620380348e-04,
        1.2315212155928235e-04,
        -8.7985748380390304e-05,
        1.8952644758594150e-05,
        4.4101711646149510e-07,
        -1.5292393205490473e-06,
        4.5050196748941396e-07,
        -2.9971703975339992e-08,
        -2.3573145628841274e-08,
        9.6228336343706644e-09,
        -1.3038786850216866e-09,
        -2.9423462000188749e-10,
        1.8813720970012326e-10,
        -3.7682054143672871e-11,
        -1.9125961925325896e-12,
        3.3516145414580478e-12,
        -9.0842416922143343e-13,
        4.3951786654616853e-14,
        4.5793924208226145e-14,
        -1.4916540225229369e-14,
    ]
    c_del_g = [
        1.980495234559052,
        1.3148750635418816e-02,
        -3.5137830163154959e-03,
        1.4111112411286597e-04,
        -2.4707009281715875e-05,
        -4.9602024972950076e-08,
        1.5268651833078018e-06,
        -4.8070752083129165e-07,
        -3.5826648758785495e-08,
        6.0264253483044428e-08,
        -4.2948016776289677e-09,
        -7.5840171520624722e-09,
        1.0468151234732659e-09,
        1.1267346944343615e-09,
        -1.4810551229871294e-10,
        -1.9605287726598419e-10,
        9.8596597553068932e-12,
        3.6752354493074790e-11,
        3.2634850377633029e-12,
        -6.6207839211074316e-12,
        -1.9158341579839089e-12,
        9.6091495871419851e-13,
        6.3198529742791721e-13,
        -6.4681177081027385e-14,
        -1.8198241524824965e-13,
    ]
    c_xsqr_f = [
        2.675986138240137,
        0.4041429091631520,
        2.1888771714164858e-02,
        -3.4310286472213617e-02,
        9.8724790919419380e-03,
        -7.7267251256297631e-04,
        -4.6681418487147020e-04,
        2.0604262514245964e-04,
        -3.1387761886573218e-05,
        -5.1728966665387510e-06,
        3.9417564710109155e-06,
        -8.6522505504893487e-07,
        -1.6220695979729527e-08,
        6.8546255754808882e-08,
        -2.0405647520593817e-08,
        1.4047699248287415e-09,
        1.0523175986154598e-09,
        -4.3422357653977173e-10,
        5.9649738481937220e-11,
        1.3017424915773290e-11,
        -8.4605289440986553e-12,
        1.7046483669069801e-12,
        8.2185647176657995e-14,
        -1.4448442442471787e-13,
        3.5720454372167865e-14,
    ]
    c_xsqr_g = [
        1.723549588238691,
        0.1365565801015080,
        2.0457962179522337e-03,
        -3.9875695195008110e-04,
        2.3949621855833269e-05,
        -1.6129278268772751e-06,
        -1.1466609509480641e-06,
        4.3086322193297555e-07,
        1.7612995328875059e-09,
        -4.5839686845239313e-08,
        5.9957170539526316e-09,
        5.3204258865235943e-09,
        -1.1050097059595032e-09,
        -7.7028480982566094e-10,
        1.5644044393248180e-10,
        1.3525529252156332e-10,
        -1.5409274967126407e-11,
        -2.6052305868162762e-11,
        -8.3781981352615275e-13,
        4.8823761700234058e-12,
        1.1086589979392158e-12,
        -7.5851658287717783e-13,
        -4.0599884565395428e-13,
        7.9971584909799275e-14,
        1.3500020545897939e-13,
    ]
    c_vx_f = [
        1.226904583058190,
        -0.3621914072547197,
        6.0117947617747081e-02,
        1.8037337764424607e-02,
        -1.4439005957980123e-02,
        3.8147446724517908e-03,
        1.3679160269450818e-05,
        -3.7851338401354573e-04,
        1.3568342238781006e-04,
        -1.3336183765173537e-05,
        -7.5468390663036011e-06,
        3.7919580869305580e-06,
        -6.4560788919254541e-07,
        -1.0509789897250599e-07,
        9.0282233408123247e-08,
        -2.1598200223849062e-08,
        -2.6200750125049410e-10,
        1.8693270043002030e-09,
        -6.0097600840247623e-10,
        4.7263196689684150e-11,
        3.3052446335446462e-11,
        -1.4738090470256537e-11,
        2.1945176231774610e-12,
        4.7409048908875206e-13,
        -3.3502478569147342e-13,
    ]
    c_vx_g = [
        1.862646413811875,
        7.5988886169808666e-02,
        -8.3110620384910993e-03,
        1.1236935254690805e-03,
        -1.0549380723194779e-04,
        -3.8256672783453238e-05,
        2.2883355513325654e-05,
        -2.4595515448511130e-06,
        -2.2063956882489855e-06,
        7.2331970290773207e-07,
        2.2080170614557915e-07,
        -1.2957057474505262e-07,
        -2.9737380539129887e-08,
        2.2171316129693253e-08,
        5.9127004825576534e-09,
        -3.7179338302495424e-09,
        -1.4794271269158443e-09,
        5.5412448241032308e-10,
        3.8726354734119894e-10,
        -4.6562413924533530e-11,
        -9.2734525614091013e-11,
        -1.1246343578630302e-11,
        1.6909724176450425e-11,
        5.6146245985821963e-12,
        -2.7408274955176282e-12,
    ]
    c_vy_f = [
        2.408884004758557,
        0.1441097208627301,
        -5.0093583831079742e-02,
        1.0574012517851051e-02,
        -4.7245491418700381e-04,
        -5.6874753986616233e-04,
        2.2050994176359695e-04,
        -3.0071128379836054e-05,
        -6.5175276460682774e-06,
        4.2908624511150961e-06,
        -8.8327783029362728e-07,
        -3.5778896608773536e-08,
        7.6164115048182878e-08,
        -2.1399959173606931e-08,
        1.1599700144859781e-09,
        1.2029935880786269e-09,
        -4.6385151497574384e-10,
        5.7945164222417134e-11,
        1.5725836188806852e-11,
        -9.1953450409576476e-12,
        1.7449824918358559e-12,
        1.2301937246661510e-13,
        -1.6739387653785798e-13,
        4.5505543777579760e-14,
        -4.3223757906218907e-15,
    ]
    c_vy_g = [
        1.970558139796674,
        1.9874189524780751e-02,
        -5.3520719319403742e-03,
        2.3885486654173116e-04,
        -4.1428357951582839e-05,
        -6.3229035418110869e-07,
        2.8594609307941443e-06,
        -8.5378305322625359e-07,
        -8.2383358224191738e-08,
        1.1218202137786015e-07,
        -6.0736651874560010e-09,
        -1.4453200922748266e-08,
        1.7154640064021009e-09,
        2.1673530992138979e-09,
        -2.4074988114186624e-10,
        -3.7678839381882767e-10,
        1.1723938486696284e-11,
        7.0125182882740944e-11,
        7.5127332133106960e-12,
        -1.2478237332302910e-11,
        -3.8880659802842388e-12,
        1.7635456983633446e-12,
        1.2439449470491581e-12,
        -9.4195068411906391e-14,
        -3.4105815394092076e-13,
    ]
    if alf < 0:
        raise ValueError("alf < 0, invalid choice")
    else:
        if alf <= 9.0:
            eff = (np.pi / 4.00) * alf * chbmts(0.00, 10.00, c_eff_f, 25, alf)
            delta = -0.125 * alf * chbmts(0.00, 10.00, c_del_f, 25, alf)
            xsqr = 0.25 * chbmts(0.00, 10.00, c_xsqr_f, 25, alf)
            vx = 0.25 * chbmts(0.00, 10.00, c_vx_f, 25, alf)
            vy = 0.25 * chbmts(0.00, 10.00, c_vy_f, 25, alf)
        elif alf >= 10.00:
            y = 1.0 - 18.0 / alf
            eff = 1.00 - chbmts(-1.00, 1.00, c_eff_g, 25, y) / alf**2
            delta = (2.0 * chbmts(-1.00, 1.00, c_del_g, 25, y) / alf - 0.25 * np.pi) / eff
            xsqr = ((-np.pi / alf) * chbmts(-1.00, 1.00, c_xsqr_g, 25, y) + 2.0 / 3.0) / eff
            vx = g0 + g1 * chbmts(-1.00, 1.00, c_vx_g, 25, y) / (alf**2)
            vy = (-chbmts(-1.00, 1.00, c_vy_g, 25, y) / (alf**2) + 1.0 / 3.0) / eff
        else:
            eff_f = (np.pi / 4.00) * alf * chbmts(0.00, 10.00, c_eff_f, 25, alf)
            del_f = -0.125 * alf * chbmts(0.00, 10.00, c_del_f, 25, alf)
            xsqr_f = 0.25 * chbmts(0.00, 10.00, c_xsqr_f, 25, alf)
            vx_f = 0.25 * chbmts(0.00, 10.00, c_vx_f, 25, alf)
            vy_f = 0.25 * chbmts(0.00, 10.00, c_vy_f, 25, alf)
            y = 1.0 - 18.0 / alf
            eff_g = 1.00 - chbmts(-1.00, 1.00, c_eff_g, 25, y) / alf**2
            del_g = (2.0 * chbmts(-1.00, 1.00, c_del_g, 25, y) / alf - 0.25 * np.pi) / eff_g
            xsqr_g = ((-np.pi / alf) * chbmts(-1.00, 1.00, c_xsqr_g, 25, y) + 2.0 / 3.0) / eff_g
            vx_g = g0 + g1 * chbmts(-1.00, 1.00, c_vx_g, 25, y) / (alf**2)
            vy_g = (-chbmts(-1.00, 1.00, c_vy_g, 25, y) / (alf**2) + 1.0 / 3.0) / eff_g
            eff = (10.0 - alf) * eff_f + (alf - 9.0) * eff_g
            delta = (10.0 - alf) * del_f + (alf - 9.0) * del_g
            xsqr = (10.0 - alf) * xsqr_f + (alf - 9.0) * xsqr_g
            vx = (10.0 - alf) * vx_f + (alf - 9.0) * vx_g
            vy = (10.0 - alf) * vy_f + (alf - 9.0) * vy_g
    return eff, delta, xsqr, vx, vy


def chbmts(a, b, c, m, x):
    """
    ! Essentially CHEBEV of "Numerical Recipes"
    """
    d = 0.0
    ddd = 0.0
    y = (2.0 * x - a - b) / (b - a)
    for j in range(m - 1, 0, -1):
        sv = d
        d = 2.0 * y * d - ddd + c[j]
        ddd = sv
    out = y * d - ddd + 0.5 * c[0]
    return out


# ------------------------------------------------------------------------------------------------- #
# Sample functions
# ------------------------------------------------------------------------------------------------- #


def sam0(sx, sy, sz, isam):
    """
    ! Calculates the sample time widths
    """
    # Old code
    # varx=1
    # vary=1
    # varz=1
    # RAE code (assumes plate sample, so correct for MAPS vanadium)
    # A more sophisticated version would do different things depending on sample type but usually
    # this contribution is small, and in any case this will be close enough for most geometries
    varx = 0
    # vary = ((sx)**2 + (sy)**2) # WRONG
    vary = (sy**2) * sample_shape_scaling_factors[isam]
    varz = 0
    return varx, vary, varz


# Sample type: 0==flat plate, 1==ellipse, 2==annulus, 3==sphere, 4==solid cylinder
sample_shape_scaling_factors = collections.defaultdict(lambda: 1.0 / 12)
sample_shape_scaling_factors[2] = 1.0 / 8
