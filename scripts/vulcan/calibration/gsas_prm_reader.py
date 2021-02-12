# Read a GSAS instrument parameter file (prm) file and
# parse the Rietveld peak profile
from typing import List
from collections import namedtuple
import math

GSAS_BACK_TO_BACK_EXPONENTIAL_PARAMS = ['alp', 'bet_0', 'bet_1', 'sig_0', 'sig_1', 'sig_2',
                                        'gam_0', 'gam_1', 'gam_2', 'g1ec', 'g2ec', 'gsf', 'rstr', 'rsta', 'rsca',
                                        'L11', 'L22', 'L33', 'L12', 'L13', 'L23']
GSASBackToBackExponential = namedtuple('GSASBackToBackExponential', GSAS_BACK_TO_BACK_EXPONENTIAL_PARAMS)
BackToBackExponentialParameter = namedtuple('BackToBackExponentialParameter', ['A', 'B', 'S'])


def main(prm_file_name):

    # Load file
    with open(prm_file_name) as prm_file:
        prm_lines = prm_file.readlines()

    # Scan prm file text
    vulcan_profile_dict = scan_prm_content(prm_lines)

    for bank_id in [3]:
        calculate_vulcan_peak_parameters(vulcan_profile_dict[bank_id], 1.75)

    return


def calculate_vulcan_peak_parameters(gsas_profile: GSASBackToBackExponential,
                                     dspace: float):
    """

    https://usermanual.wiki/Pdf/GSAS20Manual.1353272546/html

    Parameters
    ----------
    gsas_profile
    dspace

    Returns
    -------

    """

    print(f'alpha = {gsas_profile.alp}, beta0 = {gsas_profile.bet_0}, beta1 = {gsas_profile.bet_1}, '
          f'sigma0 = {gsas_profile.sig_0}, sigma1 = {gsas_profile.sig_1},'
          f'sigma2 = {gsas_profile.sig_2}')

    # A(d) = alpha_1 / d
    alpha = gsas_profile.alp / dspace
    beta = gsas_profile.bet_0 + gsas_profile.bet_1 / dspace**4
    sq_sigma = gsas_profile.sig_0**2 + gsas_profile.sig_1 * dspace**2 + gsas_profile.sig_2**2 * dspace**4

    print(f'@ d = {dspace}: A = {alpha}, B = {beta}, S = {math.sqrt(sq_sigma)}')


def scan_prm_content(content: List[str]):

    # Determine number of banks
    num_banks = -1
    last_visited_line_index = 0
    print(f'total lines = {len(content)}')
    for line_index, line in enumerate(content):
        print(line)
        if line.startswith('INS') and line.count('BANK') == 1:
            terms = line.split()
            num_banks = int(terms[2])
            last_visited_line_index = line_index
            break
    assert num_banks > 0, f'Unable to find "INS  BANK  " line'

    # Index the important lines
    profile_param_dict = dict()
    for line in content[last_visited_line_index:]:
        if line.startswith('INS') and line.count('PRCF')  > 0:
            terms = line.split()
            profile_param_dict[terms[1]] = terms[2:]

    profile_dict = dict()
    for bank_id in range(1, num_banks + 1):
        profile = parse_gsas_profile_3(profile_param_dict, bank_id)
        profile_dict[bank_id] = profile

    return profile_dict


def parse_gsas_profile_3(prm_line_dict, bank_id):
    """

    The doc says that there are 21 parameters and also mentions that they are
    'alp', 'bet-0', 'bet-1', 'sig-0', 'sig-1','sig-2',
    'gam-0', 'gam-1', 'gam-2', 'g1ec', 'g2ec', 'gsf', 'rstr', 'rsta', 'rsca',
    'L11','L22', 'L33', 'L12', 'L13', and 'L23',

    In PRM file:
    INS  1PRCF11   0.100000E+01   0.381119E-01   0.856975E-02   0.000000E+00
    INS  1PRCF12   0.527898E+03   0.200103E+03   0.000000E+00   0.483957E+01
    INS  1PRCF13  0.258743E+001  0.000000E+000  0.000000E+000  0.000000E+000
    INS  1PRCF14  0.000000E+000  0.000000E+000  0.000000E+000  0.000000E+000
    INS  1PRCF15  0.000000E+000  0.000000E+000  0.000000E+000  0.000000E+000
    INS  1PRCF16  0.000000E+000

    Parameters
    ----------
    prm_line_dict
    bank_id

    Returns
    -------

    """
    param_dict = dict()

    # Create parameter dictionary
    num_lines = 6
    param_index = 0
    for line_index in range(1, num_lines + 1):
        key = f'{bank_id}PRCF1{line_index}'
        terms = prm_line_dict[key]
        for value in terms:
            param_dict[GSAS_BACK_TO_BACK_EXPONENTIAL_PARAMS[param_index]] = float(value)
            param_index += 1

    return GSASBackToBackExponential(**param_dict)


if __name__ in ['__main__', 'mantidqt.widgets.codeeditor.execution']:
    pre_x_prm = 'Vulcan-189092-s.prm'
    main(pre_x_prm)
