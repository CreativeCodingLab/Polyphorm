## needs the freeimage package installed to load/save high dynamic range images
## read more here: https://imageio.readthedocs.io/en/latest/formats.html

import sys
import imageio as iio

for arg in sys.argv[1:]:
    hdr_image = iio.imread(arg + '.hdr', format='HDR-FI')
    iio.imwrite(arg + '.pfm', hdr_image, format='PFM-FI')