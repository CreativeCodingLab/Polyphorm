#define PT_GROUP_SIZE_X 10
#define PT_GROUP_SIZE_Y 10
#define PI 3.141592
#define PI2 6.283184
#define INV_PI4 0.079577
#define RAY_EPSILON 1.e-5
#define INTENSITY_EPSILON 1.e-4
#define NUMERICAL_EPSILON 1.e-4

// More accurate PI
#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif
#define     EQN_EPS     1e-9
#define	    IsZero(x)	((x) > -EQN_EPS && (x) < EQN_EPS)
#define     cbrt(x)     ((x) > 0.0 ? pow((float)(x), 1.0/3.0) : ((x) < 0.0 ? -pow((float)-(x), 1.0/3.0) : 0.0))

// Control flags
#define TEMPORAL_ACCUMULATION
#define RUSSIAN_ROULETTE
// #define GRADIENT_GUIDING

// Illumination types
#define WHITESKY_ILLUMINATION
#define POINT_LIGHT
// #define POINT_ILLUMINATION
// #define HALO_ILLUMINATION
// #define TRACE_ILLUMINATION

//#define SPHERE_DEBUG

RWTexture2D<float4> tex_accumulator: register(u0);
Texture3D tex_trace : register(t1);
SamplerState tex_trace_sampler : register(s1);
Texture3D tex_deposit : register(t2);
SamplerState tex_deposit_sampler : register(s2);
Texture2D tex_palette_trace : register(t3);
SamplerState tex_palette_trace_sampler : register(s3);
Texture2D tex_palette_data : register(t4);
SamplerState tex_palette_data_sampler : register(s4);

Texture2D tex_nature_hdri : register(t5);
SamplerState tex_nature_hdri_sampler : register(s5);

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

    /* For Slime Mold Rendering */

    float sigma1_s_r;
    float sigma1_s_g;
    float sigma1_s_b;
    float sigma1_a_r;

    float sigma1_a_g;
    float sigma1_a_b;
    float slime_ior;
    float aperture;
    
    float focus_dist;
    float light_pos;
    float sphere_pos;
    int shininess;

    float some_slider;
    int tmp1;
    int tmp2;
    int tmp3;
    

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

void swap(inout float t1, inout float t2) {
    float tmp = t1;
    t1 = t2;
    t2 = tmp;
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

float3 get_rho_gradient(float3 rp, float dp) {
    float3 gradient;
    gradient.x = get_rho(rp + float3(dp, 0.0, 0.0)) - get_rho(rp - float3(dp, 0.0, 0.0));
    gradient.y = get_rho(rp + float3(0.0, dp, 0.0)) - get_rho(rp - float3(0.0, dp, 0.0));
    gradient.z = get_rho(rp + float3(0.0, 0.0, dp)) - get_rho(rp - float3(0.0, 0.0, dp));
    return gradient / dp;
}

float get_halo(float3 rp) {
    float halo = tex_deposit.SampleLevel(tex_deposit_sampler, rp / float3(grid_x, grid_y, grid_z), 0).r;
    // return 0.01 * galaxy_weight * halo;
    return halo;
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
	float sigma_max_inv = rho_max_inv / ((sigma1_a_r + sigma1_s_r + sigma1_a_g + sigma1_s_g + sigma1_a_b + sigma1_s_b) / 3);    // this greater, steps greater
    float t = t_min;
    float event_rho = 0.0;
	do {
		t += delta_step(sigma_max_inv, rng.random_float());
		event_rho = get_rho(rp + t * rd);
	} while (t <= t_max && rng.random_float() > event_rho * rho_max_inv);

	return t;
}

float delta_tracking_in_volume(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng, inout bool hit_surface) {
	float sigma_max_inv = rho_max_inv / ((sigma1_a_r + sigma1_s_r + sigma1_a_g + sigma1_s_g + sigma1_a_b + sigma1_s_b) / 3);    // this greater, steps greater
    float t = t_min;
    float event_rho = 0.0;
    float grad = 0.0;

    if (t_min >= t_max) return t;   // t_min > t_max is not supposed to happen
	do {
		t += delta_step(sigma_max_inv, rng.random_float());

        if (get_halo(rp + t * rd) > 500) return 0;

        event_rho = get_rho(rp + t * rd);
        // grad = get_rho_gradient(rp,1.0);
        // if (length(grad) > 0.001) return t;
        // If the ray gets out of a high density volume, then stop
        if (event_rho < 0.5) {
            hit_surface = true;
            break;
        }
	} while (t <= t_max && rng.random_float() > 1 * rho_max_inv); 

	return t;
}

float delta_tracking_out_volume(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng, inout bool hit_surface) {
	float sigma_max_inv = rho_max_inv / ((sigma1_a_r + sigma1_s_r + sigma1_a_g + sigma1_s_g + sigma1_a_b + sigma1_s_b) / 3);    // this greater, steps greater
    float t = t_min;
    float event_rho = 0.0;
    float grad = 0.0;

    if (t_min >= t_max) return t;   // t_min > t_max is not supposed to happen
	do {
		t += delta_step(sigma_max_inv, rng.random_float());

        event_rho = get_rho(rp + t * rd);
        // grad = get_rho_gradient(rp,1.0);
        // if (length(grad) > 0.001) return t;
        // If the ray enters a high density volume, then stop
        if (event_rho > 0.5) {
            hit_surface = true;
            break;
        }
    } while (t <= t_max);
	return t;
}

// current_layer: 0 out-volume, 1 outer-jello, 2 in-volume
float delta_tracking_multi_layers(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng, inout int current_layer, inout bool hit_surface) {
	float sigma_max_inv = rho_max_inv / ((sigma1_a_r + sigma1_s_r + sigma1_a_g + sigma1_s_g + sigma1_a_b + sigma1_s_b) / 3);    // this greater, steps greater
    float t = t_min;
    float event_rho = 0.0;
    float grad = 0.0;

    if (t_min >= t_max) return t;   // t_min > t_max is not supposed to happen
	do {
		t += delta_step(sigma_max_inv, rng.random_float());

        // if (get_halo(rp + t * rd) > 500) return 0;

        event_rho = get_rho(rp + t * rd);

        if (current_layer == 0 && event_rho > 0.4) {
            hit_surface = true;
            current_layer = 1;
            break;
        }

        if (current_layer == 1) {
            if (event_rho > 0.5) {
                current_layer = 2;
            }
            if (event_rho < 0.4) {
                current_layer = 0;
            }
            hit_surface = true;
            break;
        }

        if (current_layer == 2 && event_rho < 0.5) {
            current_layer = 1;
            hit_surface = true;
            break;
        }

        if (current_layer == 2 && rng.random_float() < 1 * rho_max_inv) return t;
        if (current_layer == 1 && rng.random_float() < 5 * rho_max_inv) return t;

	} while (t <= t_max); 

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

// Helper for intersect_sphere()
bool solveQuadratic(float a, float b, float c, inout float x0, inout float x1) {
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) {
        x0 = -0.5 * b / a;
        x1 = -0.5 * b / a;
    }
    else {
        float q = (b > 0) ?
            -0.5 * (b + sqrt(discr)) :
            -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) {
        float tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    return true;
}

// DEBUG: Sphere Material Test
bool intersect_sphere(float3 rp, float3 rd, inout float t_intersect) {
    
    // Sphere Config for debug
    float3 center = float3(grid_x/2.0, grid_y/2.0 + sphere_pos, grid_z/2.0);
    float radius = grid_x / 10.0;

    float3 L = rp - center;
    float a = dot(rd, rd);
    float b = 2 * dot(rd, L);
    float c = dot(L, L) - radius * radius;
    float t0, t1;
    if (!solveQuadratic(a, b, c, t0, t1)) return false;
    if (t0 < 0) {
        t0 = t1;
        return false;
    }
    t_intersect = t0;
    return true;
}


// DEBUG
float3 get_sphere_normal(float3 rp) {
    float3 center = float3(grid_x/2.0, grid_y/2.0 + sphere_pos, grid_z/2.0);
    return normalize(rp - center);
}

// DEBUG
float3 reflect_sphere(float3 rp, float3 rd) {
    float3 n = get_sphere_normal(rp);
    return rd - 2 * dot(normalize(rd), n) * n;
}

// DEBUG: Sphere Material Test
float sphere_tracking_in_volume(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng, inout bool hit_surface) {
	
    float t_intersect = t_min;
    if (t_min >= t_max) return t_intersect;   // t_min > t_max is not supposed to happen

    if (intersect_sphere(rp, rd, t_intersect)) {
        hit_surface = true;
    }
    else {
        t_intersect = t_max + 0.0001;
    }


    float t = t_min;

	do {
		t += rng.random_float();
	} while (t <= t_intersect && rng.random_float() > rho_max_inv);

	return t;
}

// DEBUG: Sphere Material Test
float sphere_tracking_out_volume(float3 rp, float3 rd, float t_min, float t_max, float rho_max_inv, inout RNG rng, inout bool hit_surface) {

    float t = t_min;
    if (t_min >= t_max) return t;   // t_min > t_max is not supposed to happen

    if (intersect_sphere(rp, rd, t)) {
        hit_surface = true;
        return t;
    }
    else {
        return t_max + 0.0001;
    }
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

// Normal is defined as a density gradient
// Offset is variable
float3 get_normal(float3 rp) {
    return normalize(-get_rho_gradient(rp, 1.0));
}

// rp: surface position
// rd: light to surface
// out: surface to reflected light
float3 reflect(float3 rp, float3 rd) {
    float3 n = get_normal(rp);

    return rd - 2 * dot(normalize(rd), n) * n;
}

// rp is in to the point
float3 refract(float3 rp, float3 rd) {
    float3 n = get_normal(rp);

    float cosi = clamp(dot(rd, n), -1, 1);
    float etai = 1;
    float etat = slime_ior;

    if (cosi < 0) cosi = -cosi;
    else {
        swap(etai, etat);
        n = -n;
    }

    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? 0 : eta * rd + (eta * cosi - sqrt(k)) * n;
}

// return kr: amount of reflected light. thus 1-kr is refracted light
float fresnel(float3 rp, float3 rd) {

    // negativec of density gradient should be surface normal
    float3 n = get_normal(rp);

    float cosi = clamp(dot(rd, n), -1, 1);
    float etai = 1;
    float etat = slime_ior;

    if (cosi > 0) swap(etai, etat);

    float sint = etai / etat * sqrt(max(0, 1 - cosi * cosi));

    // Total internal reflection
    if (sint >= 1) {
        return 1;
    }
    else {
        float cost = sqrt(max(0, 1 - sint * sint));
        cosi = abs(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        return (Rs * Rs + Rp * Rp) / 2;
    }
}

int SolveQuadric(inout float3 c, inout float2 s) {
    float p, q, D;

    /* normal form: x^2 + px + q = 0 */

    p = c[ 1 ] / (2 * c[ 2 ]);
    q = c[ 0 ] / c[ 2 ];

    D = p * p - q;

    if (IsZero(D))
    {
	s[ 0 ] = - p;
	return 1;
    }
    else if (D < 0)
    {
	return 0;
    }
    else /* if (D > 0) */
    {
	float sqrt_D = sqrt(D);

	s[ 0 ] =   sqrt_D - p;
	s[ 1 ] = - sqrt_D - p;
	return 2;
    }
}

int SolveCubic(inout float4 c, inout float3 s) {
    int     i, num;
    float  sub;
    float  A, B, C;
    float  sq_A, p, q;
    float  cb_p, D;

    /* normal form: x^3 + Ax^2 + Bx + C = 0 */

    A = c[ 2 ] / c[ 3 ];
    B = c[ 1 ] / c[ 3 ];
    C = c[ 0 ] / c[ 3 ];

    /*  substitute x = y - A/3 to eliminate quadric term:
	x^3 +px + q = 0 */

    sq_A = A * A;
    p = 1.0/3 * (- 1.0/3 * sq_A + B);
    q = 1.0/2 * (2.0/27 * A * sq_A - 1.0/3 * A * B + C);

    /* use Cardano's formula */

    cb_p = p * p * p;
    D = q * q + cb_p;

    if (IsZero(D))
    {
	if (IsZero(q)) /* one triple solution */
	{
	    s[ 0 ] = 0;
	    num = 1;
	}
	else /* one single and one double solution */
	{
	    float u = cbrt(-q);
	    s[ 0 ] = 2 * u;
	    s[ 1 ] = - u;
	    num = 2;
	}
    }
    else if (D < 0) /* Casus irreducibilis: three real solutions */
    {
	float phi = 1.0/3 * acos(-q / sqrt(-cb_p));
	float t = 2 * sqrt(-p);

	s[ 0 ] =   t * cos(phi);
	s[ 1 ] = - t * cos(phi + M_PI / 3);
	s[ 2 ] = - t * cos(phi - M_PI / 3);
	num = 3;
    }
    else /* one real solution */
    {
	float sqrt_D = sqrt(D);
	float u = cbrt(sqrt_D - q);
	float v = - cbrt(sqrt_D + q);

	s[ 0 ] = u + v;
	num = 1;
    }

    /* resubstitute */

    sub = 1.0/3 * A;

    for (i = 0; i < num; ++i)
	s[ i ] -= sub;

    return num;
}


int SolveQuartic(inout float3 c1, inout float2 c2, inout float4 s) {
    float4  coeffs;
    float  z, u, v, sub;
    float  A, B, C, D;
    float  sq_A, p, q, r;
    int     i, num = 0;

    /* normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0 */

    A = c2[ 0 ] / c2[ 1 ];
    B = c1[ 2 ] / c2[ 1 ];
    C = c1[ 1 ] / c2[ 1 ];
    D = c1[ 0 ] / c2[ 1 ];

    /*  substitute x = y - A/4 to eliminate cubic term:
	x^4 + px^2 + qx + r = 0 */

    sq_A = A * A;
    p = - 3.0/8 * sq_A + B;
    q = 1.0/8 * sq_A * A - 1.0/2 * A * B + C;
    r = - 3.0/256*sq_A*sq_A + 1.0/16*sq_A*B - 1.0/4*A*C + D;

    if (IsZero(r))
    {
        /* no absolute term: y(y^3 + py + q) = 0 */

        coeffs[ 0 ] = q;
        coeffs[ 1 ] = p;
        coeffs[ 2 ] = 0;
        coeffs[ 3 ] = 1;

        float3 s3;
        s3[0] = s[0];
        s3[1] = s[1];
        s3[2] = s[2];
        num = SolveCubic(coeffs, s3);
        s[0] = s3[0];
        s[1] = s3[1];
        s[2] = s3[2];

        int n = num++;
        if (n == 0) s[0] = 0;
        if (n == 1) s[1] = 0;
        if (n == 2) s[2] = 0;
        if (n == 3) s[3] = 0;
        //s[ num++ ] = 0;
    }
    else
    {
        /* solve the resolvent cubic ... */

        coeffs[ 0 ] = 1.0/2 * r * p - 1.0/8 * q * q;
        coeffs[ 1 ] = - r;
        coeffs[ 2 ] = - 1.0/2 * p;
        coeffs[ 3 ] = 1;

        float3 s3;
        s3[0] = s[0];
        s3[1] = s[1];
        s3[2] = s[2];
        SolveCubic(coeffs, s3);
        s[0] = s3[0];
        s[1] = s3[1];
        s[2] = s3[2];

        /* ... and take the one real solution ... */

        z = s[ 0 ];

        /* ... to build two quadric equations */

        u = z * z - r;
        v = 2 * z - p;

        if (IsZero(u))
            u = 0;
        else if (u > 0)
            u = sqrt(u);
        else
            return 0;

        if (IsZero(v))
            v = 0;
        else if (v > 0)
            v = sqrt(v);
        else
            return 0;

        coeffs[ 0 ] = z - u;
        coeffs[ 1 ] = q < 0 ? -v : v;
        coeffs[ 2 ] = 1;

        float2 s2;
        float3 coeffs3;
        s2[0] = s[0];
        s2[1] = s[1];
        coeffs3[0] = coeffs[0];
        coeffs3[1] = coeffs[1];
        coeffs3[2] = coeffs[2];
        num = SolveQuadric(coeffs3, s2);
        s[0] = s2[0];
        s[1] = s2[1];

        coeffs[ 0 ]= z + u;
        if (q < 0) coeffs[1] = v;
        if (q >= 0) coeffs[1] = -v;
        //coeffs[ 1 ] = q < 0 ? v : -v;
        coeffs[ 2 ] = 1;

        int n = num;
        s2[0] = s[0 + n];
        s2[1] = s[1 + n];
        coeffs3[0] = coeffs[0];
        coeffs3[1] = coeffs[1];
        coeffs3[2] = coeffs[2];
        num += SolveQuadric(coeffs3, s2);

        if (n == 0) {
            s[0] = s2[0];
            s[1] = s2[1];
        }
        else if (n == 1) {
            s[1] = s2[0];
            s[2] = s2[1];
        }
        else if (n == 2) {
            s[2] = s2[0];
            s[3] = s2[1];
        }
    }

    /* resubstitute */

    sub = 1.0/4 * A;

    for (i = 0; i < num; ++i)
	    s[ i ] -= sub;

    return num;
}

bool box_intersection(float3 bb_min, float3 bb_max, float3 rp, float3 rd) {
    float tmin = (bb_min.x - rp.x) / rd.x; 
    float tmax = (bb_max.x - rp.x) / rd.x; 
 
    if (tmin > tmax) swap(tmin, tmax);

    float tymin = (bb_min.y - rp.y) / rd.y; 
    float tymax = (bb_max.y - rp.y) / rd.y; 
 
    if (tymin > tymax) swap(tymin, tymax); 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
 
    if (tymin > tmin) 
        tmin = tymin; 
 
    if (tymax < tmax) 
        tmax = tymax; 
 
    float tzmin = (bb_min.z - rp.z) / rd.z; 
    float tzmax = (bb_max.z - rp.z) / rd.z; 
 
    if (tzmin > tzmax) swap(tzmin, tzmax); 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
 
    if (tzmin > tmin) 
        tmin = tzmin; 
 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true; 
}

bool intersect_plane(float3 bb_min, float3 bb_max, float3 rp, float3 rd) {

    float3 center = float3(-1 * grid_x, grid_y/2.0, grid_z/2.0);

    // Translate
    rp = rp - center;

    // Angle of Torus
    float theta = PI / 2;

    float tmin = (bb_min.x - rp.x) / rd.x; 
    float tmax = (bb_max.x - rp.x) / rd.x; 
 
    if (tmin > tmax) swap(tmin, tmax);

    float tymin = (bb_min.y - rp.y) / rd.y; 
    float tymax = (bb_max.y - rp.y) / rd.y; 
 
    if (tymin > tymax) swap(tymin, tymax); 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
 
    if (tymin > tmin) 
        tmin = tymin; 
 
    if (tymax < tmax) 
        tmax = tymax; 
 
    float tzmin = (bb_min.z - rp.z) / rd.z; 
    float tzmax = (bb_max.z - rp.z) / rd.z; 
 
    if (tzmin > tzmax) swap(tzmin, tzmax); 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
 
    if (tzmin > tmin) 
        tmin = tzmin; 
 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true; 
}

bool intersect_torus(float3 rp, float3 rd) {

    float3 center = float3(1.5 * grid_x, grid_y/2.0, grid_z/2.0);
    
    // Torus Config
    float R = 300;
    float r = 50;

    float3 bb_max = R+r;
    float3 bb_min = -R-r;

    // Translate
    rp = rp - center;

    // Angle of Torus
    // PI/2 is parallel to slime mold
    float theta = PI / 1.6; 

    float3x3 rotate_mat = {cos(theta), -sin(theta), 0,
                           sin(theta), cos(theta), 0,
                           0, 0, 1};

    // Rotate eye vector and eye position
    rd = mul(rotate_mat, rd);
    rp = mul(rotate_mat, rp);

    if(!box_intersection(bb_min, bb_max, rp, rd)) return false;

    float c4 = pow((rd.x * rd.x + rd.y * rd.y + rd.z * rd.z), 2);
    float c3 = 4 * (rd.x * rd.x + rd.y * rd.y + rd.z * rd.z) * (rp.x * rd.x + rp.y * rd.y + rp.z * rd.z);
    float c2 = 2 * (rd.x * rd.x + rd.y * rd.y + rd.z * rd.z) * (rp.x * rp.x + rp.y * rp.y + rp.z * rp.z - (r * r + R * R)) 
                + 4 * pow((rp.x * rd.x + rp.y * rd.y + rp.z * rd.z), 2) + 4 * R * R * rd.y * rd.y;
    float c1 = 4 * (rp.x * rp.x + rp.y * rp.y + rp.z * rp.z - (r * r + R * R)) * (rp.x * rd.x + rp.y * rd.y + rp.z * rd.z) 
                + 8 * R * R * rp.y * rd.y;
    float c0 = pow((rp.x * rp.x + rp.y * rp.y + rp.z * rp.z - (r * r + R * R)), 2) - 4 * R * R * (r * r - rp.y * rp.y);

    float3 v3 = float3(c0, c1, c2);
    float2 v2 = float2(c3, c4);
    float4 v4 = float4(0,0,0,0);
    int n = SolveQuartic(v3, v2, v4); 

    if (n > 0 && v4[0] > 0) return true;
    else return false;
}

float3 calc_ambient_color(float3 rp, float3 rd, float bokeh, inout RNG rng) {

    // Sphere for hdri
    float3 center = float3(grid_x/2.0, grid_y/2.0, grid_z/2.0);
    float radius = 5 * grid_x;


    rp = rp - center;

    float theta = -PI / 2; 
    float3x3 rotate_mat = {1, 0, 0,
                           0, cos(theta), -sin(theta),
                           0, sin(theta), cos(theta)};

    // Rotate eye vector and eye position
    rd = mul(rotate_mat, rd);
    rp = mul(rotate_mat, rp);


    float3 L = rp;
    float a = dot(rd, rd);
    float b = 2 * dot(rd, L);
    float c = dot(L, L) - radius * radius;
    float t0, t1;
    if (!solveQuadratic(a, b, c, t0, t1)) return float3(0, 0, 0);
    t0 = t1;
    if (t0 < 0) {
        t0 = t1;
        return float3(0, 0, 0);
    }

    // point on sphere
    float3 sp = rp + rd * t0;

    // float phi = atan2(sp.z, sp.x);
    // float theta = asin(sp.y);
    // float u = 1 - (phi + M_PI) / (2 * M_PI);
    // float v = (theta + M_PI / 2) / M_PI;


    float u = 0.5f + atan2(sp.x, sp.z) / (2 * M_PI) + bokeh * 2 * (rng.random_float() - 0.5);
    float v = 0.5f + asin(sp.y / radius) / M_PI + bokeh * 2 * (rng.random_float() - 0.5);

    return tex_nature_hdri.SampleLevel(tex_nature_hdri_sampler, float2(u, v), 0).rgb;


    return float3(1, 1, 1);

}

// Randomly sample a point on the surface of unit hemisphere
float3 sample_hemisphere(float3 normal, inout RNG rng) {
    
    float3 candidate_vec;
    
    while(true) {
        // Random sampling in (-1,-1,-1), (1,1,1)
        candidate_vec = 2.0 * float3(rng.random_float(), rng.random_float(), rng.random_float()) - float3(1,1,1);
        
        // Good if the sampling point is in an unit sphere
        if (length(candidate_vec) <= 1.0) break;
    }

    // Alywas point to the normal direction
    if (dot(normal, candidate_vec) < 0) candidate_vec = -candidate_vec;

    return normalize(candidate_vec); 
}

// Randomly sample a point on the surface of unit hemisphere
float2 sample_disk(inout RNG rng) {
    
    float2 candidate_vec;
    
    while(true) {
        // Random sampling in (-1,-1), (1,1)
        candidate_vec = 2.0 * float2(rng.random_float(), rng.random_float()) - float2(1,1);
        
        // Good if the sampling point is in an unit sphere
        if (length(candidate_vec) <= 1.0) break;
    }

    return candidate_vec; 
}

// rd is the direct reflection direction
// n controls glossiness
float3 apply_glossy(float3 rd, float n, inout RNG rng) {
    
    float u0 = rng.random_float();
    float u1 = rng.random_float();
    
    float3 t = normalize(rd);
    float3 w = normalize(float3(abs(rd.x), abs(rd.y), abs(rd.z)));
    if (w.x < w.y) {
        if (w.x < w.z) t.x = 1.0f;
        else t.z = 1.0f;
    }
    else if (w.y < w.z) t.y = 1.0f;
    else t.z = 1.0f;
    
    w = normalize(rd);
    float3 u = normalize(cross(t, w));
    float3 v = cross(w, u);

    float theta = acos(pow(1.0f - u0, 1.0f / (1.0f + n)));
    float phi = 2.0f * PI * u1;
    float3 a = float3(cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta));
    float3 R = 0;

    return 0;
}

float3 calc_fake_key_light(float3 rd) {
    float3 light_dir = float3(1,0,0);

    light_dir = normalize(light_dir);

    float cos_simil = dot(light_dir, normalize(rd));

    return cos_simil > 0 ? cos_simil * cos_simil : 0;
}


float3 get_incident_L(float3 rp, float3 rd, float3 c_low, float3 c_high, int nBounces, inout RNG rng) {
    float3 L = float3(0.0, 0.0, 0.0);
    float throughput = 1.0;
    float albedo = sigma_s / (sigma_a + sigma_s);
    float rho_max_inv = 1.0 / trace_to_rho(trace_max);

    // Combining rgb for now since less code change is needed, eventually need an array for each wavelength
    float3 throughput_rgb = float3(1.0, 1.0, 1.0);
    float3 albedo_rgb = float3(sigma1_s_r / (sigma1_a_r + sigma1_s_r),
                               sigma1_s_g / (sigma1_a_g + sigma1_s_g),
                               sigma1_s_b / (sigma1_a_b + sigma1_s_b));

    #ifdef POINT_LIGHT
    float3 trim_min = float3(trim_x_min, trim_y_min, trim_z_min);
    float3 trim_max = float3(trim_x_max, trim_y_max, trim_z_max);
    float3 l_rel_pos = 0.5 * (trim_min + trim_max);
    // float3 lp = c_low + l_rel_pos * c_high;
    float3 lp = c_low + float3(1.0 * c_high.x, c_high.y / 2.0 + light_pos, c_high.z / 2.0);
    #endif

    bool in_volume = false;
    int current_layer = 0;

    int bounce_n = 0;
    for (int n = 0; n < nBounces; n++) {
       

        // Sample collision distance
        float2 t = ray_AABB_intersection(rp, rd, c_low, c_high);
        // if (t.x == -1 && t.y == -1) return float3(0,0,0); // ray_AABB_intersection failed

        float t_event;
        bool hit_surface = false;

        #ifdef SPHERE_DEBUG
        if (in_volume) {
            t_event = sphere_tracking_in_volume(rp, rd, 0.0, t.y, rho_max_inv, rng, hit_surface);
        }
        else {
            t_event = sphere_tracking_out_volume(rp, rd, 0.0, t.y, rho_max_inv, rng, hit_surface);
        }
        #else
        if (in_volume) {
            t_event = delta_tracking_multi_layers(rp, rd, 0.0, t.y, rho_max_inv, rng, current_layer, hit_surface);
            if (t_event == 0) return float3(1,1,5);
        }
        else {
            t_event = delta_tracking_multi_layers(rp, rd, 0.0, t.y, rho_max_inv, rng, current_layer, hit_surface);
        }
        #endif

        // If the ray gets out of AABB, return color
        if (t_event >= t.y) {

            // return float3(0,0,0);

            // Render HDRI as a texture
            if (bounce_n == 0) {
                // return calc_ambient_color(rp, rd, 0.001, rng) * 1;
                return float3(0,0,0);
            }
            // Render HDRI as a light
            return L + throughput_rgb * calc_ambient_color(rp, rd, 0, rng) * 2;
            return float3(0,0,0);
            // if (n == 0) return float3(0,0,0);
            // else return L + throughput_rgb * get_sky_L(rd);
        }

        //return float3(1,0,0);

        // Move to the next intersection
        rp += t_event * rd;

        #ifdef POINT_LIGHT
        // If a ray hits a surface and get inside.
        if (hit_surface) {

            // frenels()
            float reflect_chance = fresnel(rp, rd);

            // Calculate Light Contribution
            float3 ld = normalize(lp - rp); // surface to light
            float l_distance = length(ld);
            float transmittance = occlusion_tracking(rp, ld, 0.0, l_distance, rho_max_inv, 10.0, rng);
            //float light_contribution = 100.0 * transmittance / max(l_distance * l_distance, 1.0);
            float3 light_reflect = reflect(rp, ld);  // surface to reflected light
            float specular_factor = dot(normalize(rd), normalize(light_reflect)); 

            specular_factor = pow(specular_factor, shininess);

            // REFLECT
            if (rng.random_float() < reflect_chance) {
                rd = reflect(rp, rd);
            }
            else {
                float3 refract_dir = refract(rp, rd);
                if (refract_dir.x == 0 && refract_dir.y == 0 && refract_dir.z == 0) {
                    rd = reflect(rp, rd);
                }
                else {
                    rd = refract_dir;
                    rp += rd * RAY_EPSILON;
                    in_volume = !in_volume;
                    if (current_layer == 1) throughput_rgb *= float3(0.98, 0.98, 0.98);
                    if (current_layer == 2) throughput_rgb *= albedo_rgb;
                }
            }

            // // Calculate specular light contribution with the probability of specular_factor
            // //if (1<0){
            // if (specular_factor > 0) {

            //     // Reflect 
            //     if (rng.random_float() < specular_factor) {
            //         rd = reflect(rp, rd);
            //     }
            //     else {
            //         throughput_rgb = throughput_rgb;
            //         in_volume = !in_volume;
            //         //rd = sample_HG(rd, scattering_anisotropy, rng);
            //         throughput_rgb *= albedo_rgb;
            //     }

            //     // if (rng.random_float() < specular_factor) {
            //     // //if (0) {    // Just reflect, don't explicitely calculate specular

            //     //     float intensity = 1.0f;
            //     //     int light_exposure = 5;   

            //     //     float3 specular_color = float3(1,1,1) * specular_factor * intensity * pow(2, light_exposure);
            //     //     return throughput_rgb * specular_color * transmittance / specular_factor;
            //     // }
            //     // else {
            //     //     in_volume = !in_volume;
            //     //     throughput_rgb = throughput_rgb / (1 - specular_factor);
            //     //     throughput_rgb *= albedo_rgb;
            //     // }
            // }
            // else{
            //     //return float3(100,0,0);
            //     in_volume = !in_volume;
            //     throughput_rgb *= albedo_rgb;
            // }
        }
        else {
            //return float3(100,0,0);
            if (current_layer == 1) throughput_rgb *= float3(0.98, 0.98, 0.98);
            if (current_layer == 2) throughput_rgb *= albedo_rgb;
            rd = sample_HG(rd, scattering_anisotropy, rng);
        }
        #else
        //rd = sample_HG(rd, scattering_anisotropy, rng);
        #endif

        // Since light raytracing and volume raytracing are independent, peform light tracing only when out-volume mode
        if (!in_volume) {
            if (bounce_n == 0) {
                if (intersect_torus(rp, rd)) {
                    int torus_light_exposure = 4;
                    return L + throughput_rgb * float3(1,1,1) * pow(2, torus_light_exposure);
                }
                //if (intersect_plane(float3(0, -100, -100), float3(0, 100, 100), rp, rd)) return L + throughput_rgb * float3(1,1,1) * 3.0;
            }
            else {
                bool hit_surface_shadow_ray = false;
                delta_tracking_out_volume(rp, rd, 0.0, t.y, rho_max_inv, rng, hit_surface_shadow_ray);

                if (!hit_surface_shadow_ray) {
                    return L + throughput_rgb * float3(1,1,1) * calc_fake_key_light(rd) * pow(2, 4);
                }
            }
        }
        

        // Adjust the path throughput (RR or modulate)
        #ifdef RUSSIAN_ROULETTE
        if (n >= 10) {
            // Terminate by 50% chancev after 15 bounces
            float p = 0.5;
            if (rng.random_float() < p) {
                return float3(0,0,0);
            }
            else {
                throughput_rgb = throughput_rgb / (1 - p);
            }
        }
        #endif

        bounce_n++;
    }
    return float3(0,0,0);;
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
        
        if (0) { // ignore ray marching
        // if (sigma_s < INTENSITY_EPSILON) {
        // If there's no appreciable scattering, we can just use the
        // emission-absorption model and ray-march the solution
            int iSteps = int(length(rd));
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
          float lens_radius = aperture / 2.0;
          float2 random_disk = sample_disk(rng);
          float2 random_lens = lens_radius * random_disk;
          float3 offset = camZ * random_lens.x + camY * random_lens.y;

          rp = rp + offset * focus_dist;
          rd = normalize(rd - offset / focus_dist);
          path_L = get_incident_L(rp, rd, float3(0.0, 0.0, 0.0), float3(grid_x, grid_y, grid_z), n_bounces + 1, rng);
        }
    } else {
        path_L = get_sky_L(rd);
        //path_L = float3(0,0,0);
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