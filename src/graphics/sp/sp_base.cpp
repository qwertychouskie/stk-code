//  SuperTuxKart - a fun racing game with go-kart
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2017 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "graphics/sp/sp_base.hpp"
#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/shader_based_renderer.hpp"
#include "graphics/shared_gpu_objects.hpp"
#include "graphics/shadow_matrices.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/shaders.hpp"
#include "graphics/stk_tex_manager.hpp"
#include "graphics/sp/sp_instanced_data.hpp"
#include "graphics/sp/sp_per_object_uniform.hpp"
#include "graphics/sp/sp_mesh.hpp"
#include "graphics/sp/sp_mesh_buffer.hpp"
#include "graphics/sp/sp_mesh_node.hpp"
#include "graphics/sp/sp_shader.hpp"
#include "graphics/sp/sp_uniform_assigner.hpp"
#include "tracks/track.hpp"
#include "modes/profile_world.hpp"
#include "utils/log.hpp"
#include "utils/profiler.hpp"
#include "utils/string_utils.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SP
{

// ----------------------------------------------------------------------------
ShadowMatrices* g_stk_sm = NULL;
// ----------------------------------------------------------------------------
bool sp_culling = true;
// ----------------------------------------------------------------------------
bool g_handle_shadow = false;
// ----------------------------------------------------------------------------
bool g_handle_rsm = false;
// ----------------------------------------------------------------------------
bool g_destroying_sectors = false;

// ----------------------------------------------------------------------------
std::unordered_map<std::string, SPShader*> g_shaders;
// ----------------------------------------------------------------------------
SPShader* g_glow_shader = NULL;
// ----------------------------------------------------------------------------
std::unordered_map<SPMeshBuffer*, std::vector<SPMeshNode*> > g_instances;
// ----------------------------------------------------------------------------
// std::string is layer_1 and layer_2 texture name combined
typedef std::unordered_map<SPShader*, std::unordered_map<std::string,
    std::unordered_set<SPMeshBuffer*> > > DrawCall;

DrawCall g_draw_calls[DCT_COUNT];
// ----------------------------------------------------------------------------
std::array<GLuint, ST_COUNT> g_samplers;
// ----------------------------------------------------------------------------
std::vector<GLuint> sp_prefilled_tex (5);
// ----------------------------------------------------------------------------
GLuint g_skinning_buffer;
// ----------------------------------------------------------------------------
std::vector<float> g_bounding_boxes;
// ----------------------------------------------------------------------------
core::vector3df g_wind_dir;
// ----------------------------------------------------------------------------
//std::unordered_set<SPMeshNode*> g_all_nodes;
// ----------------------------------------------------------------------------
//std::unordered_set<SPDynamicDrawCall*> g_dy_dc;
// ----------------------------------------------------------------------------
float g_frustums[5][24] = { { } };
// ----------------------------------------------------------------------------
unsigned sp_solid_poly_count = 0;
// ----------------------------------------------------------------------------
unsigned sp_shadow_poly_count = 0;
// ----------------------------------------------------------------------------
unsigned sp_draw_call_count = 0;
// ----------------------------------------------------------------------------
//std::function<void(SPMeshBuffer*)> sp_mb_upload_cb = NULL;
// ----------------------------------------------------------------------------
unsigned g_skinning_offset = 0;
// ----------------------------------------------------------------------------
std::vector<SPMeshNode*> g_skinning_mesh;
// ----------------------------------------------------------------------------
bool sp_vc_srgb_cor = true;
// ----------------------------------------------------------------------------
int sp_cur_shadow_cascade = 0;
// ----------------------------------------------------------------------------
bool sp_null_device = false;
// ----------------------------------------------------------------------------
void initSTKShadowMatrices(ShadowMatrices* sm)
{
    g_stk_sm = sm;
}   // initSTKShadowMatrices
// ----------------------------------------------------------------------------
GLuint sp_mat_ubo = 0;
// ----------------------------------------------------------------------------
GLuint sp_fog_ubo = 0;
// ----------------------------------------------------------------------------
bool destroyingSectors()
{
    return g_destroying_sectors;
}   // destroyingSectors

#ifndef SERVER_ONLY
// ----------------------------------------------------------------------------
void shadowCascadeUniformAssigner(SPUniformAssigner* ua)
{
    ua->setValue(sp_cur_shadow_cascade);
}   // shadowCascadeUniformAssigner

// ----------------------------------------------------------------------------
void rsmUniformAssigner(SPUniformAssigner* ua)
{
    ua->setValue(g_stk_sm->getRSMMatrix());
}   // rsmUniformAssigner

// ----------------------------------------------------------------------------
void fogUniformAssigner(SPUniformAssigner* ua)
{
    int fog_enable = Track::getCurrentTrack() ?
        Track::getCurrentTrack()->isFogEnabled() ? 1 : 0 : 0;
    ua->setValue(fog_enable);
}   // fogUniformAssigner

// ----------------------------------------------------------------------------
void alphaBlendUse()
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}   // alphaBlendUse

// ----------------------------------------------------------------------------
void additiveUse()
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
}   // additiveUse

// ----------------------------------------------------------------------------
void ghostKartUse()
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}   // ghostKartUse
#endif

// ----------------------------------------------------------------------------
void addShader(SPShader* shader)
{
    g_shaders[shader->getName()] = shader;
}   // addShader


// ----------------------------------------------------------------------------
void loadShaders()
{
    // ========================================================================
    SPShader* shader = new SPShader("solid");

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_object_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    shader = new SPShader("solid_skinned");

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_object_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    // ========================================================================
    shader = new SPShader("alphatest");

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_alpha_test_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    shader = new SPShader("alphatest_skinned");

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_alpha_test_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    // ========================================================================
    shader = new SPShader("unlit");

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_unlit.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    addShader(shader);

    shader = new SPShader("unlit_skinned");

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_unlit.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    // ========================================================================
    shader = new SPShader("normalmap");

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_normal_map.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);

    addShader(shader);

    shader = new SPShader("normalmap_skinned");

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_normal_map.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addAllTextures(RP_1ST);

    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addAllTextures(RP_2ND); 

    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addAllTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);


    addShader(shader);

}   // loadShaders

// ----------------------------------------------------------------------------
void init()
{
    if (ProfileWorld::isNoGraphics())
    {
        return;
    }
#ifndef SERVER_ONLY

    sp_prefilled_tex[4] = SharedGPUObjects::getSkinningTexture();

    glGenBuffers(1, &sp_mat_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, sp_mat_ubo);
    glBufferData(GL_UNIFORM_BUFFER, (16 * 9 + 2) * sizeof(float), NULL,
        GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, sp_mat_ubo);

    glGenBuffers(1, &sp_fog_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, sp_fog_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 8 * sizeof(float), NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, sp_fog_ubo);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    for (unsigned st = ST_NEAREST; st < ST_COUNT; st++)
    {
        if ((SamplerType)st == ST_TEXTURE_BUFFER)
        {
            g_samplers[ST_TEXTURE_BUFFER] = 0;
            continue;
        }
        switch ((SamplerType)st)
        {
            case ST_NEAREST:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
                g_samplers[ST_NEAREST] = id;
                break;
            }
            case ST_NEAREST_CLAMPED:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
                g_samplers[ST_NEAREST_CLAMPED] = id;
                break;
            }
            case ST_TRILINEAR:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                {
                    int aniso = UserConfigParams::m_anisotropic;
                    if (aniso == 0) aniso = 1;
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        (float)aniso);
                }
                g_samplers[ST_TRILINEAR] = id;
                break;
            }
            case ST_TRILINEAR_CLAMPED:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                {
                    int aniso = UserConfigParams::m_anisotropic;
                    if (aniso == 0) aniso = 1;
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        (float)aniso);
                }
                g_samplers[ST_TRILINEAR_CLAMPED] = id;
                break;
            }
            case ST_BILINEAR:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
                g_samplers[ST_BILINEAR] = id;
                break;
            }
            case ST_BILINEAR_CLAMPED:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
                g_samplers[ST_BILINEAR_CLAMPED] = id;
                break;
            }
            case ST_SEMI_TRILINEAR:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_NEAREST);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                if (CVS->isEXTTextureFilterAnisotropicUsable())
                    glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.);
                g_samplers[ST_SEMI_TRILINEAR] = id;
                break;
            }
            case ST_SHADOW:
            {
                unsigned id;
                glGenSamplers(1, &id);
                glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glSamplerParameterf(id, GL_TEXTURE_COMPARE_MODE,
                    GL_COMPARE_REF_TO_TEXTURE);
                glSamplerParameterf(id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                g_samplers[ST_SHADOW] = id;
                break;
            }
            default:
                break;
        }
    }
    loadShaders();

#endif
}   // init

// ----------------------------------------------------------------------------
void destroy()
{
    if (sp_null_device)
    {
        return;
    }

    for (auto& p : g_shaders)
    {
        delete p.second;
    }
    g_shaders.clear();
#ifndef SERVER_ONLY
    glDeleteBuffers(1, &sp_mat_ubo);
    glDeleteBuffers(1, &sp_fog_ubo);
    glDeleteSamplers((unsigned)g_samplers.size() - 1, g_samplers.data());
#endif
}   // destroy

// ----------------------------------------------------------------------------
GLuint getSampler(SamplerType st)
{
    assert(st < ST_COUNT);
    return g_samplers[st];
}   // getSampler

// ----------------------------------------------------------------------------
SPShader* getGlowShader()
{
    return g_glow_shader;
}   // getGlowShader

// ----------------------------------------------------------------------------
SPShader* getSPShader(const std::string& name)
{
    auto ret = g_shaders.find(name);
    if (ret != g_shaders.end())
    {
        return ret->second;
    }
    return NULL;
}   // getSPShader

// ----------------------------------------------------------------------------
inline void mathPlaneNormf(float *p)
{
    float f = 1.0f / sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
    p[0] *= f;
    p[1] *= f;
    p[2] *= f;
    p[3] *= f;
}   // mathPlaneNormf

// ----------------------------------------------------------------------------
inline void mathPlaneFrustumf(float* out, const core::matrix4& pvm)
{
    // return 6 planes, 24 floats
    const float* m = pvm.pointer();

    // near
    out[0] = m[3] + m[2];
    out[1] = m[7] + m[6];
    out[2] = m[11] + m[10];
    out[3] = m[15] + m[14];
    mathPlaneNormf(&out[0]);

    // right
    out[4] = m[3] - m[0];
    out[4 + 1] = m[7] - m[4];
    out[4 + 2] = m[11] - m[8];
    out[4 + 3] = m[15] - m[12];
    mathPlaneNormf(&out[4]);

    // left
    out[2 * 4] = m[3] + m[0];
    out[2 * 4 + 1] = m[7] + m[4];
    out[2 * 4 + 2] = m[11] + m[8];
    out[2 * 4 + 3] = m[15] + m[12];
    mathPlaneNormf(&out[2 * 4]);

    // bottom
    out[3 * 4] = m[3] + m[1];
    out[3 * 4 + 1] = m[7] + m[5];
    out[3 * 4 + 2] = m[11] + m[9];
    out[3 * 4 + 3] = m[15] + m[13];
    mathPlaneNormf(&out[3 * 4]);

    // top
    out[4 * 4] = m[3] - m[1];
    out[4 * 4 + 1] = m[7] - m[5];
    out[4 * 4 + 2] = m[11] - m[9];
    out[4 * 4 + 3] = m[15] - m[13];
    mathPlaneNormf(&out[4 * 4]);

    // far
    out[5 * 4] = m[3] - m[2];
    out[5 * 4 + 1] = m[7] - m[6];
    out[5 * 4 + 2] = m[11] - m[10];
    out[5 * 4 + 3] = m[15] - m[14];
    mathPlaneNormf(&out[5 * 4]);
}   // mathPlaneFrustumf

// ----------------------------------------------------------------------------
inline core::vector3df getCorner(const core::aabbox3df& bbox, unsigned n)
{
    switch (n)
    {
    case 0:
        return irr::core::vector3df(bbox.MinEdge.X, bbox.MinEdge.Y,
        bbox.MinEdge.Z);
    case 1:
        return irr::core::vector3df(bbox.MaxEdge.X, bbox.MinEdge.Y,
        bbox.MinEdge.Z);
    case 2:
        return irr::core::vector3df(bbox.MinEdge.X, bbox.MaxEdge.Y,
        bbox.MinEdge.Z);
    case 3:
        return irr::core::vector3df(bbox.MaxEdge.X, bbox.MaxEdge.Y,
        bbox.MinEdge.Z);
    case 4:
        return irr::core::vector3df(bbox.MinEdge.X, bbox.MinEdge.Y,
        bbox.MaxEdge.Z);
    case 5:
        return irr::core::vector3df(bbox.MaxEdge.X, bbox.MinEdge.Y,
        bbox.MaxEdge.Z);
    case 6:
        return irr::core::vector3df(bbox.MinEdge.X, bbox.MaxEdge.Y,
        bbox.MaxEdge.Z);
    case 7:
        return irr::core::vector3df(bbox.MaxEdge.X, bbox.MaxEdge.Y,
        bbox.MaxEdge.Z);
    default:
        assert(false);
        return irr::core::vector3df(0);
    }
}   // getCorner

// ----------------------------------------------------------------------------
void prepareDrawCalls()
{
    if (!sp_culling)
    {
        return;
    }
    sp_solid_poly_count = sp_shadow_poly_count = sp_draw_call_count = 0;
    // 1st one is identity
    g_skinning_offset = 1;
    g_skinning_mesh.clear();
#ifndef SERVER_ONLY
    mathPlaneFrustumf(g_frustums[0], irr_driver->getProjViewMatrix());
    g_handle_shadow = Track::getCurrentTrack() &&
        Track::getCurrentTrack()->hasShadows() && CVS->isDefferedEnabled() &&
        CVS->isShadowEnabled();
    g_handle_rsm = CVS->isGlobalIlluminationEnabled() &&
        !g_stk_sm->isRSMMapAvail();

    if (g_handle_shadow)
    {
        mathPlaneFrustumf(g_frustums[1], g_stk_sm->getSunOrthoMatrices()[0]);
        mathPlaneFrustumf(g_frustums[2], g_stk_sm->getSunOrthoMatrices()[1]);
        mathPlaneFrustumf(g_frustums[3], g_stk_sm->getSunOrthoMatrices()[2]);
        mathPlaneFrustumf(g_frustums[4], g_stk_sm->getSunOrthoMatrices()[3]);
    }

    g_instances.clear();
    for (auto& p : g_draw_calls)
    {
        p.clear();
    }
#endif
}

// ----------------------------------------------------------------------------
void addObject(SPMeshNode* node)
{
    if (!sp_culling)
    {
        return;
    }
    if (node->getSPM() == NULL)
    {
        return;
    }

    const core::matrix4& model_matrix = node->getAbsoluteTransformation();
    bool added_for_skinning = false;
    for (unsigned m = 0; m < node->getSPM()->getMeshBufferCount(); m++)
    {
        SPMeshBuffer* mb = node->getSPM()->getSPMeshBuffer(m);
        SPShader* shader = node->getShader(m);
        if (shader == NULL)
        {
            continue;
        }
        core::aabbox3df bb = mb->getBoundingBox();
        model_matrix.transformBoxEx(bb);
        std::vector<bool> discard;
        discard.resize((g_handle_shadow ? 6 : 1), false);
        discard[5] = !g_handle_rsm;
        for (int dc_type = 0; dc_type < (g_handle_shadow ? 5 : 1); dc_type++)
        {
            for (int i = 0; i < 24; i += 4)
            {
                bool outside = true;
                for (int j = 0; j < 8; j++)
                {
                    const float dist =
                        getCorner(bb, j).X * g_frustums[dc_type][i] +
                        getCorner(bb, j).Y * g_frustums[dc_type][i + 1] +
                        getCorner(bb, j).Z * g_frustums[dc_type][i + 2] +
                        g_frustums[dc_type][i + 3];
                    outside = outside && dist < 0.0f;
                    if (!outside)
                    {
                        break;
                    }
                }
                if (outside)
                {
                    discard[dc_type] = true;
                    break;
                }
            }
        }
        if (g_handle_shadow ?
            (discard[0] && discard[1] && discard[2] && discard[3] &&
            discard[4] && discard[5]) : discard[0])
        {
            continue;
        }
        if (!added_for_skinning && node->getAnimationState())
        {
            added_for_skinning = true;
            int skinning_offset = g_skinning_offset + node->getTotalJoints();
            if (skinning_offset > int(stk_config->m_max_skinning_bones))
            {
                Log::error("SPBase", "No enough space to render skinned"
                    " mesh %s! Max joints can hold: %d",
                    node->getName(), stk_config->m_max_skinning_bones);
                return;
            }
            node->setSkinningOffset(g_skinning_offset);
            g_skinning_mesh.push_back(node);
            g_skinning_offset = skinning_offset;
        }
        g_instances[mb].push_back(node);
        for (int dc_type = 0; dc_type < (g_handle_shadow ? 6 : 1); dc_type++)
        {
            if (discard[dc_type])
            {
                continue;
            }
            if (dc_type == 0)
            {
                sp_solid_poly_count += mb->getIndexCount() / 3;
            }
            else
            {
                sp_shadow_poly_count += mb->getIndexCount() / 3;
            }
            if (shader->isTransparent())
            {
                // All transparent draw calls go DCT_TRANSPARENT
                if (dc_type == 0)
                {
                    auto& ret = g_draw_calls[DCT_TRANSPARENT][shader];
                    ret[mb->getTextureCompare()].insert(mb);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                auto& ret = g_draw_calls[(DrawCallType)
                    (dc_type == 0 ? dc_type : dc_type + 1)][shader];
                ret[mb->getTextureCompare()].insert(mb);
            }
        }
    }

}

// ----------------------------------------------------------------------------
void updateModelMatrix()
{
    irr_driver->setSkinningJoint(g_skinning_offset - 1);
    for (auto& p : g_instances)
    {
        for (auto& q : p.second)
        {
            p.first->addInstanceData(SPInstancedData
                (q->getAbsoluteTransformation(),
                core::vector2df(0,0), 0, 0, q->getSkinningOffset()));
        }
    }
}

// ----------------------------------------------------------------------------
void uploadAll()
{
#ifndef SERVER_ONLY

#ifdef USE_GLES2
    glBindTexture(GL_TEXTURE_2D, SharedGPUObjects::getSkinningTexture());
    unsigned buffer_offset = 0;
    std::vector<std::array<float, 16> > tmp_buf(stk_config->m_max_skinning_bones);
        //const irr::core::matrix4 m;
        //memcpy(&tmp_buf[0], m.pointer(), 64);
#else
    unsigned buffer_offset = 64;
    glBindBuffer(GL_TEXTURE_BUFFER, SharedGPUObjects::getSkinningBuffer());
#endif


    for (unsigned i = 0; i < g_skinning_mesh.size(); i++)
    {
#ifdef USE_GLES2
//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//glPixelStorei(GL_PACK_ALIGNMENT, 1);
        memcpy(&tmp_buf[buffer_offset], g_skinning_mesh[i]->getSkinningMatrices(),
            g_skinning_mesh[i]->getTotalJoints() << 6);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, buffer_offset, 4,
        //    g_skinning_mesh[i]->getTotalJoints(), GL_RGBA,
        //    GL_FLOAT, g_skinning_mesh[i]->getSkinningMatrices());
        buffer_offset += g_skinning_mesh[i]->getTotalJoints();
        /*for (int j = 0; j < g_skinning_mesh[i]->getTotalJoints(); j++)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, buffer_offset + j, 16,
                1, GL_RGBA,
                GL_FLOAT, g_skinning_mesh[i]->getSkinningMatrices() + j);
        }*/
#else
        glBufferSubData(GL_TEXTURE_BUFFER, buffer_offset,
            g_skinning_mesh[i]->getTotalJoints() << 6,
            g_skinning_mesh[i]->getSkinningMatrices());
        buffer_offset += g_skinning_mesh[i]->getTotalJoints() << 6;
#endif
    }

#ifdef USE_GLES2
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 1, 4,
            1023, GL_RGBA,
            GL_FLOAT, tmp_buf.data());
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4,
    //    stk_config->m_max_skinning_bones, 0, GL_RGBA, GL_FLOAT, tmp_buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
#endif

    for (auto& p : g_instances)
    {
        p.first->uploadInstanceData();
    }
    glBindBuffer(GL_UNIFORM_BUFFER, sp_mat_ubo);
    void* ptr = glMapBufferRange(GL_UNIFORM_BUFFER, 0,
        (16 * 9 + 2) * sizeof(float), GL_MAP_WRITE_BIT |
        GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(ptr, g_stk_sm->getMatricesData(), (16 * 9 + 2) * sizeof(float));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif
}

// ----------------------------------------------------------------------------
void draw(RenderPass rp, DrawCallType dct)
{
#ifndef SERVER_ONLY
    DrawCall* dc = NULL;
    if (dct == DCT_TRANSPARENT)
    {
        dc = &g_draw_calls[DCT_TRANSPARENT];
    }
    else if (dct == DCT_GLOW)
    {
        dc = &g_draw_calls[DCT_GLOW];
    }
    else
    {
        switch (rp)
        {
        case RP_1ST:
        case RP_2ND:
            dc = &g_draw_calls[DCT_NORMAL];
            break;
        case RP_SHADOW:
            if (dct == DCT_SHADOW1)
                dc = &g_draw_calls[DCT_SHADOW1];
            if (dct == DCT_SHADOW2)
                dc = &g_draw_calls[DCT_SHADOW2];
            if (dct == DCT_SHADOW3)
                dc = &g_draw_calls[DCT_SHADOW3];
            if (dct == DCT_SHADOW4)
                dc = &g_draw_calls[DCT_SHADOW4];
            break;
        case RP_RSM:
            assert(CVS->isGlobalIlluminationEnabled() &&
                !g_stk_sm->isRSMMapAvail() && !g_draw_calls[DCT_RSM].empty());
            dc = &g_draw_calls[DCT_RSM];
            break;
        default:
            assert(false);
            break;
        }
    }

    std::stringstream profiler_name;
    profiler_name << "SP::Draw " << dct << " with " << rp;
    PROFILER_PUSH_CPU_MARKER(profiler_name.str().c_str(),
        (uint8_t)(float(dct + rp + 2) / float(DCT_COUNT + RP_COUNT) * 255.0f),
        (uint8_t)(float(dct + 1) / (float)DCT_COUNT * 255.0f) ,
        (uint8_t)(float(rp + 1) / (float)RP_COUNT * 255.0f));
    for (auto& p : *dc)
    {
        p.first->use(rp);
        std::vector<SPUniformAssigner*> shader_uniforms;
        p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
            (p.first), &shader_uniforms, rp);
        p.first->bindPrefilledTextures(rp);
        for (auto& q : p.second)
        {
            /*std::vector<SPUniformAssigner*> material_uniforms;
            if (q.first != NULL)
            {
                p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
                    (q.first), &material_uniforms, rp);
                
            }*/
            if (q.second.empty())
            {
                continue;
            }
            p.first->bindTextures((*q.second.begin())->getMaterial(), rp);
            for (SPMeshBuffer* spmb : q.second)
            {
                /*std::vector<SPUniformAssigner*> draw_call_uniforms;
                p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
                    (draw_call), &draw_call_uniforms, rp);
                sp_draw_call_count++;*/
                spmb->draw();
                /*for (SPUniformAssigner* ua : draw_call_uniforms)
                {
                    ua->reset();
                }*/
            }
            /*for (SPUniformAssigner* ua : material_uniforms)
            {
                ua->reset();
            }*/
        }
        for (SPUniformAssigner* ua : shader_uniforms)
        {
            ua->reset();
        }
        p.first->unuse(rp);
    }
    glBindVertexArray(0);
    PROFILER_POP_CPU_MARKER();
#endif
}   // draw

//-----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
void d()
{
/*    SPShader* shader = new SPShader("solid_skinned");
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_object_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("skinning_offset", typeid(int), RP_2ND);
    shader->addUniform("joint_count", typeid(int), RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("skinning_offset", typeid(int), RP_SHADOW);
    shader->addUniform("joint_count", typeid(int), RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addPrefilledTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("alphatest_skinned");
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag",
        GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_alpha_test_pass2.frag", GL_FRAGMENT_SHADER,
        RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("skinning_offset", typeid(int), RP_2ND);
    shader->addUniform("joint_count", typeid(int), RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("skinning_offset", typeid(int), RP_SHADOW);
    shader->addUniform("joint_count", typeid(int), RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addPrefilledTextures(RP_SHADOW);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("normalmap_skinned");
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_normal_map.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df));
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "normal_map");
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map");
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("skinning_offset", typeid(int), RP_2ND);
    shader->addUniform("joint_count", typeid(int), RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("skinning_offset", typeid(int), RP_SHADOW);
    shader->addUniform("joint_count", typeid(int), RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addPrefilledTextures(RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("unlit_skinned");
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER,
        RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_unlit.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("skinning_offset", typeid(int), RP_2ND);
    shader->addUniform("joint_count", typeid(int), RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_skinning_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("skinning_offset", typeid(int), RP_SHADOW);
    shader->addUniform("joint_count", typeid(int), RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addPrefilledTextures(RP_SHADOW);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("solid");
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_object_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
    shader->addShaderFile("sp_rsm.vert", GL_VERTEX_SHADER, RP_RSM);
    shader->addShaderFile("sp_rsm.frag", GL_FRAGMENT_SHADER, RP_RSM);
    shader->linkShaderFiles(RP_RSM);
    shader->use(RP_RSM);
    shader->addBasicUniforms(RP_RSM);
    shader->addUniform("rsm_matrix", typeid(core::matrix4), RP_RSM);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_RSM);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("rsm_matrix",
        rsmUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("alphatest");
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER,
        RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_alpha_test_pass2.frag", GL_FRAGMENT_SHADER,
        RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
    shader->addShaderFile("sp_rsm.vert", GL_VERTEX_SHADER, RP_RSM);
    shader->addShaderFile("sp_rsm.frag", GL_FRAGMENT_SHADER, RP_RSM);
    shader->linkShaderFiles(RP_RSM);
    shader->use(RP_RSM);
    shader->addBasicUniforms(RP_RSM);
    shader->addUniform("rsm_matrix", typeid(core::matrix4), RP_RSM);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_RSM);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("rsm_matrix",
        rsmUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("normalmap");
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_normal_map.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "normal_map", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_object_pass2.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
    shader->addShaderFile("sp_rsm.vert", GL_VERTEX_SHADER, RP_RSM);
    shader->addShaderFile("sp_rsm.frag", GL_FRAGMENT_SHADER, RP_RSM);
    shader->linkShaderFiles(RP_RSM);
    shader->use(RP_RSM);
    shader->addBasicUniforms(RP_RSM);
    shader->addUniform("rsm_matrix", typeid(core::matrix4), RP_RSM);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_RSM);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("rsm_matrix",
        rsmUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("unlit");
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER,
        RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_unlit.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
    shader->addShaderFile("sp_rsm.vert", GL_VERTEX_SHADER, RP_RSM);
    shader->addShaderFile("sp_rsm.frag", GL_FRAGMENT_SHADER, RP_RSM);
    shader->linkShaderFiles(RP_RSM);
    shader->use(RP_RSM);
    shader->addBasicUniforms(RP_RSM);
    shader->addUniform("rsm_matrix", typeid(core::matrix4), RP_RSM);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_RSM);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("rsm_matrix",
        rsmUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("decal");
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_object_pass1.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_decal.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_two_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
    shader->addShaderFile("sp_rsm.vert", GL_VERTEX_SHADER, RP_RSM);
    shader->addShaderFile("sp_rsm.frag", GL_FRAGMENT_SHADER, RP_RSM);
    shader->linkShaderFiles(RP_RSM);
    shader->use(RP_RSM);
    shader->addBasicUniforms(RP_RSM);
    shader->addUniform("rsm_matrix", typeid(core::matrix4), RP_RSM);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_RSM);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("rsm_matrix",
        rsmUniformAssigner);
#endif
    g_shaders.push_back(shader);

    shader = new SPShader("grass");
    shader->addShaderFile("sp_grass_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_alpha_test_pass1.frag", GL_FRAGMENT_SHADER,
        RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("windDir", typeid(core::vector3df), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_1ST);
    shader->addShaderFile("sp_grass_pass.vert", GL_VERTEX_SHADER, RP_2ND);
    shader->addShaderFile("sp_grass.frag", GL_FRAGMENT_SHADER, RP_2ND);
    shader->linkShaderFiles(RP_2ND);
    shader->use(RP_2ND);
    shader->addBasicUniforms(RP_2ND);
    shader->addUniform("windDir", typeid(core::vector3df), RP_2ND);
    shader->addPrefilledTextures(RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "gloss_map", RP_2ND);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "colorization_mask", RP_2ND);
#ifndef USE_GLES2
    shader->addShaderFile("sp_grass_shadow.vert", GL_VERTEX_SHADER, RP_SHADOW);
    if (!CVS->isAMDVertexShaderLayerUsable())
    {
        shader->addShaderFile("sp_shadow.geom", GL_GEOMETRY_SHADER, RP_SHADOW);
    }
    shader->addShaderFile("sp_shadow_alpha_test.frag", GL_FRAGMENT_SHADER, RP_SHADOW);
    shader->linkShaderFiles(RP_SHADOW);
    shader->use(RP_SHADOW);
    shader->addBasicUniforms(RP_SHADOW);
    shader->addUniform("windDir", typeid(core::vector3df), RP_SHADOW);
    shader->addUniform("layer", typeid(int), RP_SHADOW);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_SHADOW);
    static_cast<SPPerObjectUniform*>(shader)->addAssignerFunction("layer",
        shadowCascadeUniformAssigner);
#endif
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("windDir", [](SPUniformAssigner* ua)
        {
            ua->setValue(g_wind_dir);
        });
    g_shaders.push_back(shader);

    shader = new SPShader("alphablend_skinned", 1, true);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(alphaBlendUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    shader = new SPShader("alphablend", 1, true);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(alphaBlendUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    shader = new SPShader("additive_skinned", 1, true);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(additiveUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    shader = new SPShader("additive", 1, true);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(additiveUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    shader = new SPShader("ghost_kart", 1, true);
    shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(ghostKartUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    shader = new SPShader("ghost_kart_skinned", 1, true);
    shader->addShaderFile("sp_skinning.vert", GL_VERTEX_SHADER, RP_1ST);
    shader->addShaderFile("sp_transparent.frag", GL_FRAGMENT_SHADER, RP_1ST);
    shader->linkShaderFiles(RP_1ST);
    shader->use(RP_1ST);
    shader->addBasicUniforms(RP_1ST);
    shader->addUniform("texture_trans", typeid(core::vector2df), RP_1ST);
    shader->addUniform("skinning_offset", typeid(int), RP_1ST);
    shader->addUniform("joint_count", typeid(int), RP_1ST);
    shader->addUniform("fog_enabled", typeid(int), RP_1ST);
    shader->addUniform("custom_alpha", typeid(float), RP_1ST);
    shader->addPrefilledTextures(RP_1ST);
    shader->addTexture(ST_TRILINEAR, GL_TEXTURE_2D, "layer_one_tex", RP_1ST);
    shader->setUseFunction(ghostKartUse);
    static_cast<SPPerObjectUniform*>(shader)
        ->addAssignerFunction("fog_enabled", fogUniformAssigner);
    g_shaders.push_back(shader);

    g_glow_shader = new SPShader("sp_glow_shader", 1);
    g_glow_shader->addShaderFile("sp_pass.vert", GL_VERTEX_SHADER, RP_1ST);
    g_glow_shader->addShaderFile("sp_glow_object.frag", GL_FRAGMENT_SHADER,
        RP_1ST);
    g_glow_shader->linkShaderFiles(RP_1ST);
    g_glow_shader->use(RP_1ST);
    g_glow_shader->addBasicUniforms(RP_1ST);
    g_shaders.push_back(g_glow_shader);*/
}

/*
// ----------------------------------------------------------------------------
void prepareDrawCalls(const scene::ICameraSceneNode* cam, bool shadow)
{
    if (!sp_culling)
    {
        return;
    }
    sp_solid_poly_count = sp_shadow_poly_count = sp_draw_call_count = 0;
    g_skinning_offset = 0;
    g_skinning_mesh.clear();
    for (unsigned i = 0; i < DCT_COUNT; i++)
    {
        g_draw_calls[i].clear();
    }
    if (sp_null_device)
    {
        return;
    }
#ifndef SERVER_ONLY
    int sector_visible = 0;
    mathPlaneFrustumf(g_frustums[0], irr_driver->getProjViewMatrix());
    if (shadow)
    {
        mathPlaneFrustumf(g_frustums[1],
            g_stk_sbr->getShadowMatrices()->getSunOrthoMatrices()[0]);
        mathPlaneFrustumf(g_frustums[2],
            g_stk_sbr->getShadowMatrices()->getSunOrthoMatrices()[1]);
        mathPlaneFrustumf(g_frustums[3],
            g_stk_sbr->getShadowMatrices()->getSunOrthoMatrices()[2]);
        mathPlaneFrustumf(g_frustums[4],
            g_stk_sbr->getShadowMatrices()->getSunOrthoMatrices()[3]);
    }

    std::unordered_map<SPShader*, std::vector<SPDrawCall*> > dc_list[6];
    const bool build_rsm_for_track = CVS->isGlobalIlluminationEnabled() &&
        !g_stk_sbr->getShadowMatrices()->isRSMMapAvail();

    for (SPDynamicDrawCall* dy_dc : g_dy_dc)
    {
        const bool discard =
            dy_dc->handleCulling(g_frustums[0], trueset_depth);
        SPShader* shader = dy_dc->getShader();
        if (!discard)
        {
            dc_list[0][shader].push_back(dy_dc);
        }
        if (shadow && !shader->isTransparent())
        {
            for (int i = 1; i < 5; i++)
            {
                const bool discard = dy_dc->handleCulling(g_frustums[i]);
                if (!discard)
                {
                    dc_list[i][shader].push_back(dy_dc);
                }
            }
        }
    }

    for (unsigned sec_num = 0; sec_num < g_sectors.size(); sec_num++)
    {
        SPSector* sec = g_sectors[sec_num];
        sec->setVisible(false);
        if (sec->getMeshCount() == 0)
        {
            continue;
        }
        if (irr_driver->getBoundingBoxesViz() && sec_num != 0)
        {
            addEdgeForViz(getCorner(sec->getSectorBBox(), 0),
                getCorner(sec->getSectorBBox(), 1));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 1),
                getCorner(sec->getSectorBBox(), 5));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 5),
                getCorner(sec->getSectorBBox(), 4));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 4),
                getCorner(sec->getSectorBBox(), 0));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 2),
                getCorner(sec->getSectorBBox(), 3));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 3),
                getCorner(sec->getSectorBBox(), 7));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 7),
                getCorner(sec->getSectorBBox(), 6));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 6),
                getCorner(sec->getSectorBBox(), 2));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 0),
                getCorner(sec->getSectorBBox(), 2));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 1),
                getCorner(sec->getSectorBBox(), 3));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 5),
                getCorner(sec->getSectorBBox(), 7));
            addEdgeForViz(getCorner(sec->getSectorBBox(), 4),
                getCorner(sec->getSectorBBox(), 6));
        }
        std::vector<bool> discard;
        discard.resize((shadow ? 6 : 1), false);
        discard[5] = !build_rsm_for_track;
        for (int dc_type = 0; dc_type < (shadow ? 5 : 1); dc_type++)
        {
            if (sec_num == 0)
            {
                break;
            }
            for (int i = 0; i < 24; i += 4)
            {
                bool outside = true;
                for (int j = 0; j < 8; j++)
                {
                    const float dist = getCorner
                        (sec->getSectorBBox(), j).X * g_frustums[dc_type][i] +
                        getCorner(sec->getSectorBBox(), j).Y *
                        g_frustums[dc_type][i + 1] +
                        getCorner(sec->getSectorBBox(), j).Z *
                        g_frustums[dc_type][i + 2] + g_frustums[dc_type][i + 3];
                    outside = outside && dist < 0.0f;
                    if (!outside)
                    {
                        break;
                    }
                }
                if (outside)
                {
                    discard[dc_type] = true;
                    break;
                }
            }
        }
        if (shadow ?
            (discard[0] && discard[1] && discard[2] && discard[3] &&
            discard[4] && discard[5]) : discard[0])
        {
            continue;
        }
        sec->setVisible(true);
        sector_visible++;
        auto it = sec->getInstanceIter();
        for (unsigned i = 0; i < sec->getMeshCount(); i++)
        {
            SPInstance* instance = it->second;
            const float depth = instance->calculateDepth(g_frustums[0]);
            if (instance->getInstanceCount() == 0 ||
                instance->discardByAbsoluteDepth())
            {
                it++;
                continue;
            }
            for (int dc_type = 0; dc_type < (shadow ? 6 : 1); dc_type++)
            {
                if (discard[dc_type])
                {
                    continue;
                }
                for (unsigned j = 0; j < instance->getDrawCallCount(); j++)
                {
                    SPDrawCall* dc = instance->getDrawCall(j);
                    SPMesh* spm = it->first;
                    if (dc_type == 0)
                    {
                        dc->setDepth(depth);
                    }
                    else if (dc_type == 5)
                    {
                        // Only render track mesh (space partitioned mesh) for
                        // rsm
                        assert(build_rsm_for_track);
                        if (!spm->isSpacePartitioned())
                        {
                            continue;
                        }
                    }
                    SPShader* dc_shader = dc->getShader();
                    if (dc_shader == NULL || (dc_type != 0 &&
                        dc_shader->isTransparent()))
                    {
                        continue;
                    }
                    if (dc_type == 0)
                    {
                        sp_solid_poly_count += instance->getInstanceCount() *
                            (dc->getIndicesCount() / 3);
                    }
                    else if (dc_type != 5)
                    {
                        sp_shadow_poly_count += instance->getInstanceCount() *
                            (dc->getIndicesCount() / 3);
                    }
                    dc_list[dc_type][dc_shader].push_back(dc);
                }
            }
            for (unsigned j = 0; j < instance->getInstanceCount(); j++)
            {
                SPMeshNode* node = instance->getInstanceNode(j);
                if (node->getChildren().size() > 0)
                {
                    g_stk_sbr->getDrawCalls()->parseSceneManagerSP
                        (node->getChildren(), cam);
                }
                if (node->getSPMesh()->isAnimated())
                {
                    g_skinning_mesh.push_back(node);
                }
            }
            it++;
        }
    }
    for (auto& each_dc : dc_list)
    {
        for (auto& p : each_dc)
        {
            std::sort(p.second.begin(), p.second.end(),
                [](const SPDrawCall* a, const SPDrawCall* b)->bool
                {
                    return a->getMaterial() < b->getMaterial();
                });
        }
    }

    std::vector<SPDrawCall*> glow_dc;
    // Solid
    for (auto& p : dc_list[0])
    {
        std::vector<std::pair<SPMaterial*, std::vector<SPDrawCall*> > > a;
        std::vector<SPDrawCall*> b;
        SPMaterial* cur_mat = p.second.front()->getMaterial();
        for (auto& dc : p.second)
        {
            if (dc->getMaterial() != cur_mat)
            {
                a.emplace_back(cur_mat, std::move(b));
                assert(b.empty());
                cur_mat = dc->getMaterial();
            }
            b.push_back(dc);
        }
        if (!b.empty())
        {
            a.emplace_back(cur_mat, std::move(b));
            assert(b.empty());
        }
        if (UserConfigParams::m_glow && CVS->isDefferedEnabled())
        {
            for (auto& p : a)
            {
                for (auto& q : p.second)
                {
                    if (q->renderGlow())
                    {
                        glow_dc.push_back(q);
                    }
                }
            }
        }
        if (p.first->isTransparent())
        {
            g_draw_calls[DCT_TRANSPARENT].emplace_back(p.first, std::move(a));
        }
        else
        {
            g_draw_calls[DCT_NORMAL].emplace_back(p.first, std::move(a));
        }
        assert(a.empty());
    }

    if (!glow_dc.empty())
    {
        std::sort(glow_dc.begin(), glow_dc.end(),
            [](const SPDrawCall* a, const SPDrawCall* b)->bool
            {
                return a->getDepth() < b->getDepth();
            });
        std::vector<std::pair<SPMaterial*, std::vector<SPDrawCall*> > > glow =
            { std::make_pair((SPMaterial*)NULL, std::move(glow_dc)) };
        assert(glow_dc.empty());
        g_draw_calls[DCT_GLOW].emplace_back(g_glow_shader, std::move(glow));
        assert(glow.empty());
    }

    if (shadow)
    {
        for (int i = 1; i < 6; i++)
        {
            for (auto& p : dc_list[i])
            {
                std::vector<std::pair<SPMaterial*, std::vector<SPDrawCall*> > > a;
                std::vector<SPDrawCall*> b;
                SPMaterial* cur_mat = p.second.front()->getMaterial();
                for (auto& dc : p.second)
                {
                    if (dc->getMaterial() != cur_mat)
                    {
                        a.emplace_back(cur_mat, std::move(b));
                        assert(b.empty());
                        cur_mat = dc->getMaterial();
                    }
                    b.push_back(dc);
                }
                if (!b.empty())
                {
                    a.emplace_back(cur_mat, std::move(b));
                    assert(b.empty());
                }
                g_draw_calls[(DrawCallType)i + 1].emplace_back(p.first,
                    std::move(a));
                assert(a.empty());
            }
        }
    }

    for (auto& p : g_draw_calls[DCT_NORMAL])
    {
        std::sort(p.second.begin(), p.second.end(),
            [](const std::pair<SPMaterial*, std::vector<SPDrawCall*> >& a,
            const std::pair<SPMaterial*, std::vector<SPDrawCall*> >& b)->bool
            {
                return a.second.size() > b.second.size();
            });
        for (auto& q : p.second)
        {
            std::sort(q.second.begin(), q.second.end(),
                [](const SPDrawCall* a, const SPDrawCall* b)->bool
                {
                    return a->getDepth() < b->getDepth();
                });
        }
    }

    for (auto& p : g_draw_calls[DCT_TRANSPARENT])
    {
        std::sort(p.second.begin(), p.second.end(),
            [](const std::pair<SPMaterial*, std::vector<SPDrawCall*> >& a,
            const std::pair<SPMaterial*, std::vector<SPDrawCall*> >& b)->bool
            {
                return a.second.size() > b.second.size();
            });
        for (auto& q : p.second)
        {
            std::sort(q.second.begin(), q.second.end(),
                [](const SPDrawCall* a, const SPDrawCall* b)->bool
                {
                    return a->getDepth() > b->getDepth();
                });
        }
    }
#endif
}   // prepareDrawCalls

// ----------------------------------------------------------------------------
void draw(RenderPass rp, DrawCallType dct)
{
#ifndef SERVER_ONLY
    DrawCall* dc = NULL;
    if (dct == DCT_TRANSPARENT)
    {
        dc = &g_draw_calls[DCT_TRANSPARENT];
    }
    else if (dct == DCT_GLOW)
    {
        dc = &g_draw_calls[DCT_GLOW];
    }
    else
    {
        switch (rp)
        {
        case RP_1ST:
        case RP_2ND:
            dc = &g_draw_calls[DCT_NORMAL];
            break;
        case RP_SHADOW:
            if (dct == DCT_SHADOW1)
                dc = &g_draw_calls[DCT_SHADOW1];
            if (dct == DCT_SHADOW2)
                dc = &g_draw_calls[DCT_SHADOW2];
            if (dct == DCT_SHADOW3)
                dc = &g_draw_calls[DCT_SHADOW3];
            if (dct == DCT_SHADOW4)
                dc = &g_draw_calls[DCT_SHADOW4];
            break;
        case RP_RSM:
            assert(CVS->isGlobalIlluminationEnabled() &&
                !g_stk_sbr->getShadowMatrices()->isRSMMapAvail() &&
                !g_draw_calls[DCT_RSM].empty());
            dc = &g_draw_calls[DCT_RSM];
            break;
        default:
            assert(false);
            break;
        }
    }

    std::stringstream profiler_name;
    profiler_name << "SP::Draw " << dct << " with " << rp;
    PROFILER_PUSH_CPU_MARKER(profiler_name.str().c_str(),
        (uint8_t)(float(dct + rp + 2) / float(DCT_COUNT + RP_COUNT) * 255.0f),
        (uint8_t)(float(dct + 1) / (float)DCT_COUNT * 255.0f) ,
        (uint8_t)(float(rp + 1) / (float)RP_COUNT * 255.0f));
    for (auto& p : *dc)
    {
        if (!p.first->hasShader(rp))
        {
            continue;
        }
        p.first->use(rp);
        std::vector<SPUniformAssigner*> shader_uniforms;
        p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
            (p.first), &shader_uniforms, rp);
        p.first->bindPrefilledTextures(rp);
        for (auto& q : p.second)
        {
            std::vector<SPUniformAssigner*> material_uniforms;
            if (q.first != NULL)
            {
                p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
                    (q.first), &material_uniforms, rp);
                q.first->bindTexturesToShader(p.first, rp);
            }
            for (auto& draw_call : q.second)
            {
                std::vector<SPUniformAssigner*> draw_call_uniforms;
                p.first->setUniformsPerObject(static_cast<SPPerObjectUniform*>
                    (draw_call), &draw_call_uniforms, rp);
                sp_draw_call_count++;
                draw_call->draw();
                for (SPUniformAssigner* ua : draw_call_uniforms)
                {
                    ua->reset();
                }
            }
            for (SPUniformAssigner* ua : material_uniforms)
            {
                ua->reset();
            }
        }
        for (SPUniformAssigner* ua : shader_uniforms)
        {
            ua->reset();
        }
        p.first->unuse(rp);
    }
    glBindVertexArray(0);
    PROFILER_POP_CPU_MARKER();
#endif
}   // draw

//-----------------------------------------------------------------------------
void updateTransformation()
{
    if (!sp_culling)
    {
        return;
    }
#ifndef SERVER_ONLY
#ifndef USE_GLES2
    const GLuint skinning_target = GL_TEXTURE_BUFFER;
#else
    const GLuint skinning_target = GL_UNIFORM_BUFFER;
#endif
#endif
    unsigned total_joint = std::accumulate(g_skinning_mesh.begin(),
        g_skinning_mesh.end(), 0,
        [] (const unsigned previous, const SPMeshNode* n)
        { return previous + n->getSPMesh()->getJointCount(); });
    if (total_joint > 1024)
    {
#ifndef SERVER_ONLY
        Log::error("SP", "Don't have enough space to render all skinned"
            " mesh! Max joints can hold: %d",
            SharedGPUObjects::getMaxMat4Size());
#endif
        total_joint = 0;
    }

    irr_driver->setSkinningJoint(total_joint);
    if (total_joint > 0)
    {
#ifndef SERVER_ONLY
        glBindBuffer(skinning_target, g_skinning_buffer);
        SPMesh* mesh_instance = NULL;
        unsigned cur_sector = 100000000;
        for (unsigned i = 0; i < g_skinning_mesh.size(); i++)
        {
            SPMeshNode* node = g_skinning_mesh[i];
            node->uploadJoints(g_skinning_offset * 16);
            if (node->getSPMesh() != mesh_instance ||
                node->getSector()->getID() != cur_sector)
            {
                node->setSkinningOffset(g_skinning_offset);
                mesh_instance = node->getSPMesh();
                cur_sector = node->getSector()->getID();
            }
            g_skinning_offset += node->getSPMesh()->getJointCount() * 4;
        }
        glBindBuffer(skinning_target, 0);
#endif
    }
    for (SPSector* sec : g_sectors)
    {
        sec->updateInstance();
    }
}   // updateTransformation

// ----------------------------------------------------------------------------
void unsynchronisedUpdate()
{
    for (SPDynamicDrawCall* dy_dc : g_dy_dc)
    {
        dy_dc->update(true);
    }
}   // unsynchronisedUpdate

// ----------------------------------------------------------------------------
void prepareScene()
{
    if (!sp_culling)
    {
        return;
    }
    for (SPDynamicDrawCall* dy_dc : g_dy_dc)
    {
        dy_dc->update();
    }
    for (SPMeshNode* node : g_all_nodes)
    {
        node->OnAnimate(irr_driver->getDevice()->getTimer()->getTime());
    }
    for (SPMeshNode* node : g_all_nodes)
    {
        node->updateSector();
    }
    g_wind_dir = core::vector3df(1.0f, 0.0f, 0.0f) *
        (irr_driver->getDevice()->getTimer()->getTime() / 1000.0f) * 1.5f;
}   // prepareScene

// ----------------------------------------------------------------------------
void drawBoundingBoxes()
{
#ifndef SERVER_ONLY
    Shaders::ColoredLine *line = Shaders::ColoredLine::getInstance();
    line->use();
    line->bindVertexArray();
    line->bindBuffer();
    line->setUniforms(irr::video::SColor(255, 255, 0, 0));
    const float *tmp = g_bounding_boxes.data();
    for (unsigned int i = 0; i < g_bounding_boxes.size(); i += 1024 * 6)
    {
        unsigned count = std::min((unsigned)g_bounding_boxes.size() - i,
            (unsigned)1024 * 6);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(float), &tmp[i]);

        glDrawArrays(GL_LINES, 0, count / 3);
    }
#endif
    g_bounding_boxes.clear();
}   // drawBoundingBoxes

// ----------------------------------------------------------------------------
void addDynamicDrawCall(SPDynamicDrawCall* dy_dc)
{
    g_dy_dc.insert(dy_dc);
}   // addDynamicDrawCall

// ----------------------------------------------------------------------------
void removeDynamicDrawCall(SPDynamicDrawCall* dy_dc)
{
    g_dy_dc.erase(dy_dc);
}   // removeDynamicDrawCall
*/

}
