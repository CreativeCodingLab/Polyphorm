import numpy as np

file_name = 'galaxiesInSdssSlice_viz_bigger_lumdist'
file_type = '.dat'
separator = ' '
mass_threshold = 0.0

with open(file_name + file_type) as f:
    lines = f.readlines()
    data = np.zeros((len(lines)-1, 4), dtype=np.float32)
    nonzero_halos = 0
    for i, line in enumerate(lines[1:]):
        line = line.split(separator)

        azimuth = float(line[1]) / 180.0 * np.pi
        polar = (90.0 - float(line[2])) / 180.0 * np.pi
        radius = float(line[3])

        x = radius * np.sin(polar) * np.cos(azimuth)
        y = radius * np.sin(polar) * np.sin(azimuth)
        z = radius * np.cos(polar)
        logmass = float(line[4])

        if (logmass > mass_threshold):
            data[nonzero_halos, :] = [x, y, z, 10.0 ** logmass / 1.0e12]
            nonzero_halos += 1

filtered_data = np.zeros((nonzero_halos, 4), dtype=np.float32)
filtered_data = data[0:nonzero_halos, :]

print('Min/Max X: ' + str(np.min(filtered_data[:,0])) + ' ' + str(np.max(filtered_data[:,0])))
print('Min/Max Y: ' + str(np.min(filtered_data[:,1])) + ' ' + str(np.max(filtered_data[:,1])))
print('Min/Max Z: ' + str(np.min(filtered_data[:,2])) + ' ' + str(np.max(filtered_data[:,2])))
print('Min/Max/Avg/Med Mass: ' + str(np.min(filtered_data[:,3])) + ' ' + str(np.max(filtered_data[:,3])) + ' ' + str(np.mean(filtered_data[:,3])) + ' ' + str(np.median(filtered_data[:,3])))
print('Example record: ' + str(filtered_data[0,:]))
print('Number of records: ' + str(nonzero_halos))

output_file_name = file_name + '_t=' + str(mass_threshold)

metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(nonzero_halos) + "\n")
metadata_file.write("Min X = " + str(np.min(filtered_data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(filtered_data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(filtered_data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(filtered_data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(filtered_data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(filtered_data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(filtered_data[:,3])) + "\n")
metadata_file.close()

filtered_data.tofile(output_file_name + '.bin')
