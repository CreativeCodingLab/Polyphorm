#define PT_GROUP_SIZE_X 10
#define PT_GROUP_SIZE_Y 10
#define PI 3.141592
#define PI2 (2.0*PI)
#define RAY_EPSILON 1.e-4
#define INTENSITY_EPSILON 1.e-4

// Control flags
#define TEMPORAL_ACCUMULATION
#define RUSSIAN_ROULETTE
// #define RAYMARCH_ORDER1

RWTexture2D<float4> tex_accumulator: register(u0);
Texture3D tex_trace : register(t1);
SamplerState tex_trace_sampler : register(s1);
Texture2D tex_false_color : register(t2);
SamplerState tex_false_color_sampler : register(s2);

cbuffer ConfigBuffer : register(b4)
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float4x4 model_matrix;
	int texcoord_map;
	float trim_x_min;
    float trim_x_max;
    float trim_y_min;
    float trim_y_max;
    float trim_z_min;
    float trim_z_max;
    float trim_density;
    float grid_x;
    float grid_y;
    float grid_z;
    float screen_width;
    float screen_height;
	float sample_weight;
	float optical_thickness;
	float highlight_density;
	float galaxy_weight;
	float histogram_base;
    float overdensity_threshold_low;
    float overdensity_threshold_high;
    float camera_x;
    float camera_y;
    float camera_z;
    int pt_iteration;
    float sigma_s;
    float sigma_a;
    float sigma_e;
    float trace_max;
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

float3 uniform_unit_sphere(RNG rng) {
    float azimuth = rng.random_float() * PI2;
	float polar = acos(2.0 * rng.random_float() - 1.0);
	float r = pow(rng.random_float(), 1.0/3.0);
	
	float3 result = float3(
        r * cos(azimuth) * sin(polar),
        r * cos(polar),
        r * sin(azimuth) * sin(polar)
    );
	return result;
}

float ray_sphere_intersection(float3 rp, float3 rd, float3 p, float r) {
    float3 os = rp - p;
    float a = dot(rd, rd);
    float b = 2.0 * dot(os, rd);
    float c = dot(os, os) - r * r;
    float discriminant = b * b - 4 * a * c;
        
    if(discriminant > 0) {
        float t = (-b - sqrt(discriminant)) / (2.0 * a);
        if(t > 0.0001) {
            return t;
        }
        t = (-b + sqrt(discriminant)) / (2.0 * a);
        if(t > 0.0001) {
            return t;
        }
    }
    return -1;
};

float2 ray_AABB_intersection(float3 rp, float3 rd, float3 c_lo, float3 c_hi) {
    float t[8];
    t[0] = (c_lo.x - rp.x) / rd.x;
    t[1] = (c_hi.x - rp.x) / rd.x;
    t[2] = (c_lo.y - rp.y) / rd.y;
    t[3] = (c_hi.y - rp.y) / rd.y;
    t[4] = (c_lo.z - rp.z) / rd.z;
    t[5] = (c_hi.z - rp.z) / rd.z;
    t[6] = max(max(min(t[0], t[1]), min(t[2], t[3])), min(t[4], t[5]));
    t[7] = min(min(max(t[0], t[1]), max(t[2], t[3])), max(t[4], t[5]));
    return (t[7] < 0 || t[6] >= t[7]) ? float2(-1.0, -1.0) : float2(t[6], t[7]);
}

float3 coord_normalized_to_texture(float3 coord, float3 c_lo, float3 c_hi, float3 size) {
    float3 coord_rel = (coord - c_lo) / (c_hi - c_lo);
    coord_rel.y = 1.0-coord_rel.y;
    coord_rel.z = 1.0-coord_rel.z;
    return coord_rel * size;
}

float remap(float val, float slope) {
	return 1.0 - exp(-slope * val);
}
float3 tonemap(float3 L, float exposure) {
	return float3(1.0, 1.0, 1.0) - exp(-exposure * L);
}

float trace_to_rho(float trace) {
    float rho = max(trace - trim_density, 0.0) * sample_weight;
    return rho / 5.0; // !!!
}

float get_rho(float3 rp) {
    float trace = tex_trace.SampleLevel(tex_trace_sampler, rp / float3(grid_x, grid_y, grid_z), 0).r;
    return trace_to_rho(trace);
}

float3 get_emitted_L(float rho) {
    return rho * tex_false_color.SampleLevel(tex_false_color_sampler, float2(remap(rho, 1.0), 0.5), 0).rgb;
}

float delta_step(float rho_max_inv, float xi) {
	return -log(max(xi, 0.001)) * rho_max_inv;
}

float3 delta_tracking(float3 rp, float3 rd, float t_max, float rho_max_inv, inout RNG rng, inout float event_rho) {
	float sigma_max_inv = rho_max_inv / (sigma_a + sigma_s);
    float t = 0.0;
	do {
		t += delta_step(sigma_max_inv, rng.random_float());
		event_rho = get_rho(rp + t * rd);
	} while (t <= t_max && event_rho * rho_max_inv < rng.random_float());

	return rp + t * rd;
}

float3 get_incident_L(float3 rp, float3 rd, float rho_max_inv, float3 c_low, float3 c_high, uint nBounces, inout RNG rng) {

    float3 L = float3(0.0, 0.0, 0.0);
    float throughput = 1.0;
    float albedo = sigma_s / (sigma_a + sigma_s);
    for (int n = 0; n < nBounces; ++n) {
        // Sample collision distance
        float event_rho = 0.0;
        float2 t = ray_AABB_intersection(rp, rd, c_low, c_high);
        float3 rp_new = delta_tracking(rp, rd, t.y, rho_max_inv, rng, event_rho);

        // Get locally emitted light
        float3 emission = get_emitted_L(event_rho);
        L += throughput * event_rho * sigma_e * emission;

        // Adjust the path throughput (RR or modulate)
        #ifdef RUSSIAN_ROULETTE
        if (rng.random_float() > albedo)
            return L;
        #else
        throughput *= albedo;
        #endif

        // Sample new direction and continue the walk
        rd = uniform_unit_sphere(rng);
    }

    return L;
}

[numthreads(PT_GROUP_SIZE_X, PT_GROUP_SIZE_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint2 pixel_xy = dispatchThreadId.xy;
    uint idx = threadIDInGroup + PT_GROUP_SIZE_X * PT_GROUP_SIZE_Y * groupID;

    // If PT has been reset, zero-out the accumulation buffer
    if (pt_iteration == 0)
        tex_accumulator[pixel_xy] = float4(0.0, 0.0, 0.0, 0.0);

    // Initialize RNG with a unique seed for each iteration
    RNG rng;
    rng.set_seed(
        rng.wang_hash(73 * idx + 1),
        rng.wang_hash(pixel_xy.x * pixel_xy.y * pt_iteration + 1)
    );

    // Compute x and y ray directions in "neutral" camera position.
    float aspect_ratio = float(screen_width) / float(screen_height);
    float rx = float(pixel_xy.x + rng.random_float()) / float(screen_width) * 2.0 - 1.0;
    float ry = float(pixel_xy.y + rng.random_float()) / float(screen_height) * 2.0 - 1.0;
    ry /= aspect_ratio;

    // Initialize ray origin and direction
    float screen_distance = 4.5;
    float3 camera_pos = float3(camera_x, camera_y, camera_z);
    float3 camZ = normalize(-camera_pos);
    float3 camY = float3(0,0,1);
	float3 camX = normalize(cross(camZ, camY));
	camY = normalize(cross(camX, camZ));
    float3 screen_pos =
        camera_pos
        + rx * camX
        + ry * camY
        + screen_distance * camZ;
    float3 rp = camera_pos;
    float3 rd = normalize(screen_pos - rp);

    // Get intersection of the ray with the volume AABB
    float3 path_L = float3(0.0, 0.0, 0.0);
    float2 t = 0.0;
    float3 diagonal_AABB = float3(1.0, grid_y / grid_x, grid_z / grid_x);
    float3 c_low = -0.5 * diagonal_AABB;
    float3 c_high = 0.5 * diagonal_AABB;
    t = ray_AABB_intersection(rp, rd, c_low, c_high);
    if (t.y >= 0) {
        t.x += RAY_EPSILON;
        t.y -= RAY_EPSILON;

        // Integrate along the ray segment that intersects the AABB
        float3 r0 = coord_normalized_to_texture(rp + t.x * rd, c_low, c_high, float3(grid_x, grid_y, grid_z));
        float3 r1 = coord_normalized_to_texture(rp + t.y * rd, c_low, c_high, float3(grid_x, grid_y, grid_z));
        rp = r0;
        rd = r1 - r0;
        uint nBounces = uint(100.0 * optical_thickness);
        float rho_max_inv = 1.0 / trace_to_rho(trace_max);

        #ifdef RAYMARCH_ORDER1
        int iSteps = int(length(rd));
        float3 dd = rd / float(iSteps);
        rd = normalize(rd);
        float tau = 0.0;
        rp += (rng.random_float() - 0.5) * dd;
        float rho0 = get_rho(rp), rho1;
        uint subsampling = min(nBounces, 1);
        for (int i = 0; i < iSteps; ++i) {
            rp += dd;
            rho1 = get_rho(rp);
            float rho = 0.5 * (rho0 + rho1);

            tau += rho;
            float transmittance = exp(-(sigma_a + sigma_s) * tau);

            float3 emission = sigma_e * get_emitted_L(rho);
            float3 scattering = float3(0.0, 0.0, 0.0);
            if (sigma_s > INTENSITY_EPSILON && rng.random_float() < 1.0 / float(subsampling)) {
                float3 rd_new = uniform_unit_sphere(rng);
                scattering = sigma_s * get_incident_L(rp - rng.random_float() * dd, rd_new, rho_max_inv, c_low, c_high, nBounces, rng);
            }

            path_L += transmittance * (emission + scattering) * rho;
            rho0 = rho1;
        }
        #else
        rd = normalize(rd);
        path_L = get_incident_L(rp, rd, rho_max_inv, c_low, c_high, nBounces, rng);
        #endif
    }

    // Write out results for the current iteration
    #ifdef TEMPORAL_ACCUMULATION
    float4 current_value = tex_accumulator[pixel_xy];
    tex_accumulator[pixel_xy] =
        current_value * float(pt_iteration) / float(pt_iteration + 1)
        + float4(tonemap(path_L, galaxy_weight), 1.0) / float(pt_iteration + 1);
    #else
    tex_accumulator[pixel_xy] = float4(tonemap(path_L, galaxy_weight), 1.0);
    #endif
}