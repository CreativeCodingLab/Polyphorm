import numpy as np
from random import *

fileName = 'poisson_4096_2d'
SCALE_MPC = 100.0
DISTRIBUTION_BIAS = 3.0

with open(fileName + '.dat') as f:
    lines = f.readlines()
    n_points = len(lines)
    data_regular = np.zeros((n_points, 4), dtype=np.float32)
    data_random = np.zeros((n_points, 4), dtype=np.float32)
    data_poisson = np.zeros((n_points, 4), dtype=np.float32)

    n_points_per_dim = int( 0.5 + float(n_points) ** (1.0/3.0) )
    dim_step = 1.0 / float(n_points_per_dim - 1)
    for i in range(n_points_per_dim):
        for j in range(n_points_per_dim):
            for k in range(n_points_per_dim):
                x = SCALE_MPC * i * dim_step
                y = SCALE_MPC * j * dim_step
                z = SCALE_MPC * ( (k * dim_step) ** DISTRIBUTION_BIAS )
                if DISTRIBUTION_BIAS > 1.0:
                    mass = z / SCALE_MPC
                else:
                    mass = 1.0
                data_regular[i*n_points_per_dim*n_points_per_dim + j*n_points_per_dim + k, :] = [x, y, z, mass]

    for i in range(n_points):
        x = SCALE_MPC * random()
        y = SCALE_MPC * random()
        z = SCALE_MPC * (random() ** DISTRIBUTION_BIAS)
        if DISTRIBUTION_BIAS > 1.0:
            mass = z / SCALE_MPC
        else:
            mass = 1.0
        data_random[i, :] = [x, y, z, mass]

    for i, line in enumerate(lines[0:]):
        line = line.split(' ')
        x = SCALE_MPC * float(line[0])
        y = SCALE_MPC * float(line[1])
        z = SCALE_MPC * (random() ** DISTRIBUTION_BIAS)
        if DISTRIBUTION_BIAS > 1.0:
            mass = z / SCALE_MPC
        else:
            mass = 1.0
        data_poisson[i, :] = [x, y, z, mass]

data = data_regular
print('Min/Max X: ' + str(np.min(data[:,0])) + ' ' + str(np.max(data[:,0])))
print('Min/Max Y: ' + str(np.min(data[:,1])) + ' ' + str(np.max(data[:,1])))
print('Min/Max Z: ' + str(np.min(data[:,2])) + ' ' + str(np.max(data[:,2])))
print('Min/Max/Avg/Med Mass: ' + str(np.min(data[:,3])) + ' ' + str(np.max(data[:,3])) + ' ' + str(np.mean(data[:,3])) + ' ' + str(np.median(data[:,3])))
print('Sample record: ' + str(data[0,:]))
print('Number of records: ' + str(n_points_per_dim * n_points_per_dim * n_points_per_dim))

output_file_name = "regular_" + str(n_points_per_dim * n_points_per_dim * n_points_per_dim) + "_3d_bias=" + str(DISTRIBUTION_BIAS)
metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(n_points_per_dim * n_points_per_dim * n_points_per_dim) + "\n")
metadata_file.write("Min X = " + str(np.min(data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(data[:,3])) + "\n")
metadata_file.close()
data = data.tofile(output_file_name + '.bin')

data = data_random
print('Min/Max X: ' + str(np.min(data[:,0])) + ' ' + str(np.max(data[:,0])))
print('Min/Max Y: ' + str(np.min(data[:,1])) + ' ' + str(np.max(data[:,1])))
print('Min/Max Z: ' + str(np.min(data[:,2])) + ' ' + str(np.max(data[:,2])))
print('Min/Max/Avg/Med Mass: ' + str(np.min(data[:,3])) + ' ' + str(np.max(data[:,3])) + ' ' + str(np.mean(data[:,3])) + ' ' + str(np.median(data[:,3])))
print('Sample record: ' + str(data[0,:]))
print('Number of records: ' + str(n_points))

output_file_name = "random_" + str(n_points) + "_3d_bias=" + str(DISTRIBUTION_BIAS)
metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(n_points) + "\n")
metadata_file.write("Min X = " + str(np.min(data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(data[:,3])) + "\n")
metadata_file.close()
data = data.tofile(output_file_name + '.bin')

data = data_poisson
print('Min/Max X: ' + str(np.min(data[:,0])) + ' ' + str(np.max(data[:,0])))
print('Min/Max Y: ' + str(np.min(data[:,1])) + ' ' + str(np.max(data[:,1])))
print('Min/Max Z: ' + str(np.min(data[:,2])) + ' ' + str(np.max(data[:,2])))
print('Min/Max/Avg/Med Mass: ' + str(np.min(data[:,3])) + ' ' + str(np.max(data[:,3])) + ' ' + str(np.mean(data[:,3])) + ' ' + str(np.median(data[:,3])))
print('Sample record: ' + str(data[0,:]))
print('Number of records: ' + str(n_points))

output_file_name = fileName + "_3d_bias=" + str(DISTRIBUTION_BIAS)
metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(n_points) + "\n")
metadata_file.write("Min X = " + str(np.min(data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(data[:,3])) + "\n")
metadata_file.close()
data = data.tofile(output_file_name + '.bin')
