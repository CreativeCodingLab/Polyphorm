
RWStructuredBuffer<float> particles_x: register(u2);
RWStructuredBuffer<float> particles_y: register(u3);
RWStructuredBuffer<float> particles_z: register(u4);
RWStructuredBuffer<float> particles_phi: register(u5);
RWStructuredBuffer<float> particles_theta: register(u6);
RWStructuredBuffer<float> particles_weights: register(u7);

cbuffer ConfigBuffer : register(b0)
{
    float sense_spread;
    float sense_distance;
    float turn_angle;
    float move_distance;
    float deposit_value;
    float decay_factor;
    float center_attraction;
    int world_width;
    int world_height;
    int world_depth;
    float move_sense_coef;
    float normalization_factor;
    int n_data_points;
    int n_agents;
    int n_iteration;
};

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

void swap_agents(uint idxA, uint idxB) {
    float tmp;
    tmp = particles_x[idxA]; particles_x[idxA] = particles_x[idxB]; particles_x[idxB] = tmp;
    tmp = particles_y[idxA]; particles_y[idxA] = particles_y[idxB]; particles_y[idxB] = tmp;
    tmp = particles_z[idxA]; particles_z[idxA] = particles_z[idxB]; particles_z[idxB] = tmp;
    tmp = particles_theta[idxA]; particles_theta[idxA] = particles_theta[idxB]; particles_theta[idxB] = tmp;
    tmp = particles_phi[idxA]; particles_phi[idxA] = particles_phi[idxB]; particles_phi[idxB] = tmp;
    tmp = particles_weights[idxA]; particles_weights[idxA] = particles_weights[idxB]; particles_weights[idxB] = tmp;
}

uint Part1By2(uint x_) {
    uint x = x_ & 0x000003ff;
    x = (x ^ (x << 16)) & 0xff0000ff;
    x = (x ^ (x <<  8)) & 0x0300f00f;
    x = (x ^ (x <<  4)) & 0x030c30c3;
    x = (x ^ (x <<  2)) & 0x09249249;
    return x;
}

uint EncodeMorton3(uint x, uint y, uint z) {
    return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

[numthreads(10,10,10)]
void main(uint thread_index : SV_GroupIndex, uint3 group_id : SV_GroupID) {
    uint group_idx = group_id.x + group_id.y * 10 + group_id.z * 100;
    uint idx = thread_index + 1000 * group_idx;

    // uint idxA = 32 * idx + (n_iteration % 32);
    // if (idxA >= n_agents-1)
    //     return;
    // uint idxB = idxA + 1;
    // idxA = idxA + (n_data_points-1);
    // idxB = idxB + (n_data_points-1);

    // float vA = particles_y[idxA];
    // float vB = particles_y[idxB];
    // if (vA > vB) {
    //     swap_agents(idxA, idxB);
    // }

    RNG rng;
    rng.set_seed(
        rng.wang_hash(73*idx),
        rng.wang_hash(13*n_iteration)
    );
    uint idxA = n_data_points + rng.random_uint() % n_agents;
    uint idxB = n_data_points + rng.random_uint() % n_agents;
    float3 posA = float3(particles_x[idxA], particles_y[idxA], particles_z[idxA]);
    float3 posAN = float3(
        0.5 * (particles_x[idxA-1] + particles_x[idxA+1]),
        0.5 * (particles_y[idxA-1] + particles_y[idxA+1]),
        0.5 * (particles_z[idxA-1] + particles_z[idxA+1]));
    float3 posB = float3(particles_x[idxB], particles_y[idxB], particles_z[idxB]);
    float3 posBN = float3(
        0.5 * (particles_x[idxB-1] + particles_x[idxB+1]),
        0.5 * (particles_y[idxB-1] + particles_y[idxB+1]),
        0.5 * (particles_z[idxB-1] + particles_z[idxB+1]));
    float dA = length(posA - posAN);
    float dB = length(posB - posBN);
    float dApot = length(posA - posBN);
    float dBpot = length(posB - posAN);
    if ((dApot + dBpot) < (dA + dB)) {
        swap_agents(idxA, idxB);
    }

    // RNG rng;
    // rng.set_seed(
    //     rng.wang_hash(73*idx),
    //     rng.wang_hash(13*n_iteration)
    // );
    // uint idxA = (n_data_points - 1) + rng.random_uint() % n_agents;
    // uint idxB = (n_data_points - 1) + rng.random_uint() % n_agents;
    // // uint vA = EncodeMorton3(uint(particles_x[idxA]), uint(particles_y[idxA]), uint(particles_z[idxA]));
    // // uint vB = EncodeMorton3(uint(particles_x[idxB]), uint(particles_y[idxB]), uint(particles_z[idxB]));
    // float vA = particles_y[idxA];
    // float vB = particles_y[idxB];
    // if (idxA < idxB && vA > vB) {
    //     swap_agents(idxA, idxB);
    // }

    // uint idxA = (n_data_points - 1) + idx;
    // uint idxB = idxA + n_agents / 2;
    // float vA = particles_z[idxA];
    // float vB = particles_z[idxB];
    // if (vA > vB) {
    //     swap_agents(idxA, idxB);
    // }
}
