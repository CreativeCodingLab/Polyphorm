import numpy as np
import math as math
import random

fileName = '64-clean-JT-3Dskan_nobottom'
scale_multiplier = 20.0
subsampling_factor = 5.0

valid_pt_number = 0
with open(fileName + '.obj') as f:
    lines = f.readlines()
    data = np.zeros((len(lines)-1, 4), dtype=np.float32)
    for i, line in enumerate(lines):
        if not (line[0] == 'v' and line[1] == ' '):
            continue # not a valid point
        if random.uniform(0.0, 1.0) > 1.0/subsampling_factor:
            continue # randomly throw out vertices
        line = line.split(' ')
        x = scale_multiplier * float(line[1])
        y = scale_multiplier * float(line[3])
        z = -scale_multiplier * float(line[2])
        # weight = float(line[4]) + float(line[5]) + float(line[6])
        weight = 1.0
        data[valid_pt_number, :] = [x, y, z, weight]
        valid_pt_number += 1

if (valid_pt_number == 0):
    print('No points found')
    exit()

filtered_data = np.zeros((valid_pt_number, 4), dtype=np.float32)
filtered_data = data[0:valid_pt_number, :]

print('Min/Max X: ' + str(np.min(filtered_data[:,0])) + ' ' + str(np.max(filtered_data[:,0])))
print('Min/Max Y: ' + str(np.min(filtered_data[:,1])) + ' ' + str(np.max(filtered_data[:,1])))
print('Min/Max Z: ' + str(np.min(filtered_data[:,2])) + ' ' + str(np.max(filtered_data[:,2])))
print('Min/Max/Avg/Med weight: ' + str(np.min(filtered_data[:,3])) + ' ' + str(np.max(filtered_data[:,3])) + ' ' + str(np.mean(filtered_data[:,3])) + ' ' + str(np.median(filtered_data[:,3])))
print('Sample record: ' + str(filtered_data[0,:]))
print('Number of records: ' + str(valid_pt_number))

output_file_name = fileName + '_n=' + str(valid_pt_number)

metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(valid_pt_number) + "\n")
metadata_file.write("Min X = " + str(np.min(filtered_data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(filtered_data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(filtered_data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(filtered_data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(filtered_data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(filtered_data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(filtered_data[:,3])) + "\n")
metadata_file.close()

filtered_data = filtered_data.tofile(output_file_name + '.bin')
