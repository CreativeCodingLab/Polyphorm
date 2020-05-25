RWTexture3D<half> tex_density: register(u0);

RWStructuredBuffer<uint> histogram: register(u1);
RWStructuredBuffer<float> particles_x: register(u2);
RWStructuredBuffer<float> particles_y: register(u3);
RWStructuredBuffer<float> particles_z: register(u4);
RWStructuredBuffer<float> particles_weights: register(u5);
RWStructuredBuffer<float> halos_densities: register(u6);

cbuffer ConfigBuffer : register(b0)
{
    int n_data_points;
    int n_histo_bins;
    float histogram_base;
    int sample_randomly;
    float world_width;
    float world_height;
    float world_depth;
}

struct RNG {
    #define BAD_W 0x464fffffU
    #define BAD_Z 0x9068ffffU
    uint m_w;
	uint m_z;

    void set_seed(uint seed1, uint seed2) {
		m_w = seed1;
		m_z = seed2;
		if (m_w == 0U || m_w == BAD_W) ++m_w;
		if (m_w == 0U || m_z == BAD_Z) ++m_z;
	}

    void get_seed(out uint seed1, out uint seed2) {
        seed1 = m_w;
        seed2 = m_z;
    }

    uint random_uint() {
		m_z = 36969U * (m_z & 65535U) + (m_z >> 16U);
		m_w = 18000U * (m_w & 65535U) + (m_w >> 16U);
		return uint((m_z << 16U) + m_w);
	}

    float random_float() {
		return float(random_uint()) / float(0xFFFFFFFFU);
	}

    uint wang_hash(uint seed) {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }
};

[numthreads(10,10,10)]
void main(uint thread_index : SV_GroupIndex, uint3 group_id : SV_GroupID){
    uint group_idx = group_id.x + group_id.y * 10 + group_id.z * 100;
    uint idx = thread_index + 1000 * group_idx;

    if (idx >= n_data_points) // Only consider halo/galaxy locations
        return;

    float x = particles_x[idx];
    float y = particles_y[idx];
    float z = particles_z[idx];
    float mass = particles_weights[idx];

    if (sample_randomly) {
        RNG rng;
        rng.set_seed(
            rng.wang_hash(73*idx),
            rng.wang_hash(uint(x*y*z))
        );
        x = rng.random_float() * world_width;
        y = rng.random_float() * world_height;
        z = rng.random_float() * world_depth;
    }

    float density = tex_density[uint3(x, y, z)];
    halos_densities[idx] = density;
    // density /= mass; // normalize by mass - for fitting
    
    uint histo_index = 0;
    if (density > 1.0e-5) {
        density = log(density) / log(histogram_base) + 5.0;
        histo_index = 1 + min(uint(density), n_histo_bins-2);
    }

    InterlockedAdd(histogram[histo_index], 1);
}

