#modified in 07/06/18
import numpy as np
from scipy.integrate import simps
from scipy import stats
import os
import matplotlib.pyplot as plt

# only take in L = 1 channel
def compute_integral(fx, rhov):
	a = simps( rhov * fx ** 3, fx)
	b = np.sqrt(4 * np.pi / 3)
	return a * b

N = int(raw_input("N= "))
Z = int(raw_input("Z= "))
x_data = [0.08, 0.10,0.12,0.15]  #need to add xdata
y_data = []
path = raw_input()
names = os.listdir(path)
names.sort()
names = [i for i in names if i.split('.')[-1] == 'txt']
for i in names:
	print i
	data = np.loadtxt(path + i)
	fx = data[:,0]
	rho3 = data[:,3]   #this is den3
	y_data.append(compute_integral(fx, rho3))
slope, intercept, r_value, p_value, std_err =  stats.linregress(x_data, y_data)
print stats.linregress(x_data, y_data)
plt.plot(x_data, y_data, 'k.', x_data, y_data, 'g-')
# plt.xlim(0,0.15)
# plt.ylim(-0.3, 0.3)
plt.show()
m2 = abs(0.5 * slope)
m1 = 14.8 * N * Z / (N + Z)
print "Econ = %.2f" % (m1/m2)**0.5
print "M-1 = %.2f" %m2