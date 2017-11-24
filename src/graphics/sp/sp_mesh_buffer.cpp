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

#include "graphics/sp/sp_mesh_buffer.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/stk_tex_manager.hpp"
#include "race/race_manager.hpp"
#include "utils/mini_glm.hpp"
#include "utils/string_utils.hpp"

namespace SP
{
// ----------------------------------------------------------------------------
void SPMeshBuffer::initDrawMaterial()
{
#ifndef SERVER_ONLY
    // layer 1, uv 1 texture (white default if none), reported by .spm
    // layer 2, uv 2 texture (white default if none), reported by .spm
    core::stringc layer_two = m_material.getTexture(1) ?
        m_material.getTexture(1)->getName().getPtr() : "";
    layer_two.make_lower();
    const std::string lt_cmp = StringUtils::getBasename(layer_two.c_str());
    STKTexManager* stktm = STKTexManager::getInstance();
    if (m_material.getTexture(0) == NULL)
    {
        m_material.setTexture(0, stktm->getUnicolorTexture
            (irr::video::SColor(255, 255, 255, 255)));
    }
    if (m_material.getTexture(1) == NULL)
    {
        m_material.setTexture(1, stktm->getUnicolorTexture
            (irr::video::SColor(255, 255, 255, 255)));
    }
    m_stk_material = material_manager->getMaterialFor(m_material.getTexture(0),
        lt_cmp);
    if (m_stk_material == NULL)
    {
        m_stk_material = material_manager->getSPMaterial("solid");
    }
    else
    {
        // install
        m_stk_material->getTexture();
    }
    m_tex_cmp = m_material.getTexture(0)->getName().getPtr();
    m_tex_cmp += m_material.getTexture(1)->getName().getPtr();

    // Default all transparent first
    for (unsigned i = 2; i < video::MATERIAL_MAX_TEXTURES; i++)
    {
        m_material.setTexture(i, stktm->getUnicolorTexture
            (irr::video::SColor(0, 0, 0, 0)));
    }

    // Below all textures are reported by material_manager
    // Splatting different case, 3 4 5 6 are 1 2 3 4 splatting detail
    if (m_stk_material->getShaderName() == "splatting")
    {
        TexConfig stc(true/*srgb*/, false/*premul_alpha*/, true/*mesh_tex*/,
            false/*set_material*/);
        m_material.setTexture(2, stktm->getTexture(m_stk_material
            ->getSplatting1(), &stc));
        m_material.setTexture(3, stktm->getTexture(m_stk_material
            ->getSplatting2(), &stc));
        m_material.setTexture(4, stktm->getTexture(m_stk_material
            ->getSplatting3(), &stc));
        m_material.setTexture(5, stktm->getTexture(m_stk_material
            ->getSplatting4(), &stc));
    }
    else
    {
        // layer 3, gloss texture (transparent default if none)
        // layer 4, normal map texture (transparent default if none)
        // layer 5, colorization mask texture (white default if non-colorizable,
        // transparent otherwise)
        if (CVS->isDefferedEnabled())
        {
            if (!m_stk_material->getGlossMap().empty())
            {
                m_material.setTexture(2, stktm
                    ->getTexture(m_stk_material->getGlossMap()));
            }
            if (!m_stk_material->getNormalMap().empty())
            {
                TexConfig nmtc(false/*srgb*/, false/*premul_alpha*/,
                    true/*mesh_tex*/, false/*set_material*/,
                    false/*color_mask*/, true/*normal_map*/);
                m_material.setTexture(3, stktm
                    ->getTexture(m_stk_material->getNormalMap(), &nmtc));
            }
        }
        if (m_stk_material->isColorizable())
        {
            TexConfig cmtc(false/*srgb*/, false/*premul_alpha*/,
                true/*mesh_tex*/, false/*set_material*/, true/*color_mask*/);
            if (!m_stk_material->getColorizationMask().empty())
            {
                m_material.setTexture(4, stktm->getTexture(m_stk_material
                    ->getColorizationMask(), &cmtc));
            }
        }
        else
        {
            m_material.setTexture(4, stktm->getUnicolorTexture
                (irr::video::SColor(255, 255, 255, 255)));
        }
    }
    if (!m_stk_material->backFaceCulling())
    {
        m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
    }
    if (race_manager->getReverseTrack() &&
        m_stk_material->getMirrorAxisInReverse() != ' ')
    {
        for (unsigned i = 0; i < getVertexCount(); i++)
        {
            using namespace MiniGLM;
            if (m_stk_material->getMirrorAxisInReverse() == 'V')
            {
                m_vertices[i].m_all_uvs[1] =
                    toFloat16(1.0f - toFloat32(m_vertices[i].m_all_uvs[1]));
            }
            else
            {
                m_vertices[i].m_all_uvs[0] =
                    toFloat16(1.0f - toFloat32(m_vertices[i].m_all_uvs[0]));
            }
        }
    }   // reverse track and texture needs mirroring

#endif
}   // initDrawMaterial

// ----------------------------------------------------------------------------
inline int srgbToLinear(float color_srgb)
{
    int ret;
    if (color_srgb <= 0.04045f)
    {
        ret = 255 * (color_srgb / 12.92f);
    }
    else
    {
        ret = 255 * (powf((color_srgb + 0.055f) / 1.055f, 2.4f));
    }
    return core::clamp(ret, 0, 255);
}

// ----------------------------------------------------------------------------
void SPMeshBuffer::uploadGLMesh(bool skinned)
{
#ifndef SERVER_ONLY
    bool use_2_uv = m_stk_material->use2UV();
    bool use_tangents = !m_stk_material->getNormalMap().empty();
    const unsigned pitch = 48 - (use_tangents ? 0 : 4) - (use_2_uv ? 0 : 4) -
        (skinned ? 0 : 16);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    unsigned v_size = m_vertices.size() * pitch;
    glBufferData(GL_ARRAY_BUFFER, v_size, NULL, GL_DYNAMIC_DRAW);
    size_t offset = 0;
    char* ptr = (char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, v_size,
        GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT |
        GL_MAP_INVALIDATE_BUFFER_BIT);
    v_size = 0;
    for (unsigned i = 0 ; i < m_vertices.size(); i++)
    {
        offset = 0;
        memcpy(ptr + v_size + offset, &m_vertices[i].m_position.X, 16);
        offset += 16;
        video::SColor vc = m_vertices[i].m_color;
        if (CVS->isDefferedEnabled() ||
            CVS->isARBSRGBFramebufferUsable())
        {
            video::SColorf tmp(vc);
            vc.setRed(srgbToLinear(tmp.r));
            vc.setGreen(srgbToLinear(tmp.g));
            vc.setBlue(srgbToLinear(tmp.b));
        }
        memcpy(ptr + v_size + offset, &vc, 4);
        offset += 4;
        memcpy(ptr + v_size + offset, &m_vertices[i].m_all_uvs[0], 4);
        offset += 4;
        if (use_2_uv)
        {
            memcpy(ptr + v_size + offset, &m_vertices[i].m_all_uvs[2], 4);
            offset += 4;
        }
        if (use_tangents)
        {
            memcpy(ptr + v_size + offset, &m_vertices[i].m_tangent, 4);
            offset += 4;
        }
        if (skinned)
        {
            memcpy(ptr + v_size + offset, &m_vertices[i].m_joint_idx[0], 16);
        }
        v_size += pitch;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * 2,
        m_indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_ins_array);
    glBufferData(GL_ARRAY_BUFFER, m_gl_instance_size * 32, NULL,
        GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    offset = 0;
    bindVAO();
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, pitch, (void*)offset);
    offset += 12;
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, pitch,
        (void*)offset);
    offset += 4;
    // Vertex color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, pitch,
        (void*)offset);
    offset += 4;
    // 1st texture coordinates
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_HALF_FLOAT, GL_FALSE, pitch, (void*)offset);
    offset += 4;
    if (use_2_uv)
    {
        // 2nd texture coordinates
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 2, GL_HALF_FLOAT, GL_FALSE, pitch,
            (void*)offset);
        offset += 4;
    }
    if (use_tangents)
    {
        // Tangent and bi-tanget sign
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_INT_2_10_10_10_REV, GL_TRUE, pitch,
            (void*)offset);
        offset += 4;
    }
    if (skinned)
    {
        // 4 Joint indices
        glEnableVertexAttribArray(6);
        glVertexAttribIPointer(6, 4, GL_SHORT, pitch, (void*)offset);
        offset += 8;
        // 4 Joint weights
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_HALF_FLOAT, GL_FALSE, pitch,
            (void*)offset);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ins_array);
    // Origin
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 32, (void*)0);
    glVertexAttribDivisorARB(8, 1);
    // Rotation (quaternion .xyz)
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 4, GL_INT_2_10_10_10_REV, GL_TRUE, 32,
        (void*)12);
    glVertexAttribDivisorARB(9, 1);
    // Scale (3 half floats and .w for quaternion)
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_HALF_FLOAT, GL_FALSE, 32, (void*)16);
    glVertexAttribDivisorARB(10, 1);
    // Misc data (texture translation and colorization info)
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_BYTE, GL_TRUE, 32, (void*)24);
    glVertexAttribDivisorARB(11, 1);
    // Skinning offset
    glEnableVertexAttribArray(12);
    glVertexAttribIPointer(12, 1, GL_INT, 32, (void*)28);
    glVertexAttribDivisorARB(12, 1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}   // uploadGLMesh

}
