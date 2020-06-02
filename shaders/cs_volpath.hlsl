static const float PI = 3.141592;
static const float PI2 = PI * 2.0;
static const int PT_GROUP_SIZE_X = 10;
static const int PT_GROUP_SIZE_Y = 10;

RWTexture2D<float4> tex_accumulator: register(u0);
RWTexture3D<half4> tex_trace: register(u1);

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
    float world_x;
    float world_y;
    float world_z;
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

float ray_sphere_intersection(float3 rd, float3 rp, float3 p, float r) {
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

float ray_AABB_intersection(float3 rd, float3 rp, float3 c_lo, float3 c_hi)
{
    // r.dir is unit direction vector of ray
    float3 dirfrac = 1.0 / rd;

    // lb is the corner of AABB with minimal coordinates -
    // - left bottom, rt is maximal corner; r.org is origin of ray
    float t1 = (c_lo.x - rp.x) * dirfrac;
    float t2 = (c_hi.x - rp.x) * dirfrac;
    float t3 = (c_lo.y - rp.y) * dirfrac;
    float t4 = (c_hi.y - rp.y) * dirfrac;
    float t5 = (c_lo.z - rp.z) * dirfrac;
    float t6 = (c_hi.z - rp.z) * dirfrac;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
    if (tmax < 0.0) {
        return tmax;
    }
    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return -tmax;
    }
    return tmin;
};

float ray_AABB_intersection_2(float3 rd, float3 rp, float3 c_lo, float3 c_hi) {
    float t[10];
    t[1] = (c_lo.x - rp.x) / rd.x;
    t[2] = (c_hi.x - rp.x) / rd.x;
    t[3] = (c_lo.y - rp.y) / rd.y;
    t[4] = (c_hi.y - rp.y) / rd.y;
    t[5] = (c_lo.z - rp.z) / rd.z;
    t[6] = (c_hi.z - rp.z) / rd.z;
    t[7] = max(max(min(t[1], t[2]), min(t[3], t[4])), min(t[5], t[6]));
    t[8] = min(min(max(t[1], t[2]), max(t[3], t[4])), max(t[5], t[6]));
    t[9] = (t[8] < 0 || t[7] > t[8]) ? -1.0 : t[7];
    return t[9];
}

float3x3 get_view_matrix(float3 cam_pos) {
	float3 y = float3(0,1,0);
	float3 z = normalize(cam_pos);
	float3 x = normalize(cross(y, z));
	y = normalize(cross(z, x));
	return float3x3(
		x.x, y.x, z.x,
		x.y, y.y, z.y,
		x.z, y.z, z.z);
}

[numthreads(PT_GROUP_SIZE_X, PT_GROUP_SIZE_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint2 pixel_xy = dispatchThreadId.xy;
    uint idx = threadIDInGroup + PT_GROUP_SIZE_X * PT_GROUP_SIZE_Y * groupID;

    RNG rng;
    rng.set_seed(
        rng.wang_hash(73*idx),
        rng.wang_hash(pixel_xy.x * pixel_xy.y * pt_iteration)
    );

    // Compute x and y ray directions in "neutral" camera position.
    float aspect_ratio = float(screen_width) / float(screen_height);
    float rx = float(pixel_xy.x + rng.random_float()) / float(screen_width) * 2.0 - 1.0;
    float ry = float(pixel_xy.y + rng.random_float()) / float(screen_height) * 2.0 - 1.0;
    ry /= aspect_ratio;

    float screen_distance = 3.73205;
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

    // Ray start and direction
    float3 rp = camera_pos;
    float3 rd = normalize(screen_pos - rp);

    // Get current ray's color
    float t = 0.0;
    float3 path_L = float3(0.0, 0.0, 0.0);
    float3 diagonal_AABB = float3(1.0, world_y / world_x, world_z / world_x);
    float3 c_low = -0.5 * diagonal_AABB;
    float3 c_high = 0.5 * diagonal_AABB;
    t = ray_AABB_intersection_2(rd, rp, c_low, c_high);
    // t = ray_sphere_intersection(rd, rp, float3(0.0, 0.0, 0.0), 1.0);
    if (t > 0) {
        path_L = float3(1.0-exp(-0.1*t), t, 0.0);
    }

    // // Reinhard tone mapping
    // float l = dot(float3(0.2126, 0.7152, 0.0722), final_color);
    // final_color /= l + 1;

    // // Average values over time.
    // tex[p] = float4(final_color, 1.0f) / float(step) + tex[p] * float(step - 1) / float(step);

    tex_accumulator[pixel_xy] = float4(path_L.r, 0.0, 0.0, 1.0);
}