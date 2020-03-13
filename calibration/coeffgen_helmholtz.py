#%%
import numpy as np
import matplotlib.pyplot as plt
import scipy as scp
from scipy.odr import *


# %%
data = np.loadtxt('coil_data.csv', delimiter=',').transpose()
data.shape

# %%
plt.plot(data[2])
plt.plot(data[3])
plt.plot(data[4])
# %%
volt_0 = data[1]

bx = data[2]
by = data[3]
bz = data[4]

bx2 = data[5]
by2 = data[6]
bz2 = data[7]

# %%
sbx = np.sqrt(bx2 - bx*bx)
sby = np.sqrt(by2 - by*by)
sbz = np.sqrt(bz2 - bz*bz)

# %%
volt = volt_0[np.where(data[0]==0)] - 2048

bx_0 = bx[np.where(data[0]==0)]
bx_1 = bx[np.where(data[0]==1)]
bx_2 = bx[np.where(data[0]==2)]

sbx_0 = sbx[np.where(data[0]==0)]
sbx_1 = sbx[np.where(data[0]==1)]
sbx_2 = sbx[np.where(data[0]==2)]

by_0 = by[np.where(data[0]==0)]
by_1 = by[np.where(data[0]==1)]
by_2 = by[np.where(data[0]==2)]

sby_0 = sby[np.where(data[0]==0)]
sby_1 = sby[np.where(data[0]==1)]
sby_2 = sby[np.where(data[0]==2)]

bz_0 = bz[np.where(data[0]==0)]
bz_1 = bz[np.where(data[0]==1)]
bz_2 = bz[np.where(data[0]==2)]

sbz_0 = sbz[np.where(data[0]==0)]
sbz_1 = sbz[np.where(data[0]==1)]
sbz_2 = sbz[np.where(data[0]==2)]

# %%
plt.errorbar(volt, bx_0, yerr=sbx_0, ls='', marker='')
plt.plot(volt, bx_0)

# %%
coeff, covar = np.polyfit(volt[10:70], bx_0[10:70], 1, full=False, cov=True)

# %%
covar

# %%
ax_0 = coeff[0]; cx_0 = coeff[1]

# %%
coeff, covar = np.polyfit(volt[10:70], bx_1[10:70], 1, full=False, cov=True)

# %%
covar

# %%
ax_1 = coeff[0]; cx_1 = coeff[1]

# %%
coeff, covar = np.polyfit(volt[10:70], bx_2[10:70], 1, full=False, cov=True)
print(covar)
ax_2 = coeff[0]; cx_2 = coeff[1]


# %%
coeff, covar = np.polyfit(volt[10:70], by_0[10:70], 1, full=False, cov=True)
print(covar)
ay_0 = coeff[0]; cy_0 = coeff[1]

coeff, covar = np.polyfit(volt[10:70], by_1[10:70], 1, full=False, cov=True)
print(covar)
ay_1 = coeff[0]; cy_1 = coeff[1]

coeff, covar = np.polyfit(volt[10:70], by_2[10:70], 1, full=False, cov=True)
print(covar)
ay_2 = coeff[0]; cy_2 = coeff[1]


# %%
coeff, covar = np.polyfit(volt[10:70], bz_0[10:70], 1, full=False, cov=True)
print(covar)
az_0 = coeff[0]; cz_0 = coeff[1]

coeff, covar = np.polyfit(volt[10:70], bz_1[10:70], 1, full=False, cov=True)
print(covar)
az_1 = coeff[0]; cz_1 = coeff[1]

coeff, covar = np.polyfit(volt[10:70], bz_2[10:70], 1, full=False, cov=True)
print(covar)
az_2 = coeff[0]; cz_2 = coeff[1]

# %%
print(ax_0, cx_0)
print(ax_1, cx_1)
print(ax_2, cx_2)

print(ay_0, cy_0)
print(ay_1, cy_1)
print(ay_2, cy_2)

print(az_0, cz_0)
print(az_1, cz_1)
print(az_2, cz_2)
# %%
plt.plot(bx_0)
plt.plot(ax_0*volt+cx_0)
plt.show()

plt.plot(bx_1)
plt.plot(ax_1*volt+cx_1)
plt.show()

plt.plot(bx_2)
plt.plot(ax_2*volt+cx_2)
plt.show()

plt.plot(by_0)
plt.plot(ay_0*volt+cy_0)
plt.show()

plt.plot(by_1)
plt.plot(ay_1*volt+cy_1)
plt.show()

plt.plot(by_2)
plt.plot(ay_2*volt+cy_2)
plt.show()

plt.plot(bz_0)
plt.plot(az_0*volt+cz_0)
plt.show()

plt.plot(bz_1)
plt.plot(az_1*volt+cz_1)
plt.show()

plt.plot(bz_2)
plt.plot(az_2*volt+cz_2)
plt.show()

# %%
A = np.array([[ax_0, ax_1, ax_2], [ay_0, ay_1, ay_2], [az_0, az_1, az_2]])
print(A)
# %%
# The 1/3 comes from the fact that cx_0 ~= cx_1 ~= cx_2 ~= B_0x, and when coil voltage
# is zero, B = B_0x
C = np.array([[cx_0+cx_1+cx_2],[cy_0+cy_1+cy_2],[cz_0+cz_1+cz_2]])*(1/3)
print(C)
# %%
A

# %%
ax_0, ax_1, ax_2

# %%
C

# %%
np.linalg.inv(A)

# %%
-np.matmul(np.linalg.inv(A), C) + 2048

# %%
"""
Results:
A = np.array([[-0.01170485,  0.007626  ,  0.78702451],
       [ 0.96502251,  0.01742897, -0.04614586],
       [ 0.00132067, -0.49386824,  0.01514241]])

C = np.array([[ 609.47766103],
       [  13.89526469],
       [-310.98467566]])

InvA = np.array([[ 0.0600763 ,  1.03692257,  0.03752142],
       [ 0.03913433,  0.00324473, -2.02411277],
       [ 1.27112273,  0.01538996,  0.02017099]])

vec(Coil) = InvA * (vec(B) - vec(C)) + 2048 [ B, C in mG]
vec(B) = A * vec(Coil -2048) + vec(C)
"""

# %%
def getB(val):
       global A, C
       val = np.array(val)
       val.shape = (3,1)
       return np.matmul(np.linalg.inv(A),(val - C))


# %%
getB([0, 0, 0])

# %%
getB([1000,-7000,-3000])

# %%
np.matmul(A,np.array([[100],[100],[100]]))+C

# %%
getB([0, 0, 0])

# %%
np.matmul(A, np.array([[1930],[ 88],[-200]]))+C

# %%
A

# %%
C

# %%
np.matmul(A, np.array([[2008],[1396],[1279]])-2048)+C

# %%
