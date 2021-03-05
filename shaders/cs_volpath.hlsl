#define PT_GROUP_SIZE_X 10
#define PT_GROUP_SIZE_Y 10
#define PI 3.141592
#define PI2 6.283184
#define INV_PI4 0.079577
#define RAY_EPSILON 1.e-5
#define INTENSITY_EPSILON 1.e-4
#define NUMERICAL_EPSILON 1.e-4

// Control flags
#define TEMPORAL_ACCUMULATION
#define RUSSIAN_ROULETTE
// #define GRADIENT_GUIDING
// #define TRACE_SHARPENING

// Illumination types
// #define WHITESKY_ILLUMINATION
// #define POINT_ILLUMINATION
#define HALO_ILLUMINATION
#define TRACE_ILLUMINATION

RWTexture2D<float4> tex_accumulator: register(u0);
Texture3D tex_trace : register(t1);
SamplerState tex_trace_sampler : register(s1);
Texture3D tex_deposit : register(t2);
SamplerState tex_deposit_sampler : register(s2);
Texture2D tex_palette_trace : register(t3);
SamplerState tex_palette_trace_sampler : register(s3);
Texture2D tex_palette_data : register(t4);
SamplerState tex_palette_data_sampler : register(s4);

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
    float camera_offset_x;
    float camera_offset_y;
    float exposure;
    int n_bounces;
    float ambient_trace;
    int compressive_accumulation;
    float guiding_strength;
    float scattering_anisotropy;
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

float3 uniform_unit_sphere(inout RNG rng) {
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
    return sample_weight * (max(trace - trim_density, 0.0) + ambient_trace);
}

float get_rho(float3 rp) {
    float trace = tex_trace.SampleLevel(tex_trace_sampler, rp / float3(grid_x, grid_y, grid_z), 0).r;
    return trace_to_rho(trace);
}

float get_halo(float3 rp) {
    float halo = tex_deposit.SampleLevel(tex_deposit_sampler, rp / float3(grid_x, grid_y, grid_z), 0).r;
    return 0.01 * galaxy_weight * halo;
}

float3 get_halo_gradient(float3 rp, float dp) {
    float3 gradient;
    gradient.x = get_halo(rp + float3(dp, 0.0, 0.0)) - get_halo(rp - float3(dp, 0.0, 0.0));
    gradient.y = get_halo(rp + float3(0.0, dp, 0.0)) - get_halo(rp - float3(0.0, dp, 0.0));
    gradient.z = get_halo(rp + float3(0.0, 0.0, dp)) - get_halo(rp - float3(0.0, 0.0, dp));
    return gradient / dp;
}

float3 get_emitted_trace_L(float rho) {
    return 0.05 * tex_palette_trace.SampleLevel(tex_palette_trace_sampler, float2(remap(rho, 1.0), 0.5), 0).rgb;
}

float3 get_emitted_data_L(float rho) {
    return tex_palette_data.SampleLevel(tex_palette_data_sampler, float2(remap(rho, 1.0), 0.5), 0).rgb;
}

float get_sky_L(float3 rd) {
    #ifdef WHITESKY_ILLUMINATION
    return float3(sigma_e, sigma_e, sigma_e);
    #else
    return float3(0.0, 0.0, 0.0);
    #endif 
}

float delta_step(float sigma_max_inv, float xi) {
	return -log(max(xi, 0.001)) * sigma_max_inv;
}

float delta_tracking(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng) {
	float sigma_max_inv = rho_max_inv / (sigma_a + sigma_s);
    float t = t_min;
    float event_rho = 0.0;
	do {
		t += delta_step(sigma_max_inv, rng.random_float());
		event_rho = get_rho(rp + t * rd);
	} while (t <= t_max && rng.random_float() > event_rho * rho_max_inv);

	return t;
}

float occlusion_tracking(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, float subsampling, inout RNG rng) {
	float sigma_max_inv = rho_max_inv / (sigma_a + sigma_s);
    float t = t_min;
    float rho_sum = 0.0;
    int iSteps = 0;
	do {
		t += subsampling * delta_step(sigma_max_inv, rng.random_float());
		rho_sum += get_rho(rp + t * rd);
        ++iSteps;
	} while (t <= t_max);
    rho_sum /= float(iSteps);
    float transmittance = exp(-(sigma_s + sigma_a) * rho_sum * (t_max - t_min));

	return transmittance;
}

void generate_basis(float3 dir, out float3 v1, out float3 v2)
{
	float inv_norm = 1.0 / sqrt(dir.x*dir.x + dir.z*dir.z);
	v1 = float3(dir.z * inv_norm, 0.0, -dir.x * inv_norm);
	v2 = cross(dir, v1);
}

float3 sample_HG(float3 v, float g, inout RNG rng)
{
	float xi = rng.random_float();
	float cos_theta;
	if (abs(g) > 1.e-3)	{
        float sqr_term = (1.0 - g*g) / (1.0 - g + 2.0*g*xi);
		cos_theta = (1.0 + g*g - sqr_term*sqr_term) / (2.0 * abs(g));
	} else {
		cos_theta = 1.0 - 2.0*xi;
	}

    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta*cos_theta));
	float phi = PI2 * rng.random_float();
    float3 v1, v2;
    generate_basis(v, v1, v2);
    return sin_theta * cos(phi) * v1 +
           sin_theta * sin(phi) * v2 +
           cos_theta * v;
}

float pdf_HG(float g, float cos_angle) {
    float denominator_cubicrt = sqrt(max(1.0 + g*g - 2.0*g*cos_angle, NUMERICAL_EPSILON));
    return (1.0 - g*g) / (denominator_cubicrt * denominator_cubicrt * denominator_cubicrt);
}

float3 get_incident_L(float3 rp, float3 rd, float3 c_low, float3 c_high, int nBounces, int nRRStartOrder, inout RNG rng) {
    float3 L = float3(0.0, 0.0, 0.0);
    float throughput = 1.0;
    float albedo = sigma_s / (sigma_a + sigma_s);
    float rho_max_inv = 1.0 / trace_to_rho(trace_max);

    #ifdef POINT_ILLUMINATION
    float3 trim_min = float3(trim_x_min, trim_y_min, trim_z_min);
    float3 trim_max = float3(trim_x_max, trim_y_max, trim_z_max);
    float3 l_rel_pos = 0.5 * (trim_min + trim_max);
    float3 lp = c_low + l_rel_pos * c_high;
    #endif

    for (int n = 0; n < nBounces; ++n) {

        // Sample collision distance
        float2 t = ray_AABB_intersection(rp, rd, c_low, c_high);
        float t_event = delta_tracking(rp, rd, 0.0, t.y, rho_max_inv, rng);
        if (t_event >= t.y)
            return L + throughput * get_sky_L(rd);
        rp += t_event * rd;
        float rho_event = get_rho(rp);

        // Get emitted light
        float3 emission = float3(0.0, 0.0, 0.0);
        #ifdef TRACE_ILLUMINATION
        emission += get_emitted_trace_L(rho_event);
        #endif
        #ifdef HALO_ILLUMINATION
        float halo_event = get_halo(rp);
        emission += get_emitted_data_L(halo_event);
        #endif
        #ifdef POINT_ILLUMINATION
        float3 ld = lp - rp;
        float l_distance = length(ld);
        ld = normalize(ld);
        // Modified delta-tracking transmittance estimator
        float transmittance = occlusion_tracking(rp, ld, 0.0, l_distance, rho_max_inv, 10.0, rng);
        emission += 100.0 * galaxy_weight * transmittance / max(l_distance * l_distance, 1.0);
        #endif
        L += throughput * rho_event * sigma_e * emission;

        // Adjust the path throughput (RR or modulate)
        #ifdef RUSSIAN_ROULETTE
        if (n >= nRRStartOrder && rng.random_float() > albedo)
            return L;
        else
            throughput *= albedo;
        #else
        throughput *= albedo;
        #endif

        // Sample new direction and continue the walk
        #ifdef GRADIENT_GUIDING
        float3 grad = get_halo_gradient(rp, 1.0);
        if (guiding_strength > INTENSITY_EPSILON && any(abs(grad) > float3(INTENSITY_EPSILON, INTENSITY_EPSILON, INTENSITY_EPSILON))) { 
            float g = min(guiding_strength * length(grad), scattering_anisotropy);
            float3 grad_norm = normalize(grad);
            float3 rd_new = sample_HG(grad_norm, g, rng);
            float pdf = pdf_HG(g, dot(grad_norm, rd_new));
            throughput /= pdf;
            rd = rd_new;
        } else {
            rd = uniform_unit_sphere(rng);
        }
        #else
        rd = sample_HG(rd, scattering_anisotropy, rng);
        #endif
    }
    return L;
}

[numthreads(PT_GROUP_SIZE_X, PT_GROUP_SIZE_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixel_xy = dispatchThreadId.xy;
    uint idx = threadIDInGroup + PT_GROUP_SIZE_X * PT_GROUP_SIZE_Y * groupID;

    // If PT has been reset, zero-out the accumulation buffer
    if (pt_iteration == 0)
        tex_accumulator[pixel_xy] = float4(0.0, 0.0, 0.0, 0.0);

    // Initialize RNG with a unique seed for each iteration
    RNG rng;
    rng.set_seed(
        rng.wang_hash(1 + 73 * idx),
        rng.wang_hash(1 + (pixel_xy.x + pixel_xy.y + pixel_xy.x * pixel_xy.y) * (pt_iteration + 1))
    );

    // Compute x and y ray directions in "neutral" camera position.
    float aspect_ratio = float(screen_width) / float(screen_height);
    float rx = float(pixel_xy.x + rng.random_float()) / float(screen_width) * 2.0 - 1.0;
    float ry = float(pixel_xy.y + rng.random_float()) / float(screen_height) * 2.0 - 1.0;
    ry /= aspect_ratio;

    // Initialize ray origin and direction
    const float screen_distance = 4.5;
    const float cam_offset_ratio = 0.45;
    float3 camera_pos = float3(camera_x, camera_y, camera_z);
    float3 camZ = normalize(-camera_pos);
    float3 camY = float3(0,0,1);
	float3 camX = normalize(cross(camZ, camY));
	camY = normalize(cross(camX, camZ));
    float3 screen_pos =
        camera_pos
        + (rx - cam_offset_ratio * camera_offset_x) * camX
        + (ry - cam_offset_ratio * camera_offset_y) * camY
        + screen_distance * camZ;

    // Get intersection of the ray with the volume AABB
    float3 grid_res = float3(grid_x, grid_y, grid_z);
    float3 diagonal_AABB = float3(1.0, grid_y / grid_x, grid_z / grid_x);
    float3 c_low = -0.5 * diagonal_AABB;
    float3 c_high = 0.5 * diagonal_AABB;
    float3 rp = coord_normalized_to_texture(camera_pos, c_low, c_high, grid_res);
    float3 rd = normalize(coord_normalized_to_texture(screen_pos, c_low, c_high, grid_res) - rp);
    float3 c_low_trimmed = float3(max(0.0,trim_x_min), max(0.0,trim_y_min), max(0.0,trim_z_min)) * grid_res;
    float3 c_high_trimmed = float3(min(1.0,trim_x_max), min(1.0,trim_y_max), min(1.0,trim_z_max)) * grid_res;
    float2 t = ray_AABB_intersection(rp, rd, c_low_trimmed, c_high_trimmed);

    // Integrate...
    float3 path_L = float3(0.0, 0.0, 0.0);
    if (t.y >= 0) {
        t.x += RAY_EPSILON;
        t.y -= RAY_EPSILON;

        // Integrate along the ray segment that intersects the AABB
        rp = rp + t.x * rd;
        rd = rd * (t.y - t.x);
        
        if (sigma_s < INTENSITY_EPSILON) {
        // If there's no appreciable scattering, we can just use the
        // emission-absorption model and ray-march the solution
            int iSteps = int(length(rd) / 1.71);
            float3 dd = rd / float(iSteps);
            rd = normalize(rd);
            float tau = 0.0;
            rp += (rng.random_float() - 0.5) * dd;
            float rho0 = get_rho(rp), rho1;
            for (int i = 0; i < iSteps; ++i) {
                rp += dd;
                rho1 = get_rho(rp);
                float rho = 0.5 * (rho0 + rho1);

                tau += rho;
                float transmittance = exp(-(sigma_a + sigma_s) * tau);
                float3 emission = float3(0.0, 0.0, 0.0);
                #ifdef TRACE_ILLUMINATION
                emission += get_emitted_trace_L(rho);
                #endif
                #ifdef HALO_ILLUMINATION
                float halo_event = get_halo(rp);
                emission += get_emitted_data_L(halo_event);
                #endif
                
                path_L += transmittance * rho * sigma_e * emission;
                rho0 = rho1;
            }
        } else {
        // If there's significant scattering, we need the full path-traced solution
            rd = normalize(rd);
            path_L = get_incident_L(rp, rd, c_low_trimmed, c_high_trimmed, n_bounces + 1, 2, rng);
            path_L *= PI2;
        }
    } else {
        path_L = get_sky_L(rd);
    }

    // Accumulate LDR or HDR values?
    path_L = (compressive_accumulation == 1) ? tonemap(path_L, exposure) : path_L;

    // Write out results for the current iteration
    #ifdef TEMPORAL_ACCUMULATION
    float4 current_value = tex_accumulator[pixel_xy];
    tex_accumulator[pixel_xy] =
        current_value * float(pt_iteration) / float(pt_iteration + 1)
        + float4(path_L, 1.0) / float(pt_iteration + 1);
    #else
    tex_accumulator[pixel_xy] = float4(path_L, 1.0);
    #endif
}