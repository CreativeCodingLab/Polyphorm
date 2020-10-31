RWTexture3D<half> tex_deposit: register(u0);
RWTexture3D<half4> tex_trace: register(u1);

RWStructuredBuffer<float> particles_x: register(u2);
RWStructuredBuffer<float> particles_y: register(u3);
RWStructuredBuffer<float> particles_z: register(u4);
RWStructuredBuffer<float> particles_phi: register(u5);
RWStructuredBuffer<float> particles_theta: register(u6);
RWStructuredBuffer<float> particles_weights: register(u7);

#define PROBABILISTIC_SAMPLING
#define AGENT_REROUTING
// #define FIXED_AGENT_DISTANCE_SAMPLING
// #define IGNORE_DATA

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

float3 rotate(float3 v, float3 a, float angle) {
    float3 result = cos(angle) * v + sin(angle) * (cross(a, v)) + dot(a, v) * (1.0 - cos(angle)) * a;
    return result;
}

float mod(float x, float y) {
     return x - y * floor(x / y);
}

#define DIR_SAMPLE_POINTS 8
#define PI 3.141592
#define HALFPI (0.5 * PI)
#define TWOPI (2.0 * PI)

[numthreads(10,10,10)]
void main(uint thread_index : SV_GroupIndex, uint3 group_id : SV_GroupID){
    uint group_idx = group_id.x + group_id.y * 10 + group_id.z * 100;
    uint idx = thread_index + 1000 * group_idx;

    // Fetch current particle state
    float x = particles_x[idx];
    float y = particles_y[idx];
    float z = particles_z[idx];
    float th = particles_theta[idx];
    float ph = particles_phi[idx];
    float particle_weight = particles_weights[idx];
    bool is_data = (th < -1.0);
    if (is_data) {
        #ifndef IGNORE_DATA
        tex_deposit[uint3(x, y, z)] += 10.0 * particle_weight; // NOT ATOMIC!
        #endif
        return;
    }

    RNG rng;
    rng.set_seed(
        rng.wang_hash(73*idx),
        rng.wang_hash(uint(x*y*z))
    );

    // Get vector which points in the current particle's direction 
    float3 center_axis = float3(sin(th) * cos(ph), cos(th), sin(th) * sin(ph));
    
    // Get base vector which points away from the current particle's direction and will be used
    // to sample environment in other directions
    float xiDirectional = 0.95 + 0.1 * rng.random_float();
    float sense_theta = th - sense_spread * xiDirectional;
    float3 off_center_base_dir = float3(sin(sense_theta) * cos(ph), cos(sense_theta), sin(sense_theta) * sin(ph));

    // Probabilistic sensing
    float sense_distance_prob = sense_distance;
    #ifdef FIXED_AGENT_DISTANCE_SAMPLING
    float xi = clamp(mod(0.01 * float(idx), 1.0), 0.001, 0.999); // random for each particle, but fixed in time
    #else
    float xi = clamp(rng.random_float(), 0.001, 0.999); // random for each particle and timestep
    #endif
    float distance_scaling_factor = -0.3033 * log( (pow(xi+0.005, -0.4) - 0.9974) / 7.326 ); // using Maxwell-Boltzmann distribution
    //float distance_scaling_factor = -log(xi); // using Exponential distribution
    sense_distance_prob *= distance_scaling_factor;

    // samplingSample environment along the movement axis
    int3 p = int3(x, y, z);
    float current_deposit = tex_deposit[p];
    float3 center_sense_pos = center_axis * sense_distance_prob;
    float deposit_ahead = tex_deposit[p + int3(center_sense_pos)];

    // Stochastic MC direction sampling
    float random_angle = rng.random_float() * TWOPI - PI;
    float3 sense_offset = rotate(off_center_base_dir, center_axis, random_angle) * sense_distance_prob;
    float sense_deposit = tex_deposit[p + int3(sense_offset)];
    // float temperature = move_sense_coef;
    // float p_straight = exp(-1.0 / (deposit_ahead * temperature));
    // float p_turn = exp(-1.0 / (sense_deposit * temperature));
    // float sharpness = 0.1 + move_sense_coef * exp(-mod(0.1 * float(idx), 5.0));
    float sharpness = move_sense_coef;
    #ifdef PROBABILISTIC_SAMPLING
    float p_straight = pow(max(deposit_ahead, 0.0), sharpness);
    float p_turn = pow(max(sense_deposit, 0.0), sharpness);
    #else
    float p_straight = deposit_ahead;
    float p_turn = sense_deposit;
    #endif
    float xiDir = rng.random_float();
    if (p_straight + p_turn > 1.0e-5)
        #ifdef PROBABILISTIC_SAMPLING
        if (xiDir < p_turn / (p_turn + p_straight)) {
        #else
        if (p_turn > p_straight) {
        #endif
            float theta_turn = th - turn_angle * xiDirectional;
            float3 off_center_base_dir_turn = float3(sin(theta_turn) * cos(ph), cos(theta_turn), sin(theta_turn) * sin(ph));
            float3 new_direction = rotate(off_center_base_dir_turn, center_axis, random_angle);
            ph = atan2(new_direction.z, new_direction.x);
            th = acos(new_direction.y / length(new_direction));
        }

    // // Experimental direction sampling 2
    // float bias = move_sense_coef;
    // float total_prob = pow(deposit_ahead, bias);
    // for (int i = 0; i < DIR_SAMPLE_POINTS; ++i) {

    //     float random_angle = rng.random_float() * TWOPI - PI;
    //     float3 sense_offset = rotate(off_center_base_dir, center_axis, random_angle) * sense_distance_prob;
    //     float sense_deposit = tex_deposit[p + int3(sense_offset)];

    //     float current_dir_prob = pow(sense_deposit, bias);
    //     total_prob += current_dir_prob;
    //     if (total_prob < 1.0e-5)
    //         continue;

    //     float xiDir = rng.random_float();
    //     if (xiDir < current_dir_prob / total_prob) {
    //         float theta_turn = th - turn_angle;
    //         float3 off_center_base_dir_turn = float3(sin(theta_turn) * cos(ph), cos(theta_turn), sin(theta_turn) * sin(ph));
    //         float3 new_direction = rotate(off_center_base_dir_turn, center_axis, random_angle);
    //         ph = atan2(new_direction.z, new_direction.x);
    //         th = acos(new_direction.y / length(new_direction));
    //     }
    // }

    // // Naive 3D sampling
    // // Sample environment away from the center axis and store max values
    // float max_value = deposit_ahead;
    // int max_value_count = 1;
    // int max_values[DIR_SAMPLE_POINTS + 1];
    // max_values[0] = 0;
    // float start_angle = rng.random_float() * PI - HALFPI;
    // for (int i = 1; i <= DIR_SAMPLE_POINTS; ++i) {
    //     float angle = start_angle + i * (TWOPI / float(DIR_SAMPLE_POINTS));
    //     float3 sense_offset = rotate(off_center_base_dir, center_axis, angle) * sense_distance_prob;
    //     float sense_deposit = tex_deposit[int3(sense_offset) + p];
    //     if (sense_deposit > max_value) {
    //         max_value_count = 1;
    //         max_value = sense_deposit;
    //         max_values[0] = i;
    //     } else if (sense_deposit == max_value) {
    //         max_values[max_value_count++] = i;
    //     }
    // }
    // // Pick direction with max value sampled
    // uint random_max_value_direction = rng.random_uint() % max_value_count;
    // int dir_index = max_values[random_max_value_direction];
    // float theta_turn = th - turn_angle;
    // float3 off_center_base_dir_turn = float3(sin(theta_turn) * cos(ph), cos(theta_turn), sin(theta_turn) * sin(ph));
    // if (dir_index > 0) {
    //     float3 best_direction = rotate(off_center_base_dir_turn, center_axis, start_angle + dir_index * (TWOPI / float(DIR_SAMPLE_POINTS)));
    //     ph = atan2(best_direction.z, best_direction.x);
    //     th = acos(best_direction.y / length(best_direction));
    // }

    // Compute rotation applied by force pointing to the center of environment.
    if (center_attraction > 0.001) { // this branch is practically free because 'center_attraction' is a uniform
        float3 to_center = float3(world_width / 2.0 - x, world_height / 2.0 - y, world_depth / 2.0 - z);
        float d_center = length(to_center);
        float d_c_turn = clamp((d_center - 50.0) / 150.0, 0.0, 1.0) * center_attraction;
        float3 dir = float3(sin(th) * cos(ph), cos(th), sin(th) * sin(ph));
        float3 center_dir = normalize(to_center);
        float3 center_angle = acos(dot(dir, center_dir));
        float st = 0.1 * d_c_turn;
        dir = sin((1 - st) * center_angle) / sin(center_angle) * dir + sin(st * center_angle) / sin(center_angle) * center_dir;
        if (length(dir) > 0.0 && (dir.z != 0.0 || dir.x != 0.0)){
            th = acos(dir.y / length(dir));
            ph = atan2(dir.z, dir.x);
        }
    }

    // Make a step
    float3 dp = float3(sin(th) * cos(ph), cos(th), sin(th) * sin(ph)) * move_distance * (0.1 + 0.9 * distance_scaling_factor);
    x += dp.x;
    y += dp.y;
    z += dp.z;

    // Keep the particle inside environment
    x = mod(x, world_width);
    y = mod(y, world_height);
    z = mod(z, world_depth);

    // Check if the particle is inactive and needs to be reset
    const float w_f = 0.9;
    const float n_agents_M = float(n_agents) / 1.0e6;
    // const float thr_f = 0.25 * n_agents_M * deposit_value + 0.5e-3 * n_agents_M;
    const float thr_f = 0.05 * n_agents_M * deposit_value + 0.1e-3 * n_agents_M;
    current_deposit = tex_deposit[uint3(x, y, z)];
    particle_weight = w_f * particle_weight + (1.0-w_f) * current_deposit;
    #ifdef AGENT_REROUTING
    if (particle_weight < thr_f) {
        x = rng.random_float() * world_width;
        y = rng.random_float() * world_height;
        z = rng.random_float() * world_depth;
        // Experimental !!!
        // int data_index = int(rng.random_float() * float(n_data_points));
        // float world_max_dim = max(max(world_width, world_height), world_depth);
        // x = particles_x[data_index] + (rng.random_float()-0.5) * 0.03 * world_max_dim;
        // y = particles_y[data_index] + (rng.random_float()-0.5) * 0.03 * world_max_dim;
        // z = particles_z[data_index] + (rng.random_float()-0.5) * 0.03 * world_max_dim;
        particle_weight = deposit_value;
    }
    #endif

    // Update particle state
    particles_x[idx] = x;
    particles_y[idx] = y;
    particles_z[idx] = z;
    particles_theta[idx] = th;
    particles_phi[idx] = ph;
    particles_weights[idx] = particle_weight;

    tex_deposit[uint3(x, y, z)] += deposit_value; // NOT ATOMIC!

    // tex_trace[uint3(x, y, z)] += (1.0 / normalization_factor) * distance_scaling_factor; // NOT ATOMIC!
    tex_trace[uint3(x, y, z)] += float4((1.0 / normalization_factor) * distance_scaling_factor, abs(center_axis.x), abs(center_axis.y), abs(center_axis.z)); // NOT ATOMIC!
}

