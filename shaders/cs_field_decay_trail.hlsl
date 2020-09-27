RWTexture3D<half4> tex_trace: register(u0);

cbuffer ConfigBuffer : register(b0)
{
    float sense_spread; // unused
    float sense_distance; // unused
    float turn_angle; // unused
    float move_distance; // unused
    float deposit_value; // unused
    float decay_factor;
    float center_attraction; // unused
    int world_width;
    int world_height;
    int world_depth;
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

[numthreads(8,8,8)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint3 p = dispatchThreadId.xyz;

    // Average trace values in a 3x3x3 neighborhood
    // Apply distance-based weighting to prevent overestimation along diagonals
    float v = 0.0;
    float w = 0.0;
    int half_window_size = 1;
    for (int dx = -half_window_size; dx <= half_window_size; dx++) {
        for (int dy = -half_window_size; dy <= half_window_size; dy++) {
            for (int dz = -half_window_size; dz <= half_window_size; dz++) {
                // float weight = 1.0 / sqrt(float(1 + abs(dx) + abs(dy) + abs(dz)));
                float weight = (all(int3(dx,dy,dz)) == 0)? 1.0 : 1.0 / sqrt(float(abs(dx) + abs(dy) + abs(dz)));
                int3 txcoord = int3(p) + int3(dx, dy, dz);
                txcoord.x = txcoord.x % world_width;
                txcoord.y = txcoord.y % world_height;
                txcoord.z = txcoord.z % world_depth;

                txcoord.x = min(max(txcoord.x, 0), world_width - 1);
                txcoord.y = min(max(txcoord.y, 0), world_height - 1);
                txcoord.z = min(max(txcoord.z, 0), world_depth - 1);

                v += weight * tex_trace[txcoord];
                w += weight;
            }
        }
    }
    v /= w;

    // Decay the deposit by a constant factor
    v *= decay_factor;
    tex_trace[p] = v;


    RNG rng;
    rng.set_seed(
        rng.wang_hash(uint(100.0*v)),
        rng.wang_hash(uint(p.x*p.y*p.z))
    );

    // Decay the trace a little
    // tex_trace[p] *= 0.99;
    // tex_trace[p] *= 0.985 + 0.01 * rng.random_float();
}