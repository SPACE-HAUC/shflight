#%%
import numpy as np
import matplotlib.pyplot as plt

# %%
data = np.loadtxt('calib0.txt', delimiter=',').transpose()
print(data.shape)

# %%
#print raw data
plt.figure(figsize=(10,10))
for i in range(1, 10):
    plt.plot(data[0], data[i], label="Sensor %d"%i)
plt.legend()
plt.show()

# %%
#determine cutoff for each sensor
cutoff_index = [0]
for i in range(1,10):
    cutoff_index.append(np.where(data[i]==65536)[0][0])
cutoff_index = np.array(cutoff_index, dtype=int)

# %%
# calculate slope and intercept of the inverse problem!
a = [0.]
c = [0.]
covs = [np.array([[0., 0.], [0., 0.]])]
for i in range(1,10):
    (sl, incpt), cov = np.polyfit(data[i][0:cutoff_index[i]-2], data[0][0:cutoff_index[i]-2], deg=1, cov=True)
    a.append(sl)
    c.append(incpt)
    covs.append(cov)


# %%
plt.figure(figsize=(10,10))
colors = ['', 'b', 'g', 'r', 'c', 'm', 'y', 'k', 'gray', '#00c9ff']
for i in range(1, 10):
    plt.plot(data[i], data[0], label="Sensor %d"%i, color=colors[i])
    plt.plot(data[i], a[i]*data[i]+c[i], label="Fit %d"%i, color=colors[i])
plt.legend()
plt.xlabel("Lux")
plt.ylabel("PWM Value")
plt.show()

# %%
print(a)
print(c)

# %%
print(covs)

# %%
