#ID DescID Mvir Vmax Vrms Rvir Rs Np X Y Z VX VY VZ JX JY JZ Spin rs_klypin Mvir_all M200b M200c M500c M2500c Xoff Voff spin_bullock b_to_a c_to_a A[x] A[y] A[z] b_to_a(500c) c_to_a(500c) A[x](500c) A[y](500c) A[z](500c) T/|U| M_pe_Behroozi M_pe_Diemer Halfmass_Radius
#a = 1.000000
#Om = 0.300000; Ol = 0.700000; h = 0.700000
#FOF linking length: 0.280000
#Unbound Threshold: 0.500000; FOF Refinement Threshold: 0.700000
#Particle mass: 1.03289e+10 Msun/h
#Box size: 512.000000 Mpc/h
#Force resolution assumed: 0.009 Mpc/h
#Units: Masses in Msun / h
#Units: Positions in Mpc / h (comoving)
#Units: Velocities in km / s (physical, peculiar)
#Units: Halo Distances, Lengths, and Radii in kpc / h (comoving)
#Units: Angular Momenta in (Msun/h) * (Mpc/h) * km/s (physical)
#Units: Spins are dimensionless
#Np is an internal debugging quantity.
#Rockst

import numpy as np

# config
dirName = 'rockstar_mnv0.60000_om0.30000_As2.1000'
fileName = 'out_66'
mass_threshold = 0.0
roi = 256.0

# convert
nonzero_halos = 0
with open(dirName + '/' + fileName + '.list') as f:
    lines = f.readlines()
    data = np.zeros((len(lines)-1, 4), dtype=np.float32)
    for i, line in enumerate(lines[16:]):
        line = line.split(' ')
        x = float(line[8])
        y = float(line[9])
        z = float(line[10])
        mass = float(line[2]) / 1.0e12

        if ((mass > mass_threshold) & (x < roi) & (y < roi) & (z < roi)):
            data[nonzero_halos, :] = [x, y, z, mass]
            nonzero_halos += 1

if (nonzero_halos == 0):
    print('No halos in selected ROI')
    exit()
n_records = nonzero_halos
filtered_data = np.zeros((n_records, 4), dtype=np.float32)
filtered_data = data[0:n_records, :]

print('Min/Max X: ' + str(np.min(filtered_data[:,0])) + ' ' + str(np.max(filtered_data[:,0])))
print('Min/Max Y: ' + str(np.min(filtered_data[:,1])) + ' ' + str(np.max(filtered_data[:,1])))
print('Min/Max Z: ' + str(np.min(filtered_data[:,2])) + ' ' + str(np.max(filtered_data[:,2])))
print('Min/Max/Avg/Med Mvir: ' + str(np.min(filtered_data[:,3])) + ' ' + str(np.max(filtered_data[:,3])) + ' ' + str(np.mean(filtered_data[:,3])) + ' ' + str(np.median(filtered_data[:,3])))
print('Sample record: ' + str(filtered_data[0,:]))
print('Number of records: ' + str(n_records))

output_file_name = dirName + '_' + fileName + 't=' + str(mass_threshold) + 'roi=' + str(roi)

metadata_file = open(output_file_name + "_metadata.txt", "w")
metadata_file.write("Number of points = " + str(n_records) + "\n")
metadata_file.write("Min X = " + str(np.min(filtered_data[:,0])) + "\n")
metadata_file.write("Max X = " + str(np.max(filtered_data[:,0])) + "\n")
metadata_file.write("Min Y = " + str(np.min(filtered_data[:,1])) + "\n")
metadata_file.write("Max Y = " + str(np.max(filtered_data[:,1])) + "\n")
metadata_file.write("Min Z = " + str(np.min(filtered_data[:,2])) + "\n")
metadata_file.write("Max Z = " + str(np.max(filtered_data[:,2])) + "\n")
metadata_file.write("Mean weight = " + str(np.mean(filtered_data[:,3])) + "\n")
metadata_file.close()

filtered_data = filtered_data.tofile(output_file_name + '.bin')