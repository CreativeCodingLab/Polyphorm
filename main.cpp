#include "platform.h"
#include "graphics.h"
#include "file_system.h"
#include "maths.h"
#include "memory.h"
#include "ui.h"
#include "font.h"
#include "input.h"
#include "random.h"
#include <cassert>
#include <mmsystem.h>
#include "logging.h"
#include <sstream>
#include <fstream>

//*** Truncation from double to float warning
#pragma warning(disable:4305)
#pragma warning(disable:4244)

//====================================================================

//*** Work regimes: Uncomment exactly one!
#define REGIME_SDSS
// #define REGIME_FRB
// #define REGIME_TNG
// #define REGIME_BOLSHOI_PLANCK
// #define REGIME_ROCKSTAR
// #define REGIME_POISSON
// #define REGIME_CONNECTOME
// #define REGIME_EMBEDDING

//*** Enable velocity analysis, which will change the trace volume from <half> to <half4>
//*** with the extra 3 channels storing the equilibrium mean unsigned orientation of the agents
// #define VELOCITY_ANALYSIS

#define HALO_COLOR_ANALYSIS

//*** How to initialize the agents in their 3D domain: Uncomment exactly one!
#define AGENTS_INIT_AROUND_DATA
// #define AGENTS_INIT_RANDOMLY

//====================================================================

#ifdef REGIME_SDSS
#define DATASET_NAME "data/SDSS/galaxiesInSdssSlice_viz_bigger_lumdist_t=0.0"
//#define DATASET_NAME "data/SDSS/galaxiesInSdssSlice_viz_huge_t=10.3"
// #define DATASET_NAME "data/SDSS/sdssGalaxy_rsdCorr_dbscan_e2p0ms3_dz0p001_m10p0_t=10.3"
#define COLOR_PALETTE_TRACE "data/palette_sunset3.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 2.55;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.1;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.91;
const float SAMPLING_EXPONENT = 3.5;
#endif

#ifdef REGIME_BOLSHOI_PLANCK
// #define DATASET_NAME "data/BP/bpdat_boxDist_trimDist_trimMass_t=0.3548_subrate=1"
#define DATASET_NAME "data/BP/bpdat_boxDist_trimDist_trimMass_t=0.05_subrate=1_ROTATED"
// #define COLOR_PALETTE_TRACE "data/palette_sunset2.tga"
#define COLOR_PALETTE_TRACE "data/palette_gogh_green.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 2.5;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.1;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.885;
const float SAMPLING_EXPONENT = 3.0;
#endif

#ifdef REGIME_TNG
#define DATASET_NAME "data/TNG/tng100-1_allSubHalos_spinEtc_t=0.0"
#define COLOR_PALETTE_TRACE "data/palette_sunset3.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 2.55;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.1;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.91;
const float SAMPLING_EXPONENT = 3.8;
#endif

#ifdef REGIME_FRB
#define DATASET_NAME "data/FRB/frb_field_cigaleMass_t=0.0_z=0.01-0.1"
#define COLOR_PALETTE_TRACE "data/palette_sunset3.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 5.0;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.2;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.93;
const float SAMPLING_EXPONENT = 3.6;
#endif

#ifdef REGIME_ROCKSTAR
#define DATASET_NAME "data/MassiveNuS/rockstar_mnv0.10000_om0.30000_As2.1000_out_66t=0.0roi=256.0"
#define COLOR_PALETTE_TRACE "data/palette_magma.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
#endif

#ifdef REGIME_POISSON
// #define DATASET_NAME "data/Conduits/poisson_256_2d_conduits_n_levels=100_ratio=1.05"
// #define DATASET_NAME "data/Poisson/regular_4096_3d"
// #define DATASET_NAME "data/Poisson/random_4096_3d"
#define DATASET_NAME "data/Poisson/poisson_4096_2d_3d"
// #define DATASET_NAME "data/Poisson/poisson_256_2d_3d_flattened"
#define COLOR_PALETTE_TRACE "data/palette_hot.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 7.5;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.1;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.96;
const float SAMPLING_EXPONENT = 4.5;
#endif

#ifdef REGIME_CONNECTOME
#define DATASET_NAME "data/Connectome/connectome0_XYZW"
#define COLOR_PALETTE_TRACE "data/palette_magneto2.tga"
#define COLOR_PALETTE_DATA "data/palette_hot.tga"
#endif

#ifdef REGIME_EMBEDDING
#define DATASET_NAME "data/Embeddings/W2V_UMAP_params_15_n=296630"
#define COLOR_PALETTE_TRACE "data/palette_magma.tga"
#define COLOR_PALETTE_DATA "data/palette_gogh_blue.tga"
const float SENSE_SPREAD = 20.0;
const float SENSE_DISTANCE = 3.0;
const float MOVE_ANGLE = 10.0;
const float MOVE_DISTANCE = 0.1;
const float AGENT_DEPOSIT = 0.0;
const float PERSISTENCE = 0.91;
const float SAMPLING_EXPONENT = 3.5;
#endif

// Other hardwired settings ==========================================
const int32_t THREAD_GROUP_SIZE = 1000; // Divisible by 10! Must align with settings inside the agent shader!
const uint32_t N_HISTOGRAM_BINS = 17; // Must align with settings inside the histo shader!
const int32_t PT_GROUP_SIZE_X = 10; // Must align with settings inside the PT shader!
const int32_t PT_GROUP_SIZE_Y = 10; // Must align with settings inside the PT shader!
const int32_t N_AGENTS_TO_CAPTURE = 1e3;
const int32_t N_AGENT_TIMESTEPS_TO_CAPTURE = 10;

//====================================================================

float world_to_grid(float world_pos_mpc, float world_size_mpc, float world_center_mpc, float grid_size_vox)
{
    float norm_pos = (world_pos_mpc - (world_center_mpc - 0.5 * world_size_mpc)) / world_size_mpc;
    return norm_pos * grid_size_vox;
}
float measure_world_to_grid(float distance_mpc, float size_world_mpc, float size_grid_vox)
{
    return size_grid_vox * distance_mpc / size_world_mpc;
}

float grid_to_world(float grid_pos_vox, float world_size_mpc, float world_center_mpc, float grid_size_vox)
{
    float norm_pos = grid_pos_vox / grid_size_vox;
    return (world_center_mpc - 0.5 * world_size_mpc) + norm_pos * world_size_mpc;
}
float measure_grid_to_world(float distance_vox, float size_world_mpc, float size_grid_vox)
{
    return size_world_mpc * distance_vox / size_grid_vox;
}

int nearest_multiple_of(int32_t n, int32_t m)
{
    int32_t r = (n-1) % m + 1;
    return n + (m - r);
}

enum VisualizationMode {
    VM_VOLUME,
    VM_VOLUME_HIGHLIGHT,
    VM_VOLUME_OVERDENSITY,
    VM_VOLUME_VELOCITY,
    VM_PARTICLES,
    VM_PATH_TRACING
};

struct SimulationConfig {
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
    int filler3;
};

struct RenderingConfig {
    Matrix4x4 projection;
    Matrix4x4 view;
    Matrix4x4 model;

    int texcoord_map;
    float trim_x_min;
    float trim_x_max;
    float trim_y_min;

    float trim_y_max;
    float trim_z_min;
    float trim_z_max;
    float trim_density;

    float world_width;
    float world_height;
    float world_depth;
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
    float sigma_t_rgb;
    float albedo_r;
    float albedo_g;
    
    float albedo_b;
    int tmp1;
    int tmp2;
    int tmp3;

    // add new variables before albedos. Some bug.
    // order matters
    // render terminates if not multiple of 4, or not every parameter specified

};

struct StatisticsConfig {
    int n_data_points;
    int n_histo_bins;
    float histogram_base;
    int sample_randomly;

    float world_width;
    float world_height;
    float world_depth;
    int filler3;
};

float quad_vertices[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f,

    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
};

uint32_t quad_vertices_stride = sizeof(float) * 6;
uint32_t quad_vertices_count = 6;

int main(int argc, char **argv)
{
    // Load configuration file
    std::ifstream config_file;
    config_file.open("config.polyp", std::ofstream::in);
    if (!config_file.good()) {
        printf("Config file missing in the root directory!\n\n");
        return 0;
    }
    int32_t NUM_AGENTS;
    uint32_t GRID_RESOLUTION, SCREEN_X, SCREEN_Y;
    float HISTOGRAM_BASE, GRID_PADDING, CAMERA_FOV;
    std::string varname;
    std::getline(config_file, varname, '='); config_file >> NUM_AGENTS;
    std::getline(config_file, varname, '='); config_file >> GRID_RESOLUTION;
    std::getline(config_file, varname, '='); config_file >> GRID_PADDING;
    std::getline(config_file, varname, '='); config_file >> SCREEN_X;
    std::getline(config_file, varname, '='); config_file >> SCREEN_Y;
    std::getline(config_file, varname, '='); config_file >> CAMERA_FOV;
    std::getline(config_file, varname, '='); config_file >> HISTOGRAM_BASE;
    config_file.close();

    // Window setup
    uint32_t window_width = SCREEN_X, window_height = SCREEN_Y;
 	Window window = platform::get_window("Space Physarum", window_width, window_height);
    assert(platform::is_window_valid(&window));

    // Data setup
        // Load dataset description from metafile
    std::string filename(DATASET_NAME);
    std::ifstream metadata_file;
    metadata_file.open((filename + "_metadata.txt").c_str(), std::ofstream::in);
    if (!metadata_file.good()) {
        printf("Metadata file missing!\n\n");
        return 0;
    }
    float data_x_min, data_x_max, data_y_min, data_y_max, data_z_min, data_z_max;
    float mean_weight;
    int data_count;
    std::getline(metadata_file, varname, '='); metadata_file >> data_count;
    std::getline(metadata_file, varname, '='); metadata_file >> data_x_min;
    std::getline(metadata_file, varname, '='); metadata_file >> data_x_max;
    std::getline(metadata_file, varname, '='); metadata_file >> data_y_min;
    std::getline(metadata_file, varname, '='); metadata_file >> data_y_max;
    std::getline(metadata_file, varname, '='); metadata_file >> data_z_min;
    std::getline(metadata_file, varname, '='); metadata_file >> data_z_max;
    std::getline(metadata_file, varname, '='); metadata_file >> mean_weight;
    metadata_file.close();

        // Load binary data points
    File raw_data = file_system::read_file((filename + ".bin").c_str());
    float *input_data = (float*)raw_data.data;
    printf("\n-> input data points: %d\n", data_count);
    printf("-> number of agents: %d\n", NUM_AGENTS);
    int32_t NUM_PARTICLES = NUM_AGENTS + data_count;

    // World and grid setup
        // Set world size to encapsulate data
    float WORLD_SIZE_X = data_x_max - data_x_min;
    float WORLD_SIZE_Y = data_y_max - data_y_min;
    float WORLD_SIZE_Z = data_z_max - data_z_min;

        // Pad the domain so that agents don't leak over the boundary
    float WORLD_SIZE_MAX = math::max(math::max(WORLD_SIZE_X, WORLD_SIZE_Y), WORLD_SIZE_Z);
    WORLD_SIZE_X += GRID_PADDING * WORLD_SIZE_MAX;
    WORLD_SIZE_Y += GRID_PADDING * WORLD_SIZE_MAX;
    WORLD_SIZE_Z += GRID_PADDING * WORLD_SIZE_MAX;
    WORLD_SIZE_MAX = math::max(math::max(WORLD_SIZE_X, WORLD_SIZE_Y), WORLD_SIZE_Z);

    float WORLD_CENTER_X = 0.5 * (data_x_max + data_x_min);
    float WORLD_CENTER_Y = 0.5 * (data_y_max + data_y_min);
    float WORLD_CENTER_Z = 0.5 * (data_z_max + data_z_min);

        // Derive the simulation grid resolution by using the specified resolution for the longest dimension
    const uint32_t GRID_RESOLUTION_X = (uint32_t)nearest_multiple_of(int32_t(float(GRID_RESOLUTION) * (WORLD_SIZE_X / WORLD_SIZE_MAX)), 8);
    const uint32_t GRID_RESOLUTION_Y = (uint32_t)nearest_multiple_of(int32_t(float(GRID_RESOLUTION) * (WORLD_SIZE_Y / WORLD_SIZE_MAX)), 8);
    const uint32_t GRID_RESOLUTION_Z = (uint32_t)nearest_multiple_of(int32_t(float(GRID_RESOLUTION) * (WORLD_SIZE_Z / WORLD_SIZE_MAX)), 8);

        // Adjust world coords to match grid proportions
    WORLD_SIZE_Y = float(GRID_RESOLUTION_Y) * WORLD_SIZE_X / float(GRID_RESOLUTION_X);
    WORLD_SIZE_Z = float(GRID_RESOLUTION_Z) * WORLD_SIZE_X / float(GRID_RESOLUTION_X);

    printf("-> simulation grid resolution: %d x %d x %d\n", GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z);
    printf("-> simulation domain: %.2f x %.2f x %.2f Mpc\n", WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);

        // Write out proxy sightline coords
    // float sightline_x = world_to_grid(0.0, WORLD_SIZE_X, WORLD_CENTER_X, GRID_RESOLUTION_X);
    // float sightline_y = world_to_grid(0.0, WORLD_SIZE_Y, WORLD_CENTER_Y, GRID_RESOLUTION_Y);
    // float sightline_z = world_to_grid(0.0, WORLD_SIZE_Z, WORLD_CENTER_Z, GRID_RESOLUTION_Z);
    // printf("-> proxy sightline orig: %.4f, %.4f, %.4f\n", sightline_x / float(GRID_RESOLUTION_X), sightline_y / float(GRID_RESOLUTION_Y), sightline_z / float(GRID_RESOLUTION_Z));
    // sightline_x = world_to_grid(-396.65753941785533, WORLD_SIZE_X, WORLD_CENTER_X, GRID_RESOLUTION_X); // SDSS_huge
    // sightline_y = world_to_grid(-48.059608925563694, WORLD_SIZE_Y, WORLD_CENTER_Y, GRID_RESOLUTION_Y);
    // sightline_z = world_to_grid(188.98960398099783, WORLD_SIZE_Z, WORLD_CENTER_Z, GRID_RESOLUTION_Z);
    // sightline_x = world_to_grid(308.97582987438165, WORLD_SIZE_X, WORLD_CENTER_X, GRID_RESOLUTION_X); // FRB_cigale
    // sightline_y = world_to_grid(-150.56131422700057, WORLD_SIZE_Y, WORLD_CENTER_Y, GRID_RESOLUTION_Y);
    // sightline_z = world_to_grid(-47.68885835995201, WORLD_SIZE_Z, WORLD_CENTER_Z, GRID_RESOLUTION_Z);
    // printf("-> proxy sightline dir: %.4f, %.4f, %.4f\n", sightline_x / float(GRID_RESOLUTION_X), sightline_y / float(GRID_RESOLUTION_Y), sightline_z / float(GRID_RESOLUTION_Z));

    // Init graphics
    printf("\nInitializing graphics...\n");
    graphics::init();
    graphics::init_swap_chain(&window);

    font::init();
    ui::init((float)window_width, (float)window_height);
    ui::set_input_responsive(true);

    // Create window render target
	RenderTarget render_target_window = graphics::get_render_target_window();
    assert(graphics::is_ready(&render_target_window));
    DepthBuffer depth_buffer = graphics::get_depth_buffer(window_width, window_height);
    assert(graphics::is_ready(&depth_buffer));
    graphics::set_render_targets_viewport(&render_target_window, &depth_buffer);

    // Rendering shader for rendering individual particles
    File draw_compute_shader_file_particle = file_system::read_file("cs_particles_transform.hlsl");
    ComputeShader draw_compute_shader_particle = graphics::get_compute_shader_from_code((char *)draw_compute_shader_file_particle.data, draw_compute_shader_file_particle.size);
    file_system::release_file(draw_compute_shader_file_particle);
    assert(graphics::is_ready(&draw_compute_shader_particle));
    printf("cs_particles_transform shader compiled...\n");

    // Shader for blitting from uint to float texture.
    File blit_compute_shader_file = file_system::read_file("cs_particles_blit.hlsl");
    ComputeShader blit_compute_shader = graphics::get_compute_shader_from_code((char *)blit_compute_shader_file.data, blit_compute_shader_file.size);
    file_system::release_file(blit_compute_shader_file);
    assert(graphics::is_ready(&blit_compute_shader));
    printf("cs_particles_blit shader compiled...\n");

    // Vertex shader
    File vertex_shader_file = file_system::read_file("vs_3d.hlsl"); 
    VertexShader vertex_shader = graphics::get_vertex_shader_from_code((char *)vertex_shader_file.data, vertex_shader_file.size);
    file_system::release_file(vertex_shader_file);
    assert(graphics::is_ready(&vertex_shader));
    printf("vs_3d shader compiled...\n");

    // Pixel shader
    File pixel_shader_file = file_system::read_file("ps_volume_trace.hlsl"); 
    PixelShader pixel_shader = graphics::get_pixel_shader_from_code((char *)pixel_shader_file.data, pixel_shader_file.size);
    file_system::release_file(pixel_shader_file);
    assert(graphics::is_ready(&pixel_shader));
    printf("ps_volume_trace shader compiled...\n");

    // Volume shader for highlighting AOIs
    File ps_volume_highlight_file = file_system::read_file("ps_volume_highlight.hlsl"); 
    PixelShader ps_volume_highlight = graphics::get_pixel_shader_from_code((char *)ps_volume_highlight_file.data, ps_volume_highlight_file.size);
    file_system::release_file(ps_volume_highlight_file);
    assert(graphics::is_ready(&ps_volume_highlight));
    printf("ps_volume_highlight shader compiled...\n");

    // Volume shader for segmenting overdensities
    File ps_volume_overdensity_file = file_system::read_file("ps_volume_overdensity.hlsl"); 
    PixelShader ps_volume_overdensity = graphics::get_pixel_shader_from_code((char *)ps_volume_overdensity_file.data, ps_volume_overdensity_file.size);
    file_system::release_file(ps_volume_overdensity_file);
    assert(graphics::is_ready(&ps_volume_overdensity));
    printf("ps_volume_overdensity shader compiled...\n");

    // Volume shader for visualizing filament directionality/velocity
    File ps_volume_velocity_file = file_system::read_file("ps_volume_velocity.hlsl"); 
    PixelShader ps_volume_velocity = graphics::get_pixel_shader_from_code((char *)ps_volume_velocity_file.data, ps_volume_velocity_file.size);
    file_system::release_file(ps_volume_velocity_file);
    assert(graphics::is_ready(&ps_volume_velocity));
    printf("ps_volume_velocity shader compiled...\n");

    // Particle system shader
    File compute_shader_file = file_system::read_file("cs_agents_propagate.hlsl");
    ComputeShader compute_shader = graphics::get_compute_shader_from_code((char *)compute_shader_file.data, compute_shader_file.size);
    file_system::release_file(compute_shader_file);
    assert(graphics::is_ready(&compute_shader));
    printf("cs_agents_propagate shader compiled...\n");

    // Particle sorting shader
    File sort_shader_file = file_system::read_file("cs_agents_sort.hlsl");
    ComputeShader sort_shader = graphics::get_compute_shader_from_code((char *)sort_shader_file.data, sort_shader_file.size);
    file_system::release_file(sort_shader_file);
    assert(graphics::is_ready(&sort_shader));
    printf("cs_agents_sort shader compiled...\n");

    // Decay/diffusion shader
    File decay_compute_shader_file = file_system::read_file("cs_field_decay.hlsl");
    ComputeShader decay_compute_shader = graphics::get_compute_shader_from_code((char *)decay_compute_shader_file.data, decay_compute_shader_file.size);
    file_system::release_file(decay_compute_shader_file);
    assert(graphics::is_ready(&decay_compute_shader));
    printf("cs_field_decay shader compiled...\n");

    // Trail Diffuse shader
    File decay_trail_shader_file = file_system::read_file("cs_field_decay_trail.hlsl");
    ComputeShader decay_trail_shader = graphics::get_compute_shader_from_code((char *)decay_trail_shader_file.data, decay_trail_shader_file.size);
    file_system::release_file(decay_trail_shader_file);
    assert(graphics::is_ready(&decay_trail_shader));
    printf("cs_field_trail shader compiled...\n");

    // Vertex shader for displaying textures.
    vertex_shader_file = file_system::read_file("vs_2d.hlsl"); 
    VertexShader vertex_shader_2d = graphics::get_vertex_shader_from_code((char *)vertex_shader_file.data, vertex_shader_file.size);
    file_system::release_file(vertex_shader_file);
    assert(graphics::is_ready(&vertex_shader_2d));
    printf("vs_2d shader compiled...\n");

    // Pixel shader for displaying textures.
    pixel_shader_file = file_system::read_file("ps_particles_color.hlsl"); 
    PixelShader pixel_shader_2d = graphics::get_pixel_shader_from_code((char *)pixel_shader_file.data, pixel_shader_file.size);
    file_system::release_file(pixel_shader_file);
    assert(graphics::is_ready(&pixel_shader_2d));
    printf("ps_particles_color shader compiled...\n");

    // Particle density histogram shader
    File file_cs_density_histo = file_system::read_file("cs_density_histo.hlsl");
    ComputeShader cs_density_histo = graphics::get_compute_shader_from_code((char *)file_cs_density_histo.data, file_cs_density_histo.size);
    file_system::release_file(file_cs_density_histo);
    assert(graphics::is_ready(&cs_density_histo));
    printf("cs_density_histo shader compiled...\n");

    // Volumetric path tracer
    File file_cs_volpath = file_system::read_file("cs_volpath.hlsl");
    ComputeShader cs_volpath = graphics::get_compute_shader_from_code((char *)file_cs_volpath.data, file_cs_volpath.size);
    file_system::release_file(file_cs_volpath);
    assert(graphics::is_ready(&cs_volpath));
    printf("cs_volpath shader compiled...\n");

    File file_ps_volpath = file_system::read_file("ps_volpath.hlsl");
    PixelShader ps_volpath = graphics::get_pixel_shader_from_code((char *)file_ps_volpath.data, file_ps_volpath.size);
    file_system::release_file(file_ps_volpath);
    assert(graphics::is_ready(&ps_volpath));
    printf("ps_volpath shader compiled...\n");

    // Textures for the simulation
    // Texture3D trail_tex_A = graphics::get_texture3D(NULL, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, DXGI_FORMAT_R16_FLOAT, 2);
    // Texture3D trail_tex_B = graphics::get_texture3D(NULL, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, DXGI_FORMAT_R16_FLOAT, 2);
    Texture3D trail_tex_A = graphics::load_texture3D("export_1/deposit.dds");
    Texture3D trail_tex_B = graphics::load_texture3D("export_1/deposit.dds");
    #ifdef VELOCITY_ANALYSIS
    Texture3D trace_tex = graphics::get_texture3D(NULL, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, DXGI_FORMAT_R16G16B16A16_FLOAT, 8);
    #else
    Texture3D trace_tex = graphics::load_texture3D("export_1/trace.dds");
    // Texture3D trace_tex = graphics::get_texture3D(NULL, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, DXGI_FORMAT_R16_FLOAT, 2);
    #endif
    Texture2D display_tex = graphics::get_texture2D(NULL, window_width, window_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    Texture2D display_tex_uint = graphics::get_texture2D(NULL, window_width, window_height, DXGI_FORMAT_R32_UINT, 4);
    Texture2D palette_trace_tex = graphics::load_texture2D(COLOR_PALETTE_TRACE);
    Texture2D palette_data_tex = graphics::load_texture2D(COLOR_PALETTE_DATA);

    TextureSampler tex_sampler_trace = graphics::get_texture_sampler(CLAMP, D3D11_FILTER_ANISOTROPIC);
    TextureSampler tex_sampler_deposit = graphics::get_texture_sampler(CLAMP, D3D11_FILTER_ANISOTROPIC);
    TextureSampler tex_sampler_display = graphics::get_texture_sampler();
    TextureSampler tex_sampler_color_palette = graphics::get_texture_sampler();

    // HDRI Image for Vol Path Rendering
    Texture2D nature_hdri_tex = graphics::load_texture2D("textures/autumn_ground_8k.tga");
    TextureSampler tex_sampler_nature_hdri = graphics::get_texture_sampler();
    
	graphics::set_blend_state(BlendType::ALPHA);

    // Particles setup
    float *particles_x = memory::alloc_heap<float>(NUM_PARTICLES);
    float *particles_y = memory::alloc_heap<float>(NUM_PARTICLES);
    float *particles_z = memory::alloc_heap<float>(NUM_PARTICLES);
    float *particles_phi = memory::alloc_heap<float>(NUM_PARTICLES);
    float *particles_theta = memory::alloc_heap<float>(NUM_PARTICLES);
    float *particles_weights = memory::alloc_heap<float>(NUM_PARTICLES);
    float *halos_densities = memory::alloc_heap<float>(data_count);

    unsigned int *density_histogram = memory::alloc_heap<unsigned int>(N_HISTOGRAM_BINS);
    for (int i = 0; i < N_HISTOGRAM_BINS; ++i) {
        density_histogram[i] = 0;
    }

    auto update_particles = [&input_data, &data_count]
        (float *px, float *py, float *pz, float *pp, float *pt, float *pw, int count, uint32_t gx, uint32_t gy, uint32_t gz, float wx, float wy, float wz, float cx, float cy, float cz, float mw)
    {
        for (int i = 0; i < count; ++i) {

            // These are the data points, read from input
            if (i < data_count) {
                int start_index = int(4*i);

                float x = input_data[start_index];
                float y = input_data[start_index + 1];
                float z = input_data[start_index + 2];
                float weight = input_data[start_index + 3];

                px[i] = world_to_grid(x, wx, cx, float(gx));
                py[i] = world_to_grid(y, wy, cy, float(gy));
                pz[i] = world_to_grid(z, wz, cz, float(gz));
                
                if (mw > 0.0)
                    pw[i] = (1.0e6 / float(data_count)) * (weight / mw);
                else
                    pw[i] = weight;

                pt[i] = -5.0; // Marker value for input data
                pp[i] = 0.0;
            }

            // These are free-flowing physarum agents
            else {
                #ifdef AGENTS_INIT_AROUND_DATA // Initialize the agents around data points to speed up convergence
                int random_data_index = (int)random::uniform(0.0, (float)(data_count-1));
                const float random_spread = 0.025;
                float radius = random_spread * math::min(math::min(gx, gy), gz) * random::uniform();
                float xi1 = random::uniform();
                float xi2 = random::uniform();
                px[i] = px[random_data_index] + radius * math::cos(math::PI2 * xi1) * math::sqrt(xi2 * (1.0-xi2));
                py[i] = py[random_data_index] + radius * math::sin(math::PI2 * xi1) * math::sqrt(xi2 * (1.0-xi2));
                pz[i] = pz[random_data_index] + 0.5 * radius * (1.0 - 2.0*xi2);
                #endif
                #ifdef AGENTS_INIT_RANDOMLY
                px[i] = random::uniform(0.0, (float)gx);
                py[i] = random::uniform(0.0, (float)gy);
                pz[i] = random::uniform(0.0, (float)gz);
                #endif
                pp[i] = random::uniform(0.0, math::PI2);
                pt[i] = math::acos(2.0 * random::uniform(0.0, 1.0) - 1.0);
                pw[i] = 1.0;
            }

        }
    };
    update_particles(particles_x, particles_y, particles_z, particles_phi, particles_theta, particles_weights,
                    NUM_PARTICLES, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z, WORLD_CENTER_X, WORLD_CENTER_Y, WORLD_CENTER_Z, mean_weight);

    // Set up buffer containing particle data
    StructuredBuffer particles_buffer_x = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_x, particles_x);
    StructuredBuffer particles_buffer_y = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_y, particles_y);
    StructuredBuffer particles_buffer_z = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_z, particles_z);
    StructuredBuffer particles_buffer_phi = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_phi, particles_phi);
    StructuredBuffer particles_buffer_theta = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_theta, particles_theta);
    StructuredBuffer particles_buffer_weights = graphics::get_structured_buffer(sizeof(float), NUM_PARTICLES);
    graphics::update_structured_buffer(&particles_buffer_weights, particles_weights);
    StructuredBuffer density_histogram_buffer = graphics::get_structured_buffer(sizeof(unsigned int), N_HISTOGRAM_BINS);
    graphics::update_structured_buffer(&density_histogram_buffer, density_histogram);
    StructuredBuffer halos_densities_buffer = graphics::get_structured_buffer(sizeof(float), data_count);
    graphics::update_structured_buffer(&halos_densities_buffer, halos_densities);

    // Set up 3D texture quad mesh.
    float super_quad_vertices_template[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f,

        -1.0f, -1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
    };  

    float *super_quad_vertices = memory::alloc_heap<float>(sizeof(super_quad_vertices_template) / sizeof(float) * GRID_RESOLUTION);
    float z_step = 2.0f / (float)GRID_RESOLUTION;
    for (int z = 0; z < int(GRID_RESOLUTION); z++) {
        float *dst = super_quad_vertices + sizeof(super_quad_vertices_template) / sizeof(float) * z;
        float *src = super_quad_vertices_template;
        memcpy(dst, src, sizeof(super_quad_vertices_template));
        dst[2] = -1.0f + z_step * (0.5 + float(z));
        dst[9] = -1.0f + z_step * (0.5 + float(z));
        dst[16] = -1.0f + z_step * (0.5 + float(z));
        dst[23] = -1.0f + z_step * (0.5 + float(z));
        dst[30] = -1.0f + z_step * (0.5 + float(z));
        dst[37] = -1.0f + z_step * (0.5 + float(z));
        dst[6] =  1.0f - z_step * (0.5 + float(z)) * 0.5f;
        dst[13] = 1.0f - z_step * (0.5 + float(z)) * 0.5f;
        dst[20] = 1.0f - z_step * (0.5 + float(z)) * 0.5f;
        dst[27] = 1.0f - z_step * (0.5 + float(z)) * 0.5f;
        dst[34] = 1.0f - z_step * (0.5 + float(z)) * 0.5f;
        dst[41] = 1.0f - z_step * (0.5 + float(z)) * 0.5f;
    }
    int super_quad_vertices_stride = sizeof(float) * 7;
    Mesh super_quad_mesh = graphics::get_mesh(super_quad_vertices, quad_vertices_count * GRID_RESOLUTION, super_quad_vertices_stride, NULL, 0, 0);
    Mesh quad_mesh = graphics::get_mesh(quad_vertices, quad_vertices_count, quad_vertices_stride, NULL, 0, 0);

    // Set up 3D rendering matrices buffer
    float aspect_ratio = float(window_width) / float(window_height);
    Matrix4x4 projection_matrix;
    if (CAMERA_FOV > 0.0)
        projection_matrix = math::get_perspective_projection_dx_rh(math::deg2rad(CAMERA_FOV), aspect_ratio, 0.01, 10.0);
    else
        projection_matrix = math::get_orthographics_projection_dx_rh(-1.4 * aspect_ratio, 1.4 * aspect_ratio, -1.4, 1.4, 0.01, 10.0);
    float azimuth = 0.0;
    float polar = math::PIHALF;
    float radius = 5.0;
    Vector3 eye_pos = Vector3(
            math::cos(azimuth) * math::sin(polar),
            math::sin(azimuth) * math::sin(polar),
            math::cos(polar)
        ) * radius;
    Matrix4x4 view_matrix = math::get_look_at(eye_pos, Vector3(0,0,0), Vector3(0,0,1));
    Vector3 camera_offset = Vector3(0.0, 0.0, 0.0);
    const float CAM_OFFSET = 0.05;

    // Assign default rendering parameters
    RenderingConfig rendering_config = {};
    rendering_config.projection = projection_matrix;
    rendering_config.view = math::get_identity();
    rendering_config.model = math::get_identity();
    rendering_config.trim_x_min = 0.0;
    rendering_config.trim_x_max = 1.0;
    rendering_config.trim_y_min = 0.0;
    rendering_config.trim_y_max = 1.0;
    rendering_config.trim_z_min = 0.0;
    rendering_config.trim_z_max = 1.0;
    rendering_config.trim_density = 1.0e-5;
    rendering_config.world_width = (float)GRID_RESOLUTION_X;
    rendering_config.world_height = (float)GRID_RESOLUTION_Y;
    rendering_config.world_depth = (float)GRID_RESOLUTION_Z;
    rendering_config.screen_width = (float)window_width;
    rendering_config.screen_height = (float)window_height;
    rendering_config.sample_weight = 0.01;
    rendering_config.optical_thickness = 0.25;
    rendering_config.highlight_density = 10.0;
    rendering_config.galaxy_weight = 0.25;
    rendering_config.histogram_base = HISTOGRAM_BASE;
    rendering_config.overdensity_threshold_low = 1.0;
    rendering_config.overdensity_threshold_high = 10.0;
    rendering_config.camera_x = eye_pos.x;
    rendering_config.camera_y = eye_pos.y;
    rendering_config.camera_z = eye_pos.z;
    rendering_config.pt_iteration = 0;
    rendering_config.sigma_s = 0.0;
    rendering_config.sigma_a = 0.5;
    rendering_config.sigma_e = 4.0;
    rendering_config.trace_max = 100.0;
    rendering_config.camera_offset_x = 0.0;
    rendering_config.camera_offset_y = 0.0;
    rendering_config.exposure = 1.0;
    rendering_config.n_bounces = 15;    // 30
    rendering_config.ambient_trace = 0.0;
    rendering_config.compressive_accumulation = 0;
    rendering_config.guiding_strength = 0.1;
    rendering_config.scattering_anisotropy = 0.5;

    rendering_config.slime_ior = 1.45;
    rendering_config.light_pos = 0;
    rendering_config.sphere_pos = 0;
    rendering_config.shininess = 64;

    rendering_config.aperture = 0;  // default 0.8
    rendering_config.focus_dist = 0.7;

    // Compute sigma_a and sigma_s for each of RGB
    rendering_config.sigma_t_rgb = 0.6;
    rendering_config.albedo_r = 0.85;   // 0.92
    rendering_config.albedo_g = 0.75;   // 0.88
    rendering_config.albedo_b = 0.24;   // 0.05
    rendering_config.some_slider = 0;

    rendering_config.sigma1_a_r = (1 - rendering_config.albedo_r) * rendering_config.sigma_t_rgb;
    rendering_config.sigma1_a_g = (1 - rendering_config.albedo_g) * rendering_config.sigma_t_rgb;
    rendering_config.sigma1_a_b = (1 - rendering_config.albedo_b) * rendering_config.sigma_t_rgb;

    rendering_config.sigma1_s_r = rendering_config.albedo_r * rendering_config.sigma_t_rgb;
    rendering_config.sigma1_s_g = rendering_config.albedo_g * rendering_config.sigma_t_rgb;
    rendering_config.sigma1_s_b = rendering_config.albedo_b * rendering_config.sigma_t_rgb;

    // printf("%f\n",rendering_config.sigma1_a_r);
    // printf("%f\n",rendering_config.sigma1_a_g);
    // printf("%f\n",rendering_config.sigma1_a_b);
    // printf("%f\n",rendering_config.sigma1_s_r);
    // printf("%f\n",rendering_config.sigma1_s_g);
    // printf("%f\n",rendering_config.sigma1_s_b);

    ConstantBuffer rendering_settings_buffer = graphics::get_constant_buffer(sizeof(RenderingConfig));
    graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
    graphics::set_constant_buffer(&rendering_settings_buffer, 4);

    // Assign default simulation parameters
    SimulationConfig simulation_config = {};
    simulation_config.sense_spread = math::deg2rad(SENSE_SPREAD);
    simulation_config.sense_distance = measure_world_to_grid(SENSE_DISTANCE, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
    simulation_config.turn_angle = math::deg2rad(MOVE_ANGLE);
    simulation_config.move_distance = measure_world_to_grid(MOVE_DISTANCE, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
    simulation_config.deposit_value = AGENT_DEPOSIT;
    simulation_config.decay_factor = PERSISTENCE;
    simulation_config.center_attraction = 0.0;
    simulation_config.world_width = int(GRID_RESOLUTION_X);
    simulation_config.world_height = int(GRID_RESOLUTION_Y);
    simulation_config.world_depth = int(GRID_RESOLUTION_Z);
    simulation_config.move_sense_coef = SAMPLING_EXPONENT;
    simulation_config.normalization_factor = 1.0;
    simulation_config.n_data_points = data_count;
    simulation_config.n_agents = NUM_AGENTS;
    simulation_config.n_iteration = 0;
    ConstantBuffer config_buffer = graphics::get_constant_buffer(sizeof(SimulationConfig));

    // Assign default misc parameters
    StatisticsConfig statistics_config = {};
    statistics_config.n_histo_bins = N_HISTOGRAM_BINS;
    statistics_config.n_data_points = data_count;
    statistics_config.histogram_base = HISTOGRAM_BASE;
    statistics_config.sample_randomly = 0;
    statistics_config.world_width = int(GRID_RESOLUTION_X);
    statistics_config.world_height = int(GRID_RESOLUTION_Y);
    statistics_config.world_depth = int(GRID_RESOLUTION_Z);
    ConstantBuffer statistics_config_buffer = graphics::get_constant_buffer(sizeof(StatisticsConfig));

    Timer timer = timer::get();
    timer::start(&timer);

    const int32_t eplot_res = 100;
    float eplot_vals[eplot_res];
    auto reset_eplot = [&eplot_vals, eplot_res]() {
        for (int i = 0; i < eplot_res; ++i)
            eplot_vals[i] = 0.0;
    };
    reset_eplot();

    // Render loop
    bool is_running = true;
    bool is_a = true;
    bool show_ui = true;
    bool run_mold = false;
    bool turning_camera = false;
    bool render_dof = true;
    bool store_deposit = false;
    bool capture_screen = false;
    bool make_screenshot = false;
    bool capture_agents = false;
    bool compute_histogram = true;
    bool run_pt = true;
    bool reset_pt = false;
    bool sort_agents = false;
    float background_color = 0.0;
    VisualizationMode vis_mode = VisualizationMode::VM_PARTICLES;

    bool smooth_trail = true;

    // Update simulation config
    graphics::update_constant_buffer(&config_buffer, &simulation_config);
    graphics::set_constant_buffer(&config_buffer, 0);

    // Decay only trail
    if (smooth_trail) {
        graphics::set_compute_shader(&decay_trail_shader);
        graphics::set_texture_compute(&trace_tex, 0);
        graphics::run_compute(GRID_RESOLUTION_X / 8, GRID_RESOLUTION_Y / 8, GRID_RESOLUTION_Z / 8);
        graphics::unset_texture_compute(0);
    }

    while(is_running)
    {
        static float sec_per_frame_amortized = 0.0;
        sec_per_frame_amortized = 0.9 * sec_per_frame_amortized + 0.1 * timer::checkpoint(&timer);
        std::ostringstream window_title;
        window_title.precision(3);
        window_title << "Polyphorm [ " << 1000.0 * sec_per_frame_amortized << " ms/frame";
        window_title << " | pass " << simulation_config.n_iteration;
        if (vis_mode == VisualizationMode::VM_PATH_TRACING)
            window_title << " | " << rendering_config.pt_iteration << " spp";
        window_title << " ]";
        platform::set_window_title(window, window_title.str().c_str());
        
        // Event loop
        input::reset();
        Event event;
        while(platform::get_event(&event)) {
            input::register_event(&event);
            switch(event.type) {
                case EventType::EXIT:
                    is_running = false;
                break;
            }
        }

        // React to inputs
        {
            if(!ui::is_registering_input()) {
                if (math::abs(input::mouse_scroll_delta()) > 0)
                    reset_pt = true;
                radius = math::max(radius - input::mouse_scroll_delta() * 0.1, 0.01);
                if (input::mouse_left_button_down()) {
                    Vector2 dm = input::mouse_delta_position();
                    azimuth -= dm.x * 0.003;
                    polar -= dm.y * 0.003;
                    polar = math::clamp(polar, 0.01f, math::PI-0.01f);
                    reset_pt |= (math::abs(dm.x) > 0 || math::abs(dm.y) > 0);
                }
                if (input::mouse_right_button_down()) {
                    Vector2 dm = input::mouse_delta_position();
                    camera_offset.x += radius * 0.03 * CAM_OFFSET * dm.x;
                    camera_offset.y -= radius * 0.03 * CAM_OFFSET * dm.y;
                    reset_pt |= (math::abs(dm.x) > 0 || math::abs(dm.y) > 0);
                }
            }
            if (turning_camera) {
                azimuth += 0.01;
                reset_pt = true;
            }
            eye_pos = Vector3(
                math::cos(azimuth) * math::sin(polar),
                math::sin(azimuth) * math::sin(polar),
                math::cos(polar)
            ) * radius;
            rendering_config.view = math::get_translation(camera_offset) * math::get_look_at(eye_pos, Vector3(0,0,0), Vector3(0,0,1));
            rendering_config.camera_x = eye_pos.x;
            rendering_config.camera_y = eye_pos.y;
            rendering_config.camera_z = eye_pos.z;
            rendering_config.camera_offset_x = camera_offset.x;
            rendering_config.camera_offset_y = camera_offset.y;
            // if (CAMERA_FOV <= 0.0)
                // rendering_config.projection = math::get_orthographics_projection_dx_rh(-0.28 * radius * aspect_ratio, 0.28 * radius * aspect_ratio, -0.28 * radius, 0.28 * radius, 0.01, 10.0);

            if (input::key_pressed(KeyCode::ESC)) is_running = false; 
            if (input::key_pressed(KeyCode::F1)) show_ui = !show_ui; 
            if (input::key_pressed(KeyCode::F2)) { // Reset particles + trails
                update_particles(particles_x, particles_y, particles_z, particles_phi, particles_theta, particles_weights,
                                NUM_PARTICLES, GRID_RESOLUTION_X, GRID_RESOLUTION_Y, GRID_RESOLUTION_Z, WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z, WORLD_CENTER_X, WORLD_CENTER_Y, WORLD_CENTER_Z, mean_weight);
                graphics::update_structured_buffer(&particles_buffer_x, particles_x);
                graphics::update_structured_buffer(&particles_buffer_y, particles_y);
                graphics::update_structured_buffer(&particles_buffer_z, particles_z);
                graphics::update_structured_buffer(&particles_buffer_phi, particles_phi);
                graphics::update_structured_buffer(&particles_buffer_theta, particles_theta);
                graphics::update_structured_buffer(&particles_buffer_weights, particles_weights);
                float clear_tex[4] = {0, 0, 0, 0};
                graphics_context->context->ClearUnorderedAccessViewFloat(trail_tex_A.ua_view, clear_tex);
                graphics_context->context->ClearUnorderedAccessViewFloat(trail_tex_B.ua_view, clear_tex);
                graphics_context->context->ClearUnorderedAccessViewFloat(trace_tex.ua_view, clear_tex);
                reset_eplot();
                simulation_config.n_iteration = 0;
            }
            if (input::key_pressed(KeyCode::F3)) run_mold = !run_mold;
            if (input::key_pressed(KeyCode::F4)) turning_camera = !turning_camera;
            if (input::key_pressed(KeyCode::F5)) capture_agents = !capture_agents;
            if (input::key_pressed(KeyCode::F6)) store_deposit = true;
            if (input::key_pressed(KeyCode::F7)) capture_screen = !capture_screen;
            if (input::key_pressed(KeyCode::F8)) { // Reset only trace
                float clear_trace[4] = {0, 0, 0, 0};
                graphics_context->context->ClearUnorderedAccessViewFloat(trace_tex.ua_view, clear_trace);
            }
            if (input::key_pressed(KeyCode::F9)) {
                std::fstream visu_state;
                visu_state.open("visu_state.tmp", std::fstream::out);
                visu_state << polar << " " << azimuth <<  " " << radius << std::endl;
                visu_state << camera_offset.x << " " << camera_offset.y << " " << camera_offset.z <<std::endl;
                visu_state << rendering_config.trim_x_min << " " << rendering_config.trim_x_max << " "
                    << rendering_config.trim_y_min << " " << rendering_config.trim_y_max << " "
                    << rendering_config.trim_z_min << " " << rendering_config.trim_z_max << std::endl;
                visu_state << rendering_config.sample_weight << " " << rendering_config.galaxy_weight << " "
                    << rendering_config.optical_thickness << " " << rendering_config.optical_thickness << " "
                    << rendering_config.trim_density << " " << rendering_config.highlight_density << " "
                    << rendering_config.overdensity_threshold_low << " " << rendering_config.overdensity_threshold_high << std::endl;
                visu_state << rendering_config.sigma_s << " " << rendering_config.sigma_a << " " << rendering_config.sigma_e << " "
                    << rendering_config.scattering_anisotropy << " "
                    << rendering_config.exposure << " " << rendering_config.trace_max << " " << rendering_config.n_bounces << " "
                    << rendering_config.ambient_trace << " " << rendering_config.compressive_accumulation << std::endl;
                visu_state.close();
            }
            if (input::key_pressed(KeyCode::F10)) {
                std::fstream visu_state;
                visu_state.open("visu_state.tmp", std::fstream::in);
                visu_state >> polar >> azimuth >> radius;
                visu_state >> camera_offset.x >> camera_offset.y >> camera_offset.z;
                visu_state >> rendering_config.trim_x_min >> rendering_config.trim_x_max
                    >> rendering_config.trim_y_min >> rendering_config.trim_y_max
                    >> rendering_config.trim_z_min >> rendering_config.trim_z_max;
                visu_state >> rendering_config.sample_weight >> rendering_config.galaxy_weight
                    >> rendering_config.optical_thickness >> rendering_config.optical_thickness
                    >> rendering_config.trim_density >> rendering_config.highlight_density
                    >> rendering_config.overdensity_threshold_low >> rendering_config.overdensity_threshold_high;
                visu_state >> rendering_config.sigma_s >> rendering_config.sigma_a >> rendering_config.sigma_e
                    >> rendering_config.scattering_anisotropy
                    >> rendering_config.exposure >> rendering_config.trace_max >> rendering_config.n_bounces
                    >> rendering_config.ambient_trace >> rendering_config.compressive_accumulation;
                visu_state.close();
                reset_pt = true;
            }
            if (input::key_pressed(KeyCode::NUM1)) {
                make_screenshot = true;
            }
        }

        // Update simulation config
        graphics::update_constant_buffer(&config_buffer, &simulation_config);
        graphics::set_constant_buffer(&config_buffer, 0);

        // Particle simulation 
        if (run_mold)
        {
            is_a = !is_a;
            graphics::set_compute_shader(&compute_shader);
            if (is_a) {
                graphics::set_texture_compute(&trail_tex_A, 0);
            } else {
                graphics::set_texture_compute(&trail_tex_B, 0);
            }
            graphics::set_texture_compute(&trace_tex, 1);
            graphics::set_structured_buffer(&particles_buffer_x, 2);
            graphics::set_structured_buffer(&particles_buffer_y, 3);
            graphics::set_structured_buffer(&particles_buffer_z, 4);
            graphics::set_structured_buffer(&particles_buffer_phi, 5);
            graphics::set_structured_buffer(&particles_buffer_theta, 6);
            graphics::set_structured_buffer(&particles_buffer_weights, 7);
            int32_t grid_z = (NUM_PARTICLES / 100) / THREAD_GROUP_SIZE;
            graphics::run_compute(10, 10, grid_z);
            graphics::unset_texture_compute(0);
            graphics::unset_texture_compute(1);
        }

        // Partial agent sorting
        if (run_mold && sort_agents)
        {
            graphics::set_compute_shader(&sort_shader);
            graphics::set_structured_buffer(&particles_buffer_x, 2);
            graphics::set_structured_buffer(&particles_buffer_y, 3);
            graphics::set_structured_buffer(&particles_buffer_z, 4);
            graphics::set_structured_buffer(&particles_buffer_phi, 5);
            graphics::set_structured_buffer(&particles_buffer_theta, 6);
            graphics::set_structured_buffer(&particles_buffer_weights, 7);
            int32_t grid_z = (NUM_PARTICLES / 100) / THREAD_GROUP_SIZE;
            // different attempts at addressing
            for (int i = 0; i < 256; ++i) {
                graphics::run_compute(10, 10, grid_z / 256);
                ++simulation_config.n_iteration;
                graphics::update_constant_buffer(&config_buffer, &simulation_config);
            }
        }

        // Decay/diffusion
        if (run_mold) {
            graphics::set_compute_shader(&decay_compute_shader);
            if (is_a) {
                graphics::set_texture_compute(&trail_tex_A, 0);
                graphics::set_texture_compute(&trail_tex_B, 1);
            } else {
                graphics::set_texture_compute(&trail_tex_B, 0);
                graphics::set_texture_compute(&trail_tex_A, 1);
            }
            graphics::set_texture_compute(&trace_tex, 2);
            graphics::run_compute(GRID_RESOLUTION_X / 8, GRID_RESOLUTION_Y / 8, GRID_RESOLUTION_Z / 8);
            
            graphics::unset_texture_compute(0);
            graphics::unset_texture_compute(1);
            graphics::unset_texture_compute(2);
        }

        // Compute agent trace histogram
        if (compute_histogram)
        {
            for (int i = 0; i < N_HISTOGRAM_BINS; ++i) {
                density_histogram[i] = 0;
            }
            graphics::update_structured_buffer(&density_histogram_buffer, density_histogram);
            graphics::update_constant_buffer(&statistics_config_buffer, &statistics_config);

            graphics::set_compute_shader(&cs_density_histo);
            graphics::set_constant_buffer(&statistics_config_buffer, 0);
            graphics::set_texture_compute(&trace_tex, 0);
            graphics::set_structured_buffer(&density_histogram_buffer, 1);
            graphics::set_structured_buffer(&particles_buffer_x, 2);
            graphics::set_structured_buffer(&particles_buffer_y, 3);
            graphics::set_structured_buffer(&particles_buffer_z, 4);
            graphics::set_structured_buffer(&particles_buffer_weights, 5);
            graphics::set_structured_buffer(&halos_densities_buffer, 6);

            int32_t grid_z = (NUM_PARTICLES / 100) / THREAD_GROUP_SIZE;
            graphics::run_compute(10, 10, grid_z);

            graphics::unset_texture_compute(0);
        }

        // Export the current state of the simulation
        if (store_deposit)
        {
            printf("Exporting simulation data...\n");
            store_deposit = false;
            if (is_a)
                graphics::save_texture3D(&trail_tex_A, "export/deposit");
            else
                graphics::save_texture3D(&trail_tex_B, "export/deposit");
            graphics::save_texture3D(&trace_tex, "export/trace");

            std::ofstream metadata;
            metadata.open("export/export_metadata.txt", std::ofstream::out);
            metadata << "dataset: " << DATASET_NAME << std::endl;
            metadata << "number of data points: " << data_count << std::endl;
            metadata << "number of agents: " << int(NUM_AGENTS) / 1e6 << "M" << std::endl;
            metadata << "simulation grid resolution: " << int(GRID_RESOLUTION_X) << " x " << int(GRID_RESOLUTION_Y) << " x " << int(GRID_RESOLUTION_Z) << " [vox]" << std::endl;
            metadata << "simulation grid size: " << WORLD_SIZE_X << " x " << WORLD_SIZE_Y << " x " << WORLD_SIZE_Z << " [mpc]" << std::endl;
            metadata << "simulation grid center: (" << WORLD_CENTER_X << ", " << WORLD_CENTER_Y << ", " << WORLD_CENTER_Z << ") [mpc]" << std::endl;
            metadata << std::endl;
            metadata << "move distance: " << measure_grid_to_world(simulation_config.move_distance, WORLD_SIZE_X, float(GRID_RESOLUTION_X)) << " [mpc]" << std::endl;
            metadata << "move distance grid: " << simulation_config.move_distance << " [vox]" << std::endl;
            metadata << "sense distance: " << measure_grid_to_world(simulation_config.sense_distance, WORLD_SIZE_X, float(GRID_RESOLUTION_X)) << " [mpc]" << std::endl;
            metadata << "sense distance grid: " << simulation_config.sense_distance << " [vox]" << std::endl;
            metadata << "move spread: " << math::rad2deg(simulation_config.turn_angle) << " [deg]" << std::endl;
            metadata << "sense spread: " << math::rad2deg(simulation_config.sense_spread) << " [deg]" << std::endl;
            metadata << "persistence coefficient: " << simulation_config.decay_factor << std::endl;
            metadata << "agent deposit: " << simulation_config.deposit_value << std::endl;
            metadata << "sampling sharpness: " << simulation_config.move_sense_coef << std::endl;
            metadata.close();

            graphics::capture_structured_buffer(&halos_densities_buffer, halos_densities, data_count, sizeof(float));
            std::ofstream halos_measurements;
            halos_measurements.open("export/halos_measurements.csv", std::ofstream::out);
            halos_measurements.precision(5);
            halos_measurements << "M200b/10^12 | Trace | X (world) | Y (world) | Z (world) | X (grid) | Y (grid) | Z (grid) " << std::endl;
            for (int i = 0; i < data_count; ++i) {
                halos_measurements
                    << particles_weights[i] << ","
                    << halos_densities[i] << ","
                    << grid_to_world(particles_x[i], WORLD_SIZE_X, WORLD_CENTER_X, GRID_RESOLUTION_X) << ","
                    << grid_to_world(particles_y[i], WORLD_SIZE_Y, WORLD_CENTER_Y, GRID_RESOLUTION_Y) << ","
                    << grid_to_world(particles_z[i], WORLD_SIZE_Z, WORLD_CENTER_Z, GRID_RESOLUTION_Z) << ","
                    << particles_x[i] << ","
                    << particles_y[i] << ","
                    << particles_z[i] << std::endl;
            }
            halos_measurements.close();

            printf("Done exporting simulation data.\n");
        }

        // Capture agents' coordinates
        if (capture_agents)
        {
            static bool first_time_capture = true;
            if (first_time_capture) {
                first_time_capture = false;
                std::ofstream agents;
                agents.open("export/agents.txt", std::ofstream::out);
                agents.close();
            }

            static int timestep_counter = 0;
            if (timestep_counter >= N_AGENT_TIMESTEPS_TO_CAPTURE) {
                capture_agents = false;
                timestep_counter = 0;
                printf("Done exporting agents.\n");
            } else {
                printf("Exporting agents, timestep %d/%d...\n", timestep_counter+1, N_AGENT_TIMESTEPS_TO_CAPTURE);
                graphics::capture_structured_buffer(&particles_buffer_x, particles_x, NUM_PARTICLES, sizeof(float));
                graphics::capture_structured_buffer(&particles_buffer_y, particles_y, NUM_PARTICLES, sizeof(float));
                graphics::capture_structured_buffer(&particles_buffer_z, particles_z, NUM_PARTICLES, sizeof(float));
                graphics::capture_structured_buffer(&particles_buffer_weights, particles_weights, NUM_PARTICLES, sizeof(float));
                std::ofstream agents;
                agents.open("export/agents.txt", std::ofstream::out | std::ofstream::app);
                agents << "*** timestep " << timestep_counter << " [X Y Z D] ***" << std::endl;
                agents.precision(7);
                for (int i = data_count; i < int(NUM_PARTICLES); ++i) {
                    agents 
                        << measure_grid_to_world(particles_x[i], WORLD_SIZE_X, float(GRID_RESOLUTION_X)) << " "
                        << measure_grid_to_world(particles_y[i], WORLD_SIZE_Y, float(GRID_RESOLUTION_Y)) << " "
                        << measure_grid_to_world(particles_z[i], WORLD_SIZE_Z, float(GRID_RESOLUTION_Z)) << " "
                        << particles_weights[i] << std::endl;
                }
                agents.close();
                ++timestep_counter;
            }
        }

        // Rendering
        {
            graphics::set_render_targets_viewport(&render_target_window);
            graphics::clear_render_target(&render_target_window, background_color, background_color, background_color, 1.0);
            
            if(vis_mode == VisualizationMode::VM_PARTICLES) {
                uint32_t clear_tex_uint[4] = {0, 0, 0, 0};
                graphics_context->context->ClearUnorderedAccessViewUint(display_tex_uint.ua_view, clear_tex_uint);
                graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
                graphics::set_compute_shader(&draw_compute_shader_particle);
                graphics::set_structured_buffer(&particles_buffer_theta, 6);
                graphics::set_texture_compute(&display_tex_uint, 0);
                graphics::set_structured_buffer(&particles_buffer_x, 2);
                graphics::set_structured_buffer(&particles_buffer_y, 3);
                graphics::set_structured_buffer(&particles_buffer_z, 4);

                int32_t grid_z = (NUM_PARTICLES / 100) / THREAD_GROUP_SIZE;
                graphics::run_compute(10, 10, grid_z);
                graphics::unset_texture_compute(0);

                graphics::set_compute_shader(&blit_compute_shader);
                graphics::set_texture_compute(&display_tex_uint, 0);
                graphics::set_texture_compute(&display_tex, 1);
                graphics::run_compute(window_width, window_height, 1);
                graphics::unset_texture_compute(0);
                graphics::unset_texture_compute(1);

                graphics::set_vertex_shader(&vertex_shader_2d);
                graphics::set_pixel_shader(&pixel_shader_2d);
                graphics::set_texture(&display_tex, 0);
                graphics::set_texture_sampler(&tex_sampler_display, 0);
                graphics::draw_mesh(&quad_mesh);
                graphics::unset_texture(0);
            }
            else if (vis_mode == VisualizationMode::VM_VOLUME
                  || vis_mode == VisualizationMode::VM_VOLUME_HIGHLIGHT
                  || vis_mode == VisualizationMode::VM_VOLUME_OVERDENSITY
                  || vis_mode == VisualizationMode::VM_VOLUME_VELOCITY) {
                graphics::set_vertex_shader(&vertex_shader);
                graphics::set_texture(&trace_tex, 0);
                graphics::set_texture_sampler(&tex_sampler_trace, 0);
                if (vis_mode == VisualizationMode::VM_VOLUME) {
                    graphics::set_pixel_shader(&pixel_shader);
                    graphics::set_texture(&palette_trace_tex, 1);
                    graphics::set_texture_sampler(&tex_sampler_color_palette, 1);
                }
                else if (vis_mode == VisualizationMode::VM_VOLUME_HIGHLIGHT) {
                    graphics::set_pixel_shader(&ps_volume_highlight);
                    if (is_a) {
                        graphics::set_texture(&trail_tex_A, 1);
                    } else {
                        graphics::set_texture(&trail_tex_B, 1);
                    }
                    graphics::set_texture_sampler(&tex_sampler_deposit, 1);
                }
                else if (vis_mode == VisualizationMode::VM_VOLUME_OVERDENSITY) {
                    graphics::set_pixel_shader(&ps_volume_overdensity);
                }
                else if (vis_mode == VisualizationMode::VM_VOLUME_VELOCITY) {
                    graphics::set_pixel_shader(&ps_volume_velocity);
                }

                // Draw the stack most perpendicular to the current origin-relative camera position
                float rotation_angle = 0.0;
                if (math::abs(eye_pos.z) >= math::abs(eye_pos.x) && math::abs(eye_pos.z) >= math::abs(eye_pos.y)) {
                    rotation_angle = (eye_pos.z > 0.0)? 0.0 : math::PI;
                    rendering_config.model = math::get_rotation(rotation_angle, Vector3(0, 1, 0)) * math::get_scale(1.0, WORLD_SIZE_Y / WORLD_SIZE_X, WORLD_SIZE_Z / WORLD_SIZE_X);
                    rendering_config.texcoord_map = (eye_pos.z > 0.0)? 1 : -1;
                    graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
                    graphics::draw_mesh(&super_quad_mesh);
                } else if (math::abs(eye_pos.y) >= math::abs(eye_pos.x) && math::abs(eye_pos.y) >= math::abs(eye_pos.z)) {
                    rotation_angle = (eye_pos.y > 0.0)? -math::PIHALF : math::PIHALF;
                    rendering_config.model = math::get_rotation(rotation_angle, Vector3(1, 0, 0)) * math::get_scale(1.0, WORLD_SIZE_Z / WORLD_SIZE_X, WORLD_SIZE_Y / WORLD_SIZE_X);
                    rendering_config.texcoord_map = (eye_pos.y > 0.0)? 2 : -2;
                    graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
                    graphics::draw_mesh(&super_quad_mesh);
                } else if (math::abs(eye_pos.x) > math::abs(eye_pos.y) && math::abs(eye_pos.x) > math::abs(eye_pos.z)) {
                    rotation_angle = (eye_pos.x > 0.0)? math::PIHALF : -math::PIHALF;
                    rendering_config.model = math::get_rotation(rotation_angle, Vector3(0, 1, 0)) * math::get_scale(WORLD_SIZE_Z / WORLD_SIZE_X, WORLD_SIZE_Y / WORLD_SIZE_X, 1.0);
                    rendering_config.texcoord_map = (eye_pos.x > 0.0)? 3 : -3;
                    graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
                    graphics::draw_mesh(&super_quad_mesh);
                }

                graphics::unset_texture(0);
                graphics::unset_texture(1);
            }
            else if (vis_mode == VisualizationMode::VM_PATH_TRACING) {
                if (reset_pt) {
                    reset_pt = false;
                    rendering_config.pt_iteration = 0;
                }
                graphics::update_constant_buffer(&rendering_settings_buffer, &rendering_config);
                // Nature HDRI Texture
                graphics::set_texture_sampled_compute(&nature_hdri_tex, 5);
                graphics::set_texture_sampler_compute(&tex_sampler_nature_hdri, 5);
                
                if (run_pt && rendering_config.pt_iteration < 1e5) {
                    graphics::set_compute_shader(&cs_volpath);
                    graphics::set_texture_compute(&display_tex, 0);
                    graphics::set_texture_sampled_compute(&trace_tex, 1);
                    graphics::set_texture_sampler_compute(&tex_sampler_trace, 1);
                    if (is_a) {
                        graphics::set_texture_sampled_compute(&trail_tex_A, 2);
                    } else {
                        graphics::set_texture_sampled_compute(&trail_tex_B, 2);
                    }
                    graphics::set_texture_sampler(&tex_sampler_deposit, 2);
                    graphics::set_texture_sampled_compute(&palette_trace_tex, 3);
                    graphics::set_texture_sampler_compute(&tex_sampler_color_palette, 3);
                    graphics::set_texture_sampled_compute(&palette_data_tex, 4);
                    graphics::set_texture_sampler_compute(&tex_sampler_color_palette, 4);


                    graphics::run_compute(
                        rendering_config.screen_width / int(PT_GROUP_SIZE_X),
                        rendering_config.screen_height / int(PT_GROUP_SIZE_Y),
                        1);
                    graphics::unset_texture_compute(0);
                    graphics::unset_texture_sampled_compute(1);
                    graphics::unset_texture_sampled_compute(2);
                    graphics::unset_texture_sampled_compute(3);
                    rendering_config.pt_iteration++;
                }

                graphics::set_vertex_shader(&vertex_shader_2d);
                graphics::set_pixel_shader(&ps_volpath);
                graphics::set_texture(&display_tex, 0);
                graphics::set_texture_sampler(&tex_sampler_display, 0);
                graphics::draw_mesh(&quad_mesh);
                graphics::unset_texture(0);
                graphics::unset_texture(5);
            }
        }

        // Process and draw density histogram
        if (compute_histogram)
        {
            // Get histogram statistics
            graphics::capture_structured_buffer(&density_histogram_buffer, density_histogram, N_HISTOGRAM_BINS, sizeof(unsigned int));
            float norm_coef = float(density_histogram[0]);
            float energy = 0.0;
            for (int b = 1; b < N_HISTOGRAM_BINS-1; b++) {
                norm_coef += float(density_histogram[b]);
                energy += float(density_histogram[b]) * math::pow(HISTOGRAM_BASE, b-6);
            }
            float mean = energy / norm_coef;
            float variance = float(density_histogram[0]) * math::square(-mean);
            for (int b = 1; b < N_HISTOGRAM_BINS-1; b++) {
                variance += float(density_histogram[b]) * math::square(math::pow(HISTOGRAM_BASE, b-6) - mean);
            }
            variance /= norm_coef;

            // const float smoothing_coef = 0.5;
            // simulation_config.normalization_factor = smoothing_coef * simulation_config.normalization_factor + (1.0 - smoothing_coef) * mean;

            graphics::set_render_targets_viewport(&render_target_window);

            // Draw histogram
                // X start | Y start | bar width | bar gap
            const Vector4 histo_params = Vector4(5.0, float(SCREEN_Y)-20.0, 10.0, 4.0);
                // relative height | total width | void | void
            const Vector4 histo_params2 = Vector4(0.15*float(SCREEN_Y), (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w), 0.0, 0.0);
            const Vector4 label_params = Vector4(8.6, 3.0, 0.0, 0.0);
            const Vector4 label_color = Vector4(0.5, 0.95, 0.55, 0.55);
            std::stringstream histo_label;
            for (int b = 0; b < N_HISTOGRAM_BINS-1; ++b) {
                float current_bar = 0.5 + histo_params2.x * float(density_histogram[b]) / norm_coef;
                Vector4 bar_color = Vector4(0.4, 0.9, 0.5, 0.5);
                if (b == 0)
                    bar_color = Vector4(0.2, 0.2, 0.2, 0.5); // Null bin
                if (vis_mode == VisualizationMode::VM_VOLUME_HIGHLIGHT && b == 6 + math::floor(log(rendering_config.highlight_density) / log(HISTOGRAM_BASE)))
                    bar_color = Vector4(0.9, 0.3, 0.2, 0.5); // Highlighted bin
                ui::draw_rect(
                    histo_params.x + b*(histo_params.z+histo_params.w),
                    histo_params.y - current_bar,
                    histo_params.z,
                    current_bar,
                    bar_color);
                if (b == 0)
                    continue;
                histo_label.str("");
                histo_label << b-6;
                ui::draw_text(histo_label.str().c_str(),
                    Vector2(histo_params.x + b*(histo_params.z+histo_params.w) + 2.0,
                    histo_params.y + 2.0),
                    label_color);
            }

            // Draw energy plot
            static int eplot_ptr = 0;
            static int frame_num = 0;
            float objective_value = mean;
                // Update plot only every several frames
            if (run_mold && frame_num % int(1.0e3 / float(eplot_res)) == 0) {
                eplot_vals[eplot_ptr] = objective_value;
                eplot_ptr = (eplot_ptr + 1) % eplot_res;
            }
            ++frame_num;
            static float max_objective = 0.0;
            if (objective_value > max_objective)
                max_objective = objective_value;
                // bar width | plot height | void | Y offset
            const Vector4 eplot_params = Vector4(histo_params2.y/float(eplot_res), 0.1*float(SCREEN_Y), 0.0, 10.0);
            const Vector4 eplot_color = Vector4(0.95, 0.25, 0.15, 0.65);
            for (int i = 0; i < eplot_res; ++i) {
                float eplot_val = eplot_vals[(eplot_ptr + i) % eplot_res];
                ui::draw_rect(
                    histo_params.x + i*eplot_params.x,
                    histo_params.y - eplot_params.y * eplot_val / max_objective,
                    eplot_params.x,
                    2.0,
                    eplot_color);
            }
            ui::draw_text("E",
                Vector2(histo_params.x + (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w) + histo_params.w,
                histo_params.y - label_params.x - eplot_params.y * eplot_vals[(eplot_ptr-1) % eplot_res] / max_objective),
                eplot_color);

            // Draw labels
            int label_counter = 0;
            histo_label.precision(4);
            histo_label.str("");
            histo_label << "E: " << mean;
            ui::draw_text(histo_label.str().c_str(),
                Vector2(histo_params.x + (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w) + histo_params.w,
                histo_params.y - (label_params.x-label_counter*label_params.y) * histo_params.w),
                label_color);
            ++label_counter;
            histo_label.str("");
            // histo_label << "V: " << variance;
            histo_label << "M: " << float(density_histogram[N_HISTOGRAM_BINS-1]) / 1.e5;
            ui::draw_text(histo_label.str().c_str(),
                Vector2(histo_params.x + (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w) + histo_params.w,
                histo_params.y - (label_params.x-label_counter*label_params.y) * histo_params.w),
                label_color);
            ++label_counter;
            histo_label.precision(2);
            histo_label.str("");
            histo_label << "null: " << 100.0 * float(density_histogram[0]) / norm_coef << "%";
            ui::draw_text(histo_label.str().c_str(),
                Vector2(histo_params.x + (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w) + histo_params.w,
                histo_params.y - (label_params.x-label_counter*label_params.y) * histo_params.w),
                label_color);
            ++label_counter;
            histo_label.str("");
            histo_label << "(log " << HISTOGRAM_BASE << ")";
            ui::draw_text(histo_label.str().c_str(),
                Vector2(histo_params.x + (N_HISTOGRAM_BINS-1)*(histo_params.z+histo_params.w) + histo_params.w,
                histo_params.y - (label_params.x-label_counter*label_params.y) * histo_params.w),
                label_color);
            ++label_counter;

            // Draw trimming visualization
            float slice_wz = math::max(rendering_config.trim_z_max - rendering_config.trim_z_min, 0.01);
            const Vector4 trim_params = Vector4(float(SCREEN_X)-10.0, float(SCREEN_Y)-10.0, 0.08 * float(SCREEN_Y), 0.0);
            const Vector4 cube_color = Vector4(0.5, 0.95, 0.5, 0.1 * slice_wz);
            const Vector4 slice_color1 = Vector4(0.55, 0.75, 0.0, 0.3 * slice_wz);
            const Vector4 slice_color2 = Vector4(0.0, 0.75, 0.95, 0.4 * slice_wz);
            ui::draw_rect(
                trim_params.x - trim_params.z,
                trim_params.y - trim_params.z,
                trim_params.z,
                trim_params.z,
                cube_color);
            float slice_px = 1.0 - (rendering_config.trim_x_max + rendering_config.trim_x_min) / 2.0;
            float slice_wx = math::max(rendering_config.trim_x_max - rendering_config.trim_x_min, 0.025);
            float slice_py = 1.0 - (rendering_config.trim_y_max + rendering_config.trim_y_min) / 2.0;
            float slice_wy = math::max(rendering_config.trim_y_max - rendering_config.trim_y_min, 0.025);
            if (slice_wy > slice_wx) {
                ui::draw_rect(trim_params.x - math::min(slice_py + 0.5*slice_wy, 1.0) * trim_params.z, trim_params.y - trim_params.z, trim_params.z * (slice_wy - math::max(math::max(1.0-slice_py, slice_py) + 0.5*slice_wy - 1.0, 0.0)), trim_params.z, slice_color1);
                ui::draw_rect(trim_params.x - trim_params.z, trim_params.y - math::min(slice_px + 0.5*slice_wx, 1.0) * trim_params.z, trim_params.z, trim_params.z * (slice_wx - math::max(math::max(1.0-slice_px, slice_px) + 0.5*slice_wx - 1.0, 0.0)), slice_color2);
            } else {
                ui::draw_rect(trim_params.x - trim_params.z, trim_params.y - math::min(slice_px + 0.5*slice_wx, 1.0) * trim_params.z, trim_params.z, trim_params.z * (slice_wx - math::max(math::max(1.0-slice_px, slice_px) + 0.5*slice_wx - 1.0, 0.0)), slice_color2);
                ui::draw_rect(trim_params.x - math::min(slice_py + 0.5*slice_wy, 1.0) * trim_params.z, trim_params.y - trim_params.z, trim_params.z * (slice_wy - math::max(math::max(1.0-slice_py, slice_py) + 0.5*slice_wy - 1.0, 0.0)), trim_params.z, slice_color1);
            }
            ui::draw_text("Y", Vector2(trim_params.x - 0.52 * trim_params.z, trim_params.y - trim_params.z - 15.0), label_color);
            ui::draw_text("X", Vector2(trim_params.x - trim_params.z - 10.0, trim_params.y - 0.52 * trim_params.z), label_color);

            ui::end();
        }

        // Frame capturing
        if (is_running && make_screenshot) {
            graphics::capture_current_frame();
            make_screenshot = false;
        }
        if (is_running && capture_screen) {
            graphics::capture_current_frame();
        }

        // UI
        if (show_ui) {
            graphics::set_render_targets_viewport(&render_target_window);

            Panel panel = ui::start_panel("", Vector2(0.0, 0.0), 1.0);
            const float smoothing_coef = 0.1;

            float ss = math::rad2deg(simulation_config.sense_spread);
            reset_pt |= ui::add_slider(&panel, "SENSE ANGLE [DEG]", &ss, 0.0, 90.0);
            simulation_config.sense_spread = math::deg2rad(ss);
            float sd_mpc = measure_grid_to_world(simulation_config.sense_distance, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
            reset_pt |= ui::add_slider(&panel, "SENSE DIST [MPC]", &sd_mpc, 0.0, 10.0);
            simulation_config.sense_distance = measure_world_to_grid(sd_mpc, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
            float ts = math::rad2deg(simulation_config.turn_angle);
            reset_pt |= ui::add_slider(&panel, "MOVE ANGLE [DEG]", &ts, 0.0, 45.0);
            simulation_config.turn_angle = math::deg2rad(ts);
            float md_mpc = measure_grid_to_world(simulation_config.move_distance, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
            reset_pt |= ui::add_slider(&panel, "MOVE DIST [MPC]", &md_mpc, 0.0, 1.0);
            simulation_config.move_distance = measure_world_to_grid(md_mpc, WORLD_SIZE_X, float(GRID_RESOLUTION_X));
            reset_pt |= ui::add_slider(&panel, "AGENT DEPOSIT", &simulation_config.deposit_value, 0.0, 10.0);
            reset_pt |= ui::add_slider(&panel, "PERSISTENCE", &simulation_config.decay_factor, 0.8, 0.995);
            // reset_pt |= ui::add_slider(&panel, "CENTER ATTRACTION", &simulation_config.center_attraction, 0.0, 10.0);
            reset_pt |= ui::add_slider(&panel, "SAMPLING EXP", &simulation_config.move_sense_coef, 0.0001, 10.0);

            float swgt = log(rendering_config.sample_weight) / log(10.0);
            reset_pt |= ui::add_slider(&panel, "TRACE WEIGHT", &swgt, -5.0, 3.0);
            rendering_config.sample_weight = math::pow(10.0, swgt);
            reset_pt |= ui::add_slider(&panel, "DEPOSIT WEIGHT", &rendering_config.galaxy_weight, 0.0, 1.0);

            // ui::add_toggle(&panel, "AGENT SORTING", &sort_agents);
            ui::add_toggle(&panel, "TRACE HISTOGRAM", &compute_histogram);
            static bool random_histogram_sampling = false;
            ui::add_toggle(&panel, "HIST RNG SAMPLING", &random_histogram_sampling);
            statistics_config.sample_randomly = random_histogram_sampling ? 1 : 0;
            static bool do_trimming = false;
            ui::add_toggle(&panel, "VOLUME TRIMMING", &do_trimming);
            if (do_trimming) {
                float trim_pos = (rendering_config.trim_x_max + rendering_config.trim_x_min) / 2.0;
                float trim_width = rendering_config.trim_x_max - rendering_config.trim_x_min;
                reset_pt |= ui::add_slider(&panel, "X POS", &trim_pos, 0.0, 1.0);
                reset_pt |=ui::add_slider(&panel, "X WIDTH", &trim_width, 0.0, 1.0);
                rendering_config.trim_x_min = smoothing_coef * rendering_config.trim_x_min + (1.0-smoothing_coef) * (trim_pos - 0.5 * trim_width);
                rendering_config.trim_x_max = smoothing_coef * rendering_config.trim_x_max + (1.0-smoothing_coef) * (trim_pos + 0.5 * trim_width);
                trim_pos = (rendering_config.trim_y_max + rendering_config.trim_y_min) / 2.0;
                trim_width = rendering_config.trim_y_max - rendering_config.trim_y_min;
                reset_pt |= ui::add_slider(&panel, "Y POS", &trim_pos, 0.0, 1.0);
                reset_pt |=ui::add_slider(&panel, "Y WIDTH", &trim_width, 0.0, 1.0);
                rendering_config.trim_y_min = smoothing_coef * rendering_config.trim_y_min + (1.0-smoothing_coef) * (trim_pos - 0.5 * trim_width);
                rendering_config.trim_y_max = smoothing_coef * rendering_config.trim_y_max + (1.0-smoothing_coef) * (trim_pos + 0.5 * trim_width);
                trim_pos = (rendering_config.trim_z_max + rendering_config.trim_z_min) / 2.0;
                trim_width = rendering_config.trim_z_max - rendering_config.trim_z_min;
                reset_pt |= ui::add_slider(&panel, "Z POS", &trim_pos, 0.0, 1.0);
                reset_pt |=ui::add_slider(&panel, "Z WIDTH", &trim_width, 0.0, 1.0);
                rendering_config.trim_z_min = smoothing_coef * rendering_config.trim_z_min + (1.0-smoothing_coef) * (trim_pos - 0.5 * trim_width);
                rendering_config.trim_z_max = smoothing_coef * rendering_config.trim_z_max + (1.0-smoothing_coef) * (trim_pos + 0.5 * trim_width);
            }

            bool is_toggled;
            is_toggled = vis_mode == VisualizationMode::VM_VOLUME;
            ui::add_toggle(&panel, "VIS: TRACE", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_VOLUME : vis_mode;
            if (vis_mode == VisualizationMode::VM_VOLUME) {
                ui::add_slider(&panel, "OPTI THICKNESS", &rendering_config.optical_thickness, 0.0, 1.0);
                float trd = log(rendering_config.trim_density) / log(HISTOGRAM_BASE);
                reset_pt |= ui::add_slider(&panel, "TRIM DENSITY", &trd, -5.0, 9.0);
                rendering_config.trim_density = math::pow(HISTOGRAM_BASE, trd);
                ui::add_slider(&panel, "BACKGROUND COL", &background_color, 0.0, 1.0);
            }

            is_toggled = vis_mode == VisualizationMode::VM_VOLUME_HIGHLIGHT;
            ui::add_toggle(&panel, "VIS: HIGHLIGHTS", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_VOLUME_HIGHLIGHT : vis_mode;
            if (vis_mode == VisualizationMode::VM_VOLUME_HIGHLIGHT) {
                ui::add_slider(&panel, "OPTI THICKNESS", &rendering_config.optical_thickness, 0.0, 1.0);
                float hgd = log(rendering_config.highlight_density) / log(HISTOGRAM_BASE);
                ui::add_slider(&panel, "HGLGHT DENSITY", &hgd, -5.0, 9.0);
                rendering_config.highlight_density = math::pow(HISTOGRAM_BASE, hgd);
                float trd = log(rendering_config.trim_density) / log(HISTOGRAM_BASE);
                reset_pt |= ui::add_slider(&panel, "TRIM DENSITY", &trd, -5.0, 9.0);
                rendering_config.trim_density = math::pow(HISTOGRAM_BASE, trd);
                ui::add_slider(&panel, "BACKGROUND COL", &background_color, 0.0, 1.0);
            }

            is_toggled = vis_mode == VisualizationMode::VM_VOLUME_OVERDENSITY;
            ui::add_toggle(&panel, "VIS: OVERDENSITY", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_VOLUME_OVERDENSITY : vis_mode;
            if (vis_mode == VisualizationMode::VM_VOLUME_OVERDENSITY) {
                ui::add_slider(&panel, "OPTI THICKNESS", &rendering_config.optical_thickness, 0.0, 1.0);
                float trd = log(rendering_config.trim_density) / log(HISTOGRAM_BASE);
                reset_pt |= ui::add_slider(&panel, "TRIM DENSITY", &trd, -5.0, 9.0);
                rendering_config.trim_density = math::pow(HISTOGRAM_BASE, trd);
                float odt_low = log(rendering_config.overdensity_threshold_low) / log(HISTOGRAM_BASE);
                float odt_high = log(rendering_config.overdensity_threshold_high) / log(HISTOGRAM_BASE);
                ui::add_slider(&panel, "OVERDENSITY LO", &odt_low, -5.0, odt_high);
                ui::add_slider(&panel, "OVERDENSITY HI", &odt_high, odt_low, 9.0);
                rendering_config.overdensity_threshold_low = math::pow(HISTOGRAM_BASE, odt_low);
                rendering_config.overdensity_threshold_high = math::pow(HISTOGRAM_BASE, odt_high);
            }

            #ifdef VELOCITY_ANALYSIS
            is_toggled = vis_mode == VisualizationMode::VM_VOLUME_VELOCITY;
            ui::add_toggle(&panel, "VIS: VELOCITY", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_VOLUME_VELOCITY : vis_mode;
            if (vis_mode == VisualizationMode::VM_VOLUME_VELOCITY) {
                ui::add_slider(&panel, "OPTI THICKNESS", &rendering_config.optical_thickness, 0.0, 1.0);
                float trd = log(rendering_config.trim_density) / log(HISTOGRAM_BASE);
                reset_pt |= ui::add_slider(&panel, "TRIM DENSITY", &trd, -5.0, 9.0);
                rendering_config.trim_density = math::pow(HISTOGRAM_BASE, trd);
                ui::add_slider(&panel, "BACKGROUND COL", &background_color, 0.0, 1.0);
            }
            #endif

            is_toggled = vis_mode == VisualizationMode::VM_PARTICLES;
            ui::add_toggle(&panel, "VIS: PARTICLES", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_PARTICLES : vis_mode;

            is_toggled = vis_mode == VisualizationMode::VM_PATH_TRACING;
            ui::add_toggle(&panel, "VIS: PATH TRACING", &is_toggled);
            vis_mode = is_toggled? VisualizationMode::VM_PATH_TRACING : vis_mode;
            if (vis_mode == VisualizationMode::VM_PATH_TRACING) {
                float sigma_t = rendering_config.sigma_a + rendering_config.sigma_s;
                float albedo = sigma_t < 1.e-5 ? 0.0 : rendering_config.sigma_s / sigma_t;
                reset_pt |= ui::add_slider(&panel, "SIGMA_T", &sigma_t, 0.0, 5.0);
                reset_pt |= ui::add_slider(&panel, "ALBEDO", &albedo, 0.0, 0.99);
                rendering_config.sigma_a = (1.0 - albedo) * sigma_t;
                rendering_config.sigma_s = albedo * sigma_t;
                reset_pt |= ui::add_slider(&panel, "SIGMA_E", &rendering_config.sigma_e, 0.0, 100.0);
                reset_pt |= ui::add_slider(&panel, "ANISOTROPY", &rendering_config.scattering_anisotropy, 0.1, 0.98);
                reset_pt |= ui::add_slider(&panel, "AMBI TRCE", &rendering_config.ambient_trace, 0.0, 1.0);

                float f_bounces = float(rendering_config.n_bounces);
                reset_pt |= ui::add_slider(&panel, "N BOUNCES", &f_bounces, 0.0, 30.0);
                rendering_config.n_bounces = int(f_bounces);
                float trmax = log(rendering_config.trace_max) / log(HISTOGRAM_BASE);
                reset_pt |= ui::add_slider(&panel, "TRACE_MAX", &trmax, -4.0, 4.0);
                rendering_config.trace_max = math::pow(HISTOGRAM_BASE, trmax);
                // reset_pt |= ui::add_slider(&panel, "GUIDING MAG", &rendering_config.guiding_strength, 0.0, 0.6);

                // Sphere Position Offset
                float sphere_pos = rendering_config.sphere_pos;
                reset_pt |= ui::add_slider(&panel, "Sphere Position", &sphere_pos, -1000.0, 1000.0);
                rendering_config.sphere_pos = sphere_pos;
                
                // Point Light Position Offset
                float light_pos = rendering_config.light_pos;
                reset_pt |= ui::add_slider(&panel, "Light Position", &light_pos, -1000.0, 1000.0);
                rendering_config.light_pos = light_pos;

                // Shininess Term
                float shininess = rendering_config.shininess;
                reset_pt |= ui::add_slider(&panel, "shininess", &shininess, 1.0, 2000.0);
                rendering_config.shininess = math::floor(shininess);

                // focus distance
                float focus_dist = rendering_config.focus_dist;
                reset_pt |= ui::add_slider(&panel, "focus_dist", &focus_dist, 0.0, 1.0);
                rendering_config.focus_dist = focus_dist;

                // Some slider for debug
                float some_slider = rendering_config.some_slider;
                reset_pt |= ui::add_slider(&panel, "some_slider", &some_slider, 0.0, 1.0);
                rendering_config.some_slider = some_slider;

                float expo = log(rendering_config.exposure) / log(10.0);
                if (bool(rendering_config.compressive_accumulation))
                    reset_pt |= ui::add_slider(&panel, "EXPOSURE", &expo, -5.0, 5.0);
                else
                    ui::add_slider(&panel, "EXPOSURE", &expo, -5.0, 5.0);
                rendering_config.exposure = math::pow(10.0, expo);

                bool compress_L = bool(rendering_config.compressive_accumulation);
                reset_pt |= ui::add_toggle(&panel, "COMPRESSIVE EXPOSURE", &compress_L);
                rendering_config.compressive_accumulation = int(compress_L);
                ui::add_toggle(&panel, "RENDER!", &run_pt);
            }

            ui::end_panel(&panel);
            ui::end();
        }

        if (run_mold) {
            ++simulation_config.n_iteration;
        }
        graphics::swap_frames();
    }

    ui::release();
    graphics::release(&render_target_window);
    graphics::release(&depth_buffer);
    graphics::release(&pixel_shader);
    graphics::release(&ps_volume_overdensity);
    graphics::release(&ps_volume_highlight);
    graphics::release(&vertex_shader);
    graphics::release(&pixel_shader_2d);
    graphics::release(&vertex_shader_2d);
    graphics::release(&draw_compute_shader_particle);
    graphics::release(&blit_compute_shader);
    graphics::release(&compute_shader);
    graphics::release(&sort_shader);
    graphics::release(&decay_compute_shader);
    graphics::release(&decay_trail_shader);
    graphics::release(&cs_density_histo);
    graphics::release(&quad_mesh);
    graphics::release(&super_quad_mesh);
    graphics::release(&trail_tex_A);
    graphics::release(&trail_tex_B);
    graphics::release(&trace_tex);
    graphics::release(&display_tex);
    graphics::release(&display_tex_uint);
    graphics::release(&palette_trace_tex);
    graphics::release(&palette_data_tex);
    graphics::release(&tex_sampler_trace);
    graphics::release(&tex_sampler_deposit);
    graphics::release(&tex_sampler_color_palette);
    graphics::release(&config_buffer);
    graphics::release(&particles_buffer_x);
    graphics::release(&particles_buffer_y);
    graphics::release(&particles_buffer_z);
    graphics::release(&particles_buffer_phi);
    graphics::release(&particles_buffer_theta);
    graphics::release(&particles_buffer_weights);
    graphics::release(&density_histogram_buffer);
    graphics::release(&halos_densities_buffer);
    graphics::release(&rendering_settings_buffer);
    graphics::release();

    printf("\n\n");
    return 0;
}