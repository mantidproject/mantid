import numpy as np
from matplotlib import pyplot as plt
from matplotlib.pyplot import figure
figure(num=None, figsize=(10, 6), dpi=80, facecolor='w', edgecolor='k')

# PDCalib
# peak_pos   obs_peak_pos     diff_peak_pos            max_y 			fwhm
pdcali_dataset = np.array([
                [0.63073,  0.6303807020787756,   -0.0003492979212244007,   553585.1532951221,  -0.00043600094837896], 
                [0.68665,  0.6863333957275309,   -0.0003166042724690454,    1334892.12795973,  0.002943373084875997], 
                [0.7283 ,  0.72800799447662  ,   -0.0002920055233799345,   2475652.703519975,  0.003198845057664761], 
                [0.81854,  0.8182249919999077,   -0.0003150080000923205,   2368601.200305329,  0.003737520580334892], 
                [0.89198,  0.8916361075538068,   -0.0003438924461931503,  1710967.4781655986,  0.004184149879801434], 
                [1.07577,  1.0752880720224502,   -0.0004819279775496454,   5142679.879816529,  0.005303074272502532], 
                [1.26146,  1.2608124973076278,   -0.0006475026923722371,    6105366.223368138,  0.006412806308394916]])

# Raw
raw_dataset = np.array([
                [0.63073,  0.6307584507809129,   2.8450780912847584e-05,    552221.9167474591,  0.002554611813544497], 
                [0.68665,  0.686740797302528 ,    9.079730252803397e-05,   1323731.6681532238,  0.002962362343038924], 
                [0.7283 ,  0.7284387141584797,   0.0001387141584797691 ,    2467733.771307486,  0.003219002758277547], 
                [0.81854,  0.8187098390887974,   0.0001698390887973078 ,    2351441.406881138,  0.003759483235260068], 
                [0.89198,  0.8921642040404092,   0.00018420404040919713,   1706624.7479229213,  0.004205263025430606], 
                [1.07577,  1.075918896113921 ,  0.00014889611392110425 ,    5111351.790345363,  0.005333044365854872], 
                [1.26146,  1.2615463729060852,   8.637290608515258e-05 ,    6080796.395566094,  0.006445465267116806]])

                                                                                                     
# CC Mask
cc_mask_dataset = np.array([
                [0.63073,  0.6307896347035312,   5.9634703531208366e-05,   554973.0323885417 , 0.0025386116483661803], 
                [0.68665,  0.6867726531874018,   0.00012265318740178888,   1334176.5308801457, 0.00294520292656294  ], 
                [0.7283 ,  0.7284735240605069,   0.00017352406050696878,   2478911.920140646 , 0.0031998628715363174], 
                [0.81854,  0.8187529631442069,   0.0002129631442068236 ,   2368961.640202556 , 0.0037370685723165068], 
                [0.89198,  0.8922128005178845,   0.00023280051788454603,   1714116.8729173462, 0.00418317120911615  ], 
                [1.07577,  1.0759853055120074,   0.00021530551200754644,   5147351.920196313 , 0.005300722264655629 ], 
                [1.26146,  1.2616329095773429,   0.00017290957734283907,   6117945.921259753 , 0.006407252017960412 ]])
                                                                                                     
# CC Fallback
cc_fallback_dataset = np.array([
                [0.63073,  0.6307899323866734,   5.993238667334477e-05 ,    554972.2407970446,  0.002538504291205891], 
                [0.68665,  0.6867729568521024,   0.00012295685210239604,   1334084.6546394036,  0.002945119375776733], 
                [0.7283 ,  0.7284732162673686,   0.0001732162673686588 ,    2478711.888343714,  0.003199771317372901], 
                [0.81854,  0.8187527149839314,   0.00021271498393138355,    2368780.154428059,  0.003736922925308937], 
                [0.89198,  0.8922122238675573,   0.00023222386755727964,   1713887.8449825554,  0.004183177428430089], 
                [1.07577,  1.075985412814431 ,   0.00021541281443115956,    5146270.920487481,  0.005300806541555302], 
                [1.26146,  1.2616316490634303,   0.000171649063430257  ,    6117397.696298834,  0.006405902746057972]])


# plot peak position to expected
title = 'Absolute difference from observed peak position to expected peak position: West bank (1)'
label_y = 'Delta peak position (A)'
out_png = 'delta_peak_pos.png'

# (2)
# title = 'Ratio from observed peak position to expected peak position: West bank (1)'
# label_y = 'observed peak position / expected peak position'
# out_png = 'delta_d_over_d.png'
# 
# (3)
# title = 'Ratio from observed peak height to uncalibrated peak height: West bank (1)'
# label_y = 'observed peak height / uncalibrated peak height'
# out_png = 'peak_height.png'
# 
# (4)
# title = 'Ratio from observed peak width to uncalibrated peak width: West bank (1)'
# label_y = 'observed peak width / uncalibrated peak width'
# out_png = 'peak_width.png'

vec_x = raw_dataset[:, 0]
for dataset, name, color in [(raw_dataset, 'Raw', 'black'),
                             (pdcali_dataset, 'PDCalibration', 'red'),
                             (cc_mask_dataset, 'Cross-correlation (mask incorrect DIFC)', 'blue'),
                             (cc_fallback_dataset, 'Cross-correlation (fallback incorrect DIFC)', 'green')]:
    # (1)
    vec_y = dataset[:, 2]

    # (2) vec_y = dataset[:, 1] / vec_x

    # (3) vec_y = dataset[:, 3] / raw_dataset[:, 3]

    # (4)
    # vec_y = dataset[:, 4] / raw_dataset[:, 4]
    # vec_x = dataset[:, 0]
    # vec_x = vec_x[vec_y > 0]
    # vec_y = vec_y[vec_y > 0]

    if name == 'PDCalibration':
        plt.plot(vec_x, vec_y, color=color, label=name, linestyle='None', marker='D')
    else:
        plt.plot(vec_x, vec_y, color=color, label=name, linestyle='None', marker='o')


plt.title(title)
plt.xlabel('dSpacing (A)')
plt.ylabel(label_y)
plt.legend()
plt.savefig(out_png)
plt.show()
