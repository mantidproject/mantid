# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import tempfile

base_user_file = ("PRINT for changer\n"
                  "MASK/CLEAR \n"
                  "MASK/CLEAR/TIME\n"
                  "L/WAV 1.5 12.5 0.125/LIN\n"
                  "L/Q .001,.001, .0126, -.08, .2\n"
                  "!L/Q .001 .8 .08/log\n"
                  "L/QXY 0 0.05 .001/lin\n"
                  "BACK/M1 35000 65000\n"
                  "BACK/M2 85000 98000\n"
                  "BACK/MON/TIMES 3500 4500\n"
                  "BACK/TRANS 123 466\n"
                  "DET/REAR\n"
                  "GRAVITY/{}\n"
                  "!FIT/TRANS/OFF\n"
                  "FIT/TRANS/LOG 1.5 12.5\n"
                  "FIT/MONITOR 1000 2000\n"
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
                  "L/Q/RCut 200\n"
                  "L/Q/WCut 8.0\n"
                  "!PRINT REMOVED RCut=200 WCut=8\n"
                  "!\n"
                  "MON/DIRECT=DIRECTM1_15785_12m_31Oct12_v12.dat\n"
                  "MON/TRANS/SPECTRUM=1/INTERPOLATE\n"
                  "MON/SPECTRUM=1/INTERPOLATE\n"
                  "!TRANS/TRANSPEC=3\n"
                  "TRANS/TRANSPEC=4/SHIFT=-70\n"
                  "TRANS/RADIUS=7.0\n"
                  "TRANS/ROI=test.xml, test2.xml\n"
                  "TRANS/MASK=test3.xml, test4.xml\n"
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
                  "L/EVENTSTIME 7000.0,500.0,60000.0\n"
                  "SAMPLE/PATH/ON\n"
                  "QRESOL/ON \n"
                  "QRESOL/DELTAR=11 \n"
                  "QRESOL/LCOLLIM=12 \n"
                  "QRESOL/MODERATOR=moderator_rkh_file.txt\n"
                  "QRESOL/A1=13\n"
                  "QRESOL/A2=14\n"
                  "TUBECALIBFILE=TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
                  )


def create_user_file(user_file_content):
    temp = tempfile.NamedTemporaryFile(mode='w+', delete=False)
    temp.write(user_file_content)
    user_file_path = temp.name
    temp.close()
    return user_file_path


def make_sample_user_file(gravity ='ON'):
    return base_user_file.format(gravity)

sample_user_file = make_sample_user_file(gravity ='ON')
sample_user_file_gravity_OFF = make_sample_user_file(gravity ='OFF')
