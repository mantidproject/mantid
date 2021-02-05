import numpy as np
import matplotlib.pyplot as plt
from scipy import stats


# West Bank PD Calibration
pdcali_dataset = np.array([
                [0.63073,  0.6303807020787756,   -0.0003492979212244007,   553585.1532951221,  -0.00043600094837896], 
                [0.68665,  0.6863333957275309,   -0.0003166042724690454,    1334892.12795973,  0.002943373084875997], 
                [0.7283 ,  0.72800799447662  ,   -0.0002920055233799345,   2475652.703519975,  0.003198845057664761], 
                [0.81854,  0.8182249919999077,   -0.0003150080000923205,   2368601.200305329,  0.003737520580334892], 
                [0.89198,  0.8916361075538068,   -0.0003438924461931503,  1710967.4781655986,  0.004184149879801434], 
                [1.07577,  1.0752880720224502,   -0.0004819279775496454,   5142679.879816529,  0.005303074272502532], 
                [1.26146,  1.2608124973076278,   -0.0006475026923722371,    6105366.223368138,  0.006412806308394916]])

y = pdcali_dataset[:, 0]
x = pdcali_dataset[:, 1]

# linear regression
res = stats.linregress(x, y)
print(f'slope:        b = {res.slope}')
print(f'interception: a = {res.intercept}')

# plot
plt.plot(x, y, 'o', label='original data')
plt.plot(x, res.intercept + res.slope*x, 'r', label='fitted line')
plt.legend()
plt.show()

# Check
new_x = res.slope * x + res.intercept
percent_relative_diff = (new_x - y) / y * 100.
for i in range(len(new_x)):
    print(f'{y[i]}: {percent_relative_diff[i]:.5f}%')

