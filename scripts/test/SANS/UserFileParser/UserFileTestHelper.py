import os
import mantid

sample_user_file = ("PRINT for changer\n"
                    "MASK/CLEAR \n"
                    "MASK/CLEAR/TIME\n"
                    "L/WAV 1.5 12.5 0.125/LIN\n"
                    "L/Q .001,.001, .0126, -.08, .2\n"
                    "!L/Q .001 .8 .08/log\n"
                    "L/QXY 0 0.05 .001/lin\n"
                    "BACK/M1 35000 65000\n"
                    "BACK/M2 85000 98000\n"
                    "DET/REAR\n"
                    "GRAVITY/ON\n"
                    "!FIT/TRANS/OFF\n"
                    "FIT/TRANS/LOG 1.5 12.5\n"
                    "mask/rear h0\n"
                    "mask/rear h190>h191\n"
                    "mask/rear h167>h172\n"
                    "mask/rear v0\n"
                    "mask/rear v191\n"
                    "mask/front h0\n"
                    "mask/front h190>h191\n"
                    "mask/front v0\n"
                    "mask/front v191\n"
                    "! dead wire near top\n"
                    "mask/front h156>h159\n"
                    "!masking off beamstop arm - 12mm wide @ 19degrees\n"
                    "!mask/rear/line 12 19\n"
                    "! spot on rhs beam stop at 11m\n"
                    "! mask h57>h66+v134>v141\n"
                    "!\n"
                    "! mask for Bragg at 12m, 26/03/11, 3 time channels\n"
                    "mask/time 17500 22000\n"
                    "!\n"
                    "L/R 12 15\n"
                    "!L/Q/RCut 200\n"
                    "!L/Q/WCut 8.0\n"
                    "!PRINT REMOVED RCut=200 WCut=8\n"
                    "!\n"
                    "MON/DIRECT=DIRECTM1_15785_12m_31Oct12_v12.dat\n"
                    "MON/TRANS/SPECTRUM=1/INTERPOLATE\n"
                    "MON/SPECTRUM=1/INTERPOLATE\n"
                    "!TRANS/TRANSPEC=3\n"
                    "TRANS/TRANSPEC=4/SHIFT=-70\n"
                    "!\n"
                    "set centre 155.45 -169.6\n"
                    "!\n"
                    "! 25/10/13 centre gc 22021, fit gdw20 22023\n"
                    "set scales 0.074 1.0 1.0 1.0 1.0\n"
                    "! correction to actual sample position, notionally 81mm before shutter\n"
                    "SAMPLE/OFFSET +53.0\n"
                    "! Correction to SANS2D encoders in mm\n"
                    "DET/CORR/REAR/X -16.0\n"
                    "DET/CORR/REAR/Z 47.0\n"
                    "DET/CORR/FRONT/X -44.0\n"
                    "DET/CORR/FRONT/Y -20.0\n"
                    "DET/CORR/FRONT/Z 47.0\n"
                    "DET/CORR/FRONT/ROT 0.0\n"
                    "!\n"
                    "!! 01/10/13 MASKSANS2d_133F M3 by M1 trans Hellsing, Rennie, Jackson, L1=L2=12m A1=20 and A2=8mm\n"
                    "L/EVENTSTIME 7000.0,500.0,60000.0\n")


def create_user_file(user_file_content):
    user_file_path = os.path.join(mantid.config.getString('defaultsave.directory'), 'sample_sans_user_file.txt')
    if os.path.exists(user_file_path):
        os.remove(user_file_path)

    with open(user_file_path, 'w') as f:
        f.write(user_file_content)

    return user_file_path

