//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2015 SuperTuxKart-Team
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

#ifndef SERVER_ONLY
#include "graphics/draw_calls.hpp"

#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "graphics/command_buffer.hpp"
#include "graphics/cpu_particle_manager.hpp"
#include "graphics/draw_tools.hpp"
#include "graphics/lod_node.hpp"
#include "graphics/materials.hpp"
#include "graphics/render_info.hpp"
#include "graphics/shadow_matrices.hpp"
#include "graphics/shaders.hpp"
#include "graphics/stk_animated_mesh.hpp"
#include "graphics/stk_mesh.hpp"
#include "graphics/stk_particle.hpp"
#include "graphics/sp/sp_base.hpp"
#include "graphics/sp/sp_mesh_node.hpp"
#include "tracks/track.hpp"
#include "utils/profiler.hpp"

#include <numeric>

// ----------------------------------------------------------------------------
void DrawCalls::clearLists()
{
    ListBlendTransparent::getInstance()->clear();
    ListAdditiveTransparent::getInstance()->clear();
    ListTranslucentSkinned::getInstance()->clear();
    ListTranslucentStandard::getInstance()->clear();
    ListTranslucentTangents::getInstance()->clear();
    ListTranslucent2TCoords::getInstance()->clear();
    ListBlendTransparentFog::getInstance()->clear();
    ListAdditiveTransparentFog::getInstance()->clear();
    ListDisplacement::getInstance()->clear();

    ListSkinnedSolid::getInstance()->clear();
    ListSkinnedAlphaRef::getInstance()->clear();
    ListSkinnedNormalMap::getInstance()->clear();
    ListSkinnedUnlit::getInstance()->clear();
    ListMatDefault::getInstance()->clear();
    ListMatAlphaRef::getInstance()->clear();
    ListMatSphereMap::getInstance()->clear();
    ListMatDetails::getInstance()->clear();
    ListMatUnlit::getInstance()->clear();
    ListMatNormalMap::getInstance()->clear();
    ListMatGrass::getInstance()->clear();
    ListMatSplatting::getInstance()->clear();

    m_immediate_draw_list.clear();
    CPUParticleManager::getInstance()->reset();
}

// ----------------------------------------------------------------------------
bool DrawCalls::isCulledPrecise(const scene::ICameraSceneNode *cam,
                                const scene::ISceneNode* node,
                                bool visualization)
{
    if (!node->getAutomaticCulling() && !visualization)
        return false;

    const core::matrix4 &trans = node->getAbsoluteTransformation();
    core::vector3df edges[8];
    node->getBoundingBox().getEdges(edges);
    for (unsigned i = 0; i < 8; i++)
        trans.transformVect(edges[i]);

    /* From irrlicht
       /3--------/7
      / |       / |
     /  |      /  |
    1---------5   |
    |  /2- - -|- -6
    | /       |  /
    |/        | /
    0---------4/
    */

    if (visualization)
    {
        addEdgeForViz(edges[0], edges[1]);
        addEdgeForViz(edges[1], edges[5]);
        addEdgeForViz(edges[5], edges[4]);
        addEdgeForViz(edges[4], edges[0]);
        addEdgeForViz(edges[2], edges[3]);
        addEdgeForViz(edges[3], edges[7]);
        addEdgeForViz(edges[7], edges[6]);
        addEdgeForViz(edges[6], edges[2]);
        addEdgeForViz(edges[0], edges[2]);
        addEdgeForViz(edges[1], edges[3]);
        addEdgeForViz(edges[5], edges[7]);
        addEdgeForViz(edges[4], edges[6]);
        if (!node->getAutomaticCulling())
        {
            return false;
        }
    }

    const scene::SViewFrustum &frust = *cam->getViewFrustum();
    for (s32 i = 0; i < scene::SViewFrustum::VF_PLANE_COUNT; i++)
    {
        if (isBoxInFrontOfPlane(frust.planes[i], edges))
        {
            return true;
        }
    }
    return false;

}   // isCulledPrecise

// ----------------------------------------------------------------------------
bool DrawCalls::isBoxInFrontOfPlane(const core::plane3df &plane,
                                    const core::vector3df* edges)
{
    for (u32 i = 0; i < 8; i++)
    {
        if (plane.classifyPointRelation(edges[i]) != core::ISREL3D_FRONT)
            return false;
    }
    return true;
}   // isBoxInFrontOfPlane

// ----------------------------------------------------------------------------
void DrawCalls::addEdgeForViz(const core::vector3df &p0,
                              const core::vector3df &p1)
{
    m_bounding_boxes.push_back(p0.X);
    m_bounding_boxes.push_back(p0.Y);
    m_bounding_boxes.push_back(p0.Z);
    m_bounding_boxes.push_back(p1.X);
    m_bounding_boxes.push_back(p1.Y);
    m_bounding_boxes.push_back(p1.Z);
}   // addEdgeForViz

// ----------------------------------------------------------------------------
void DrawCalls::renderBoundingBoxes()
{
    Shaders::ColoredLine *line = Shaders::ColoredLine::getInstance();
    line->use();
    line->bindVertexArray();
    line->bindBuffer();
    line->setUniforms(irr::video::SColor(255, 255, 0, 0));
    const float *tmp = m_bounding_boxes.data();
    for (unsigned int i = 0; i < m_bounding_boxes.size(); i += 1024 * 6)
    {
        unsigned count = std::min((unsigned)m_bounding_boxes.size() - i,
            (unsigned)1024 * 6);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(float), &tmp[i]);

        glDrawArrays(GL_LINES, 0, count / 3);
    }
    m_bounding_boxes.clear();
}   // renderBoundingBoxes

// ----------------------------------------------------------------------------
void DrawCalls::parseSceneManager(core::list<scene::ISceneNode*> &List,
                                  std::vector<scene::ISceneNode *> *ImmediateDraw,
                                  const scene::ICameraSceneNode *cam,
                                  ShadowMatrices& shadow_matrices)
{
    core::list<scene::ISceneNode*>::Iterator I = List.begin(), E = List.end();
    for (; I != E; ++I)
    {
        if (LODNode *node = dynamic_cast<LODNode *>(*I))
            node->updateVisibility();
        (*I)->updateAbsolutePosition();
        if (!(*I)->isVisible())
            continue;

        if (STKParticle *node = dynamic_cast<STKParticle *>(*I))
        {
            if (!isCulledPrecise(cam, *I, irr_driver->getBoundingBoxesViz()))
                CPUParticleManager::getInstance()->addParticleNode(node);
            continue;
        }

        if (scene::IBillboardSceneNode *node =
            dynamic_cast<scene::IBillboardSceneNode *>(*I))
        {
            if (!isCulledPrecise(cam, *I))
                CPUParticleManager::getInstance()->addBillboardNode(node);
            continue;
        }

        SP::SPMeshNode* node = dynamic_cast<SP::SPMeshNode*>(*I);
        if (node)
        {
            SP::addObject(node);
        }
        parseSceneManager((*I)->getChildren(), ImmediateDraw, cam,
            shadow_matrices);
    }
}

// ----------------------------------------------------------------------------
DrawCalls::DrawCalls()
{
    m_sync = 0;
#if !defined(USE_GLES2)
    m_solid_cmd_buffer = NULL;
    m_shadow_cmd_buffer = NULL;
    m_reflective_shadow_map_cmd_buffer = NULL;
    m_glow_cmd_buffer = NULL;

    if(CVS->supportsIndirectInstancingRendering())
    {
        m_solid_cmd_buffer                 = new SolidCommandBuffer();
        m_shadow_cmd_buffer                = new ShadowCommandBuffer();
        m_reflective_shadow_map_cmd_buffer = new ReflectiveShadowMapCommandBuffer();
        m_glow_cmd_buffer                  = new GlowCommandBuffer();        
    }
#endif // !defined(USE_GLES2)
} //DrawCalls

DrawCalls::~DrawCalls()
{
    CPUParticleManager::kill();
    STKParticle::destroyFlipsBuffer();
#if !defined(USE_GLES2)
    delete m_solid_cmd_buffer;
    delete m_shadow_cmd_buffer;
    delete m_reflective_shadow_map_cmd_buffer;
    delete m_glow_cmd_buffer;
#endif // !defined(USE_GLES2)
} //~DrawCalls

// ----------------------------------------------------------------------------
 /** Prepare draw calls before scene rendering
 * \param[out] solid_poly_count Total number of polygons in objects 
 *                              that will be rendered in this frame
 * \param[out] shadow_poly_count Total number of polygons for shadow
 *                               (rendered this frame)
 */
void DrawCalls::prepareDrawCalls( ShadowMatrices& shadow_matrices,
                                  scene::ICameraSceneNode *camnode,
                                  unsigned &solid_poly_count,
                                  unsigned &shadow_poly_count)
{
    m_wind_dir = getWindDir();
    clearLists();
    m_mesh_for_skinning.clear();
    for (unsigned Mat = 0; Mat < Material::SHADERTYPE_COUNT; ++Mat)
    {
        m_solid_pass_mesh[Mat].clear();
        m_reflective_shadow_map_mesh[Mat].clear();
        for (unsigned i = 0; i < 4; i++)
            m_shadow_pass_mesh[i * Material::SHADERTYPE_COUNT + Mat].clear();
    }

    m_glow_pass_mesh.clear();
    m_deferred_update.clear();

    PROFILER_PUSH_CPU_MARKER("- culling", 0xFF, 0xFF, 0x0);
    SP::prepareDrawCalls();
    parseSceneManager(
        irr_driver->getSceneManager()->getRootSceneNode()->getChildren(),
        &m_immediate_draw_list, camnode, shadow_matrices);
    SP::updateModelMatrix();
    PROFILER_POP_CPU_MARKER();

    PROFILER_PUSH_CPU_MARKER("- cpu particle generation", 0x2F, 0x1F, 0x11);
    CPUParticleManager::getInstance()->generateAll();
    PROFILER_POP_CPU_MARKER();

    PROFILER_PUSH_CPU_MARKER("- cpu particle upload", 0x3F, 0x03, 0x61);
    CPUParticleManager::getInstance()->uploadAll();
    PROFILER_POP_CPU_MARKER();

    PROFILER_PUSH_CPU_MARKER("- SP::upload instance and skinning matrices",
        0xFF, 0x0, 0xFF);
    SP::uploadAll();
    PROFILER_POP_CPU_MARKER();
}

// ----------------------------------------------------------------------------
void DrawCalls::renderImmediateDrawList() const
{
    glActiveTexture(GL_TEXTURE0);
    for(auto node: m_immediate_draw_list)
        node->render();
}

// ----------------------------------------------------------------------------
void DrawCalls::renderParticlesList() const
{
    CPUParticleManager::getInstance()->drawAll();
}

#if !defined(USE_GLES2)

// ----------------------------------------------------------------------------
 /** Render the solid first pass (depth and normals)
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance and GL_ARB_draw_indirect extensions)
 */ 
void DrawCalls::drawIndirectSolidFirstPass() const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->drawIndirectFirstPass<DefaultMaterial>();
    m_solid_cmd_buffer->drawIndirectFirstPass<AlphaRef>();
    m_solid_cmd_buffer->drawIndirectFirstPass<UnlitMat>();
    m_solid_cmd_buffer->drawIndirectFirstPass<SphereMap>();
    m_solid_cmd_buffer->drawIndirectFirstPass<GrassMat>(m_wind_dir);
    m_solid_cmd_buffer->drawIndirectFirstPass<DetailMat>();
    m_solid_cmd_buffer->drawIndirectFirstPass<NormalMat>();

    if (!CVS->supportsHardwareSkinning()) return;
    m_solid_cmd_buffer->drawIndirectFirstPass<SkinnedSolid>();
    m_solid_cmd_buffer->drawIndirectFirstPass<SkinnedAlphaRef>();
    m_solid_cmd_buffer->drawIndirectFirstPass<SkinnedUnlitMat>();
    m_solid_cmd_buffer->drawIndirectFirstPass<SkinnedNormalMat>();
}

// ----------------------------------------------------------------------------
 /** Render the solid first pass (depth and normals)
 * Require OpenGL AZDO extensions. Faster than drawIndirectSolidFirstPass.
 */ 
void DrawCalls::multidrawSolidFirstPass() const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->multidrawFirstPass<DefaultMaterial>();
    m_solid_cmd_buffer->multidrawFirstPass<AlphaRef>();
    m_solid_cmd_buffer->multidrawFirstPass<SphereMap>();
    m_solid_cmd_buffer->multidrawFirstPass<UnlitMat>();
    m_solid_cmd_buffer->multidrawFirstPass<GrassMat>(m_wind_dir);
    m_solid_cmd_buffer->multidrawFirstPass<NormalMat>();
    m_solid_cmd_buffer->multidrawFirstPass<DetailMat>();

    if (!CVS->supportsHardwareSkinning()) return;
    m_solid_cmd_buffer->multidrawFirstPass<SkinnedSolid>();
    m_solid_cmd_buffer->multidrawFirstPass<SkinnedAlphaRef>();
    m_solid_cmd_buffer->multidrawFirstPass<SkinnedUnlitMat>();
    m_solid_cmd_buffer->multidrawFirstPass<SkinnedNormalMat>();
}

// ----------------------------------------------------------------------------
 /** Render the solid second pass (apply lighting on materials)
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance and GL_ARB_draw_indirect extensions)
 * \param prefilled_tex The textures which have been drawn 
 *                      during previous rendering passes.
 */ 
void DrawCalls::drawIndirectSolidSecondPass(const std::vector<GLuint> &prefilled_tex) const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->drawIndirectSecondPass<DefaultMaterial>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<AlphaRef>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<UnlitMat>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<SphereMap>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<GrassMat>(prefilled_tex, m_wind_dir);
    m_solid_cmd_buffer->drawIndirectSecondPass<DetailMat>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<NormalMat>(prefilled_tex);

    if (!CVS->supportsHardwareSkinning()) return;
    m_solid_cmd_buffer->drawIndirectSecondPass<SkinnedSolid>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<SkinnedAlphaRef>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<SkinnedUnlitMat>(prefilled_tex);
    m_solid_cmd_buffer->drawIndirectSecondPass<SkinnedNormalMat>(prefilled_tex);
}

// ----------------------------------------------------------------------------
 /** Render the solid second pass (apply lighting on materials)
 * Require OpenGL AZDO extensions. Faster than drawIndirectSolidSecondPass.
 * \param handles The handles to textures which have been drawn 
 *                during previous rendering passes.
 */
void DrawCalls::multidrawSolidSecondPass(const std::vector<uint64_t> &handles) const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->multidraw2ndPass<DefaultMaterial>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<AlphaRef>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<SphereMap>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<UnlitMat>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<NormalMat>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<DetailMat>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<GrassMat>(handles, m_wind_dir);

    if (!CVS->supportsHardwareSkinning()) return;
    m_solid_cmd_buffer->multidraw2ndPass<SkinnedSolid>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<SkinnedAlphaRef>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<SkinnedUnlitMat>(handles);
    m_solid_cmd_buffer->multidraw2ndPass<SkinnedNormalMat>(handles);
}

// ----------------------------------------------------------------------------
 /** Render normals (for debug)
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance and GL_ARB_draw_indirect extensions)
 */ 
void DrawCalls::drawIndirectNormals() const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->drawIndirectNormals<DefaultMaterial>();
    m_solid_cmd_buffer->drawIndirectNormals<AlphaRef>();
    m_solid_cmd_buffer->drawIndirectNormals<UnlitMat>();
    m_solid_cmd_buffer->drawIndirectNormals<SphereMap>();
    m_solid_cmd_buffer->drawIndirectNormals<DetailMat>();
    m_solid_cmd_buffer->drawIndirectNormals<NormalMat>();
}

// ----------------------------------------------------------------------------
 /** Render normals (for debug)
 * Require OpenGL AZDO extensions. Faster than drawIndirectNormals.
 */ 
void DrawCalls::multidrawNormals() const
{
    m_solid_cmd_buffer->bind();
    m_solid_cmd_buffer->multidrawNormals<DefaultMaterial>();
    m_solid_cmd_buffer->multidrawNormals<AlphaRef>();
    m_solid_cmd_buffer->multidrawNormals<UnlitMat>();
    m_solid_cmd_buffer->multidrawNormals<SphereMap>();
    m_solid_cmd_buffer->multidrawNormals<DetailMat>();
    m_solid_cmd_buffer->multidrawNormals<NormalMat>();
}

// ----------------------------------------------------------------------------
 /** Render shadow map
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance and GL_ARB_draw_indirect extensions)
 * \param cascade The id of the cascading shadow map.
 */
void DrawCalls::drawIndirectShadows(unsigned cascade) const
{
    m_shadow_cmd_buffer->bind();
    m_shadow_cmd_buffer->drawIndirect<DefaultMaterial>(cascade);
    m_shadow_cmd_buffer->drawIndirect<DetailMat>(cascade);
    m_shadow_cmd_buffer->drawIndirect<AlphaRef>(cascade);
    m_shadow_cmd_buffer->drawIndirect<UnlitMat>(cascade);
    m_shadow_cmd_buffer->drawIndirect<GrassMat,irr::core::vector3df>(cascade, m_wind_dir);
    m_shadow_cmd_buffer->drawIndirect<NormalMat>(cascade);
    m_shadow_cmd_buffer->drawIndirect<SplattingMat>(cascade);
    m_shadow_cmd_buffer->drawIndirect<SphereMap>(cascade);

    if (!CVS->supportsHardwareSkinning()) return;
    m_shadow_cmd_buffer->drawIndirect<SkinnedSolid>(cascade);
    m_shadow_cmd_buffer->drawIndirect<SkinnedAlphaRef>(cascade);
    m_shadow_cmd_buffer->drawIndirect<SkinnedUnlitMat>(cascade);
    m_shadow_cmd_buffer->drawIndirect<SkinnedNormalMat>(cascade);
}

// ----------------------------------------------------------------------------
 /** Render shadow map
 * Require OpenGL AZDO extensions. Faster than drawIndirectShadows.
 * \param cascade The id of the cascading shadow map.
 */
void DrawCalls::multidrawShadows(unsigned cascade) const
{
    m_shadow_cmd_buffer->bind();
    m_shadow_cmd_buffer->multidrawShadow<DefaultMaterial>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<DetailMat>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<NormalMat>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<AlphaRef>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<UnlitMat>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<GrassMat,irr::core::vector3df>(cascade, m_wind_dir); 
    m_shadow_cmd_buffer->multidrawShadow<SplattingMat>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<SphereMap>(cascade);

    if (!CVS->supportsHardwareSkinning()) return;
    m_shadow_cmd_buffer->multidrawShadow<SkinnedSolid>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<SkinnedAlphaRef>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<SkinnedUnlitMat>(cascade);
    m_shadow_cmd_buffer->multidrawShadow<SkinnedNormalMat>(cascade);
}

// ----------------------------------------------------------------------------
 /** Render reflective shadow map (need to be called only once per track)
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance and GL_ARB_draw_indirect extensions)
 * \param rsm_matrix The reflective shadow map matrix
 */
void DrawCalls::drawIndirectReflectiveShadowMaps(const core::matrix4 &rsm_matrix) const
{
    m_reflective_shadow_map_cmd_buffer->bind();
    m_reflective_shadow_map_cmd_buffer->drawIndirect<DefaultMaterial>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->drawIndirect<AlphaRef>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->drawIndirect<UnlitMat>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->drawIndirect<NormalMat>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->drawIndirect<DetailMat>(rsm_matrix);   
}

// ----------------------------------------------------------------------------
 /** Render reflective shadow map (need to be called only once per track)
 * Require OpenGL AZDO extensions. Faster than drawIndirectReflectiveShadowMaps.
 * \param rsm_matrix The reflective shadow map matrix
 */
void DrawCalls::multidrawReflectiveShadowMaps(const core::matrix4 &rsm_matrix) const
{
    m_reflective_shadow_map_cmd_buffer->bind();
    m_reflective_shadow_map_cmd_buffer->multidraw<DefaultMaterial>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->multidraw<NormalMat>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->multidraw<AlphaRef>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->multidraw<UnlitMat>(rsm_matrix);
    m_reflective_shadow_map_cmd_buffer->multidraw<DetailMat>(rsm_matrix);
}

// ----------------------------------------------------------------------------
 /** Render glowing objects.
 * Require at least OpenGL 4.0
 * or GL_ARB_base_instance, GL_ARB_draw_indirect
 *    and GL_ARB_explicit_attrib_location extensions
 */
void DrawCalls::drawIndirectGlow() const
{
    m_glow_cmd_buffer->bind();
    m_glow_cmd_buffer->drawIndirect();
}

// ----------------------------------------------------------------------------
 /** Render glowing objects.
 * Require OpenGL AZDO extensions. Faster than drawIndirectGlow.
 */
void DrawCalls::multidrawGlow() const
{
    m_glow_cmd_buffer->bind();
    m_glow_cmd_buffer->multidraw();
}
#endif // !defined(USE_GLES2)

// ----------------------------------------------------------------------------
int32_t DrawCalls::getSkinningOffset() const
{
    return std::accumulate(m_mesh_for_skinning.begin(),
        m_mesh_for_skinning.end(), 0, []
        (const unsigned int previous, const STKAnimatedMesh* m)
        { return previous + m->getTotalJoints(); });
}   // getSkinningOffset
#endif   // !SERVER_ONLY
