from __future__ import (absolute_import, division, print_function)
import numpy as np
from CrystalField.energies import energies as CFEnergy
from CrystalField.fitting import ionname2Nre
from six import iteritems
from six import string_types


def _get_normalisation(nre, bnames):
    """ Helper function to calculate the normalisation factor.
        Defined as: ||Blm|| = sum_{Jz,Jz'} |<Jz|Blm|Jz'>|^2 / (2J+1)
    """
    Jvals = [0, 5./2, 4, 9./2, 4, 5./2, 0, 7./2, 6, 15./2, 8, 15./2, 6, 7./2]
    J = (-nre / 2.) if (nre < 0) else Jvals[nre]
    retval = {}
    for bname in bnames:
        bdict = {bname: 1}
        ee, vv, ham = CFEnergy(nre, **bdict)
        Omat = np.mat(ham)
        norm = np.trace(np.real(Omat * np.conj(Omat))) / (2*J+1)
        retval[bname] = np.sqrt(np.abs(norm)) * np.sign(norm)
    return retval


def _parse_args(**kwargs):
    """ Parses input arguments for stev2norm() and norm2stev().
    """
    # Some definitions
    Blms = ['B20', 'B21', 'B22', 'B40', 'B41', 'B42', 'B43', 'B44', 'B60', 'B61', 'B62', 'B63', 'B64', 'B65', 'B66']
    # Some Error checking
    if 'Ion' not in kwargs.keys() and 'IonNum' not in kwargs.keys():
        raise NameError('You must specify the ion using either the ''Ion'', ''IonNum'' keywords')
    if 'Ion' in kwargs.keys():
        nre = ionname2Nre(kwargs['Ion'])
    else:
        nre = kwargs['IonNum']
    # Now parses the list of input crystal field parameters
    par_names = []
    Blm = {}
    for pname in Blms:
        if pname in kwargs.keys():
            par_names.append(pname)
            Blm[pname] = kwargs[pname]
    if not par_names:
        if 'B' in kwargs.keys():
            for ind, pname in enumerate(Blms):
                par_names.append(pname)
                Blm[pname] = kwargs['B'][ind]
        else:
            raise NameError('You must specify at least one input Blm parameter')
    return nre, par_names, Blm


def stev2norm(**kwargs):
    """ Calculates the "normalised" crystal field parameters of P. Fabi
        These parameters are defined in the appendix of the FOCUS program manual,
        http://purl.org/net/epubs/manifestation/5723 page 59.

        nlm = stev2norm(Ion=ionname, B=bvec)
        nlm = stev2norm(Ion=ionname, B20=b20val, ...)
        nlm = stev2norm(IonNum=ionnumber, B20=b20val, ...)
        [B20, B40] = stev2norm(IonNum=ionnumber, B20=b20val, B40=b40val, OutputTuple=True)

        Note: This function only accepts keyword inputs.

        Inputs:
            ionname - name of the (tripositive) rare earth ion, e.g. 'Ce', 'Pr'.
            ionnumber - the number index of the rare earth ion:
                   1=Ce 2=Pr 3=Nd 4=Pm 5=Sm 6=Eu 7=Gd 8=Tb 9=Dy 10=Ho 11=Er 12=Tm 13=Yb
            bvec - a vector of the Stevens CF parameters in order: [B20 B21 B22 B40 B41 ... etc.]
                    This vector can also a be dictionary instead {'B20':1, 'B40':2}
            b20val etc. - values of the Stevens CF parameters to be converted

        Outputs:
            nlm - a dictionary of the normalised crystal field parameters (default)
            [B20, etc] - a tuple of the normalised crystal field parameters (need to set OutputTuple flag)

        Note: one of the keywords: Ion and IonNum must be specified.

    """
    # Parses the input parameters
    nre, par_names, Blm = _parse_args(**kwargs)
    # Gets the normalisation constants
    norm = _get_normalisation(nre, par_names)
    # Calculates the normalised parameters.
    Nlm = {}
    for pname in par_names:
        Nlm[pname] = Blm[pname] * norm[pname]

    return Nlm


def norm2stev(**kwargs):
    """ Calculates the Stevens (conventional) crystal field parameters from "normalised" parameters
        The normalised parameters of P. Fabi are defined in the appendix of the FOCUS program manual,
        http://purl.org/net/epubs/manifestation/5723 page 59.

        nlm = norm2stev(Ion=ionname, B=bvec)
        nlm = norm2stev(Ion=ionname, B20=b20val, ...)
        nlm = norm2stev(IonNum=ionnumber, B20=b20val, ...)
        [B20, B40] = norm2stev(IonNum=ionnumber, B20=b20val, B40=b40val, OutputTuple=True)

        Note: This function only accepts keyword inputs.

        Inputs:
            ionname - name of the (tripositive) rare earth ion, e.g. 'Ce', 'Pr'.
            ionnumber - the number index of the rare earth ion:
                   1=Ce 2=Pr 3=Nd 4=Pm 5=Sm 6=Eu 7=Gd 8=Tb 9=Dy 10=Ho 11=Er 12=Tm 13=Yb
            bvec - a vector of the Stevens CF parameters in order: [B20 B21 B22 B40 B41 ... etc.]
                    This vector can also a be dictionary instead {'B20':1, 'B40':2}
            b20val etc. - values of the Stevens CF parameters to be converted

        Outputs:
            nlm - a dictionary of the normalised crystal field parameters (default)
            [B20, etc] - a tuple of the normalised crystal field parameters (need to set OutputTuple flag)

        Note: one of the keywords: Ion and IonNum must be specified.

    """
    # Parses the input parameters
    nre, par_names, Blm = _parse_args(**kwargs)
    # Gets the normalisation constants
    norm = _get_normalisation(nre, par_names)
    # Calculates the normalised parameters.
    Nlm = {}
    for pname in par_names:
        Nlm[pname] = 0 if norm[pname] == 0 else Blm[pname] / norm[pname]

    return Nlm


def split2range(*args, **kwargs):
    """ Calculates the ranges of (Stevens) crystal field parameters to give energy splittings
        around a specified a desired energy splitting

        ranges = split2range(ionname, energy_splitting, bnames)
        ranges = split2range(Ion=ionname, EnergySplitting=energy_splitting, Parameters=bnames)
        ranges = split2range(IonNum=ionnumber, EnergySplitting=energy_splitting, Parameters=bnames)
        ranges = split2range(PointGroup=point_group, EnergySplitting=energy_splitting, 'B20', 'B22', ...)
        ranges = split2range(PointGroup=point_group, EnergySplitting=energy_splitting, B20=_, B22=_, ...)
        ranges = split2range(..., Output='dictionary')
        constraint_string = split2range(..., Output='constraints')

        Inputs:
            ionname - name of the (tripositive) rare earth ion, e.g. 'Ce', 'Pr'.
            ionnumber - the number index of the rare earth ion:
                1=Ce 2=Pr 3=Nd 4=Pm 5=Sm 6=Eu 7=Gd 8=Tb 9=Dy 10=Ho 11=Er 12=Tm 13=Yb
            energy_splitting - the desired energy splitting (difference between the energies of the
                highest and lowest crystal field energy levels) in meV
            bnames - a list of names of the crystal field parameters to give the range for
            'B20', etc - the names of the parameters can be given as individual arguments or
                keyword arguments. In the case of keyword arguments, only the key names will be used

        Output:
            ranges - a dictionary of the ranges such that sampling uniformly in |Blm|<range[bnames]
                will produce crystal field splittings distributed normally around the input
                energy_splitting.
            constraint_string - a string of the form '-[range]<Blm<[range]' where [range] is the
                calculated range for that parameter.
            By default, the Output keyword is set to "dictionary".

        Note: ionname or ionnumber must be specified.
    """
    argin = {}
    argnames = ['Ion', 'EnergySplitting', 'Parameters', 'Output', 'IonNum']
    # Set default values.
    argin['Output'] = 'dictionary'
    argin['Parameters'] = []
    # Parses the non-keyword arguments
    for ia in range(3 if len(args)>3 else len(args)):
        argin[argnames[ia]] = args[ia]
    # Further arguments beyond the first 3 are treated as crystal field parameter names
    for ia in range(3, len(args)):
        if isinstance(args[ia], string_types) and args[ia].startswith('B'):
            argin['Parameters'].append(args[ia])
    # Parses the keyword arguments
    for key, value in iteritems(kwargs):
        for ia in range(len(argnames)):
            if key == argnames[ia]:
                argin[key] = value
                break
        if key.startswith('B'):
            argin['Parameters'].append(key)

    # Error checking
    if 'Ion' not in argin.keys() and 'IonNum' not in argin.keys():
        raise NameError('You must specify the ion using either the ''Ion'', ''IonNum'' keywords')
    if 'Ion' in argin.keys():
        nre = ionname2Nre(kwargs['Ion'])
    else:
        nre = argin['IonNum']
    if 'EnergySplitting' not in argin.keys():
        raise NameError('You must specify the desired energy splitting')
    if not argin['Parameters']:
        raise NameError('You must specify at least one crystal field parameter name')

    Nlm = {}
    for bname in set(argin['Parameters']):
        Nlm[bname] = 1
    Blm = norm2stev(IonNum=nre, **Nlm)
    ee, vv, ham = CFEnergy(nre, **Blm)
    # Factor of 2 is needed to get the Gaussian centred on the desired energy splitting.
    splitting_factor = 2 * argin['EnergySplitting'] / (np.max(ee-np.min(ee)))
    Nlm = {}
    for bname in Blm.keys():
        Nlm[bname] = splitting_factor
    ranges = norm2stev(IonNum=nre, **Nlm)

    if argin['Output'].lower() is 'constraints':
        constr = ''
        for bname in ranges.keys():
            constr += '%.4g<%s<%.4g,' % (-ranges[bname], bname, ranges[bname])
        return constr[:-1]
    else:
        return ranges
