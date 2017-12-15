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
#include "graphics/sp/sp_texture_manager.hpp"
#include "race/race_manager.hpp"
#include "utils/mini_glm.hpp"
#include "utils/string_utils.hpp"

namespace SP
{
// ----------------------------------------------------------------------------
SPMeshBuffer::~SPMeshBuffer()
{
#ifndef SERVER_ONLY
    for (unsigned i = 0; i < DCT_FOR_VAO; i++)
    {
        if (m_vao[i] != 0)
        {
            glDeleteVertexArrays(1, &m_vao[i]);
        }
        if (m_ins_array[i] != 0)
        {
            if (CVS->isARBBufferStorageUsable())
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_ins_array[i]);
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            glDeleteBuffers(1, &m_ins_array[i]);
        }
    }
    if (m_ibo != 0)
    {
        glDeleteBuffers(1, &m_ibo);
    }
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
    }
#endif
}   // ~SPMeshBuffer

// ----------------------------------------------------------------------------
void SPMeshBuffer::initDrawMaterial()
{
#ifndef SERVER_ONLY
    Material* m = std::get<2>(m_stk_material[0]);
    if (race_manager->getReverseTrack() && m->getMirrorAxisInReverse() != ' ')
    {
        for (unsigned i = 0; i < getVertexCount(); i++)
        {
            using namespace MiniGLM;
            if (m->getMirrorAxisInReverse() == 'V')
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
        ret = (int)(255.0f * (color_srgb / 12.92f));
    }
    else
    {
        ret = (int)(255.0f * (powf((color_srgb + 0.055f) / 1.055f, 2.4f)));
    }
    return core::clamp(ret, 0, 255);
}

// ----------------------------------------------------------------------------
void SPMeshBuffer::uploadGLMesh()
{
    if (m_uploaded_gl)
    {
        return;
    }
    m_uploaded_gl = true;
#ifndef SERVER_ONLY
    m_textures.resize(m_stk_material.size());
    for (unsigned i = 0; i < m_stk_material.size(); i++)
    {
        for (unsigned j = 0; j < 6; j++)
        {
            m_textures[i][j] = SPTextureManager::get()->getTexture
                (std::get<2>(m_stk_material[i])->getSamplerPath(j));
        }
    }

    bool use_2_uv = std::get<2>(m_stk_material[0])->use2UV();
    bool use_tangents =
        std::get<2>(m_stk_material[0])->getShaderName() == "normalmap" &&
        CVS->isDefferedEnabled();
    const bool vt_2101010 = CVS->isARBVertexType2101010RevUsable();
    const unsigned pitch = 48 - (use_tangents ? 0 : 4) - (use_2_uv ? 0 : 4) -
        (m_skinned ? 0 : 16) + (use_tangents && !vt_2101010 ? 4 : 0)
        + (!vt_2101010 ? 4 : 0) + (CVS->isARBBindlessTextureUsable() ? 48 : 0);
    m_pitch = pitch;

    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
    }
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    unsigned v_size = (unsigned)m_vertices.size() * pitch;
    glBufferData(GL_ARRAY_BUFFER, v_size, NULL, GL_DYNAMIC_DRAW);
    size_t offset = 0;
    char* ptr = (char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, v_size,
        GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT |
        GL_MAP_INVALIDATE_BUFFER_BIT);
    v_size = 0;
    for (unsigned i = 0 ; i < m_vertices.size(); i++)
    {
        offset = 0;
        memcpy(ptr + v_size + offset, &m_vertices[i].m_position.X, 12);
        offset += 12;
        if (vt_2101010)
        {
            memcpy(ptr + v_size + offset, &m_vertices[i].m_normal, 12);
            offset += 4;
        }
        else
        {
            std::array<short, 4> normal = MiniGLM::vertexType2101010RevTo4HF
                (m_vertices[i].m_normal);
            memcpy(ptr + v_size + offset, normal.data(), 8);
            offset += 8;
        }

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
            if (vt_2101010)
            {
                memcpy(ptr + v_size + offset, &m_vertices[i].m_tangent, 4);
                offset += 4;
            }
            else
            {
                std::array<short, 4> tangent = MiniGLM::
                    vertexType2101010RevTo4HF(m_vertices[i].m_tangent);
                memcpy(ptr + v_size + offset, tangent.data(), 8);
                offset += 8;
            }
        }
        if (m_skinned)
        {
            memcpy(ptr + v_size + offset, &m_vertices[i].m_joint_idx[0], 16);
        }
        v_size += pitch;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (m_ibo != 0)
    {
        glDeleteBuffers(1, &m_ibo);
    }
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * 2,
        m_indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    for (unsigned i = 0; i < DCT_FOR_VAO; i++)
    {
        recreateVAO(i);
    }
#endif
}   // uploadGLMesh

// ----------------------------------------------------------------------------
void SPMeshBuffer::recreateVAO(unsigned i)
{
#ifndef SERVER_ONLY
    bool use_2_uv = std::get<2>(m_stk_material[0])->use2UV();
    bool use_tangents =
        std::get<2>(m_stk_material[0])->getShaderName() == "normalmap" &&
        CVS->isGLSL();
    const bool vt_2101010 = CVS->isARBVertexType2101010RevUsable();
    const unsigned pitch = m_pitch;

    size_t offset = 0;

    if (m_ins_array[i] == 0)
    {
        glGenBuffers(1, &m_ins_array[i]);
    }
    else
    {
        if (CVS->isARBBufferStorageUsable())
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_ins_array[i]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glDeleteBuffers(1, &m_ins_array[i]);
        glGenBuffers(1, &m_ins_array[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_ins_array[i]);
#ifndef USE_GLES2
    if (CVS->isARBBufferStorageUsable())
    {
        glBufferStorage(GL_ARRAY_BUFFER, m_gl_instance_size[i] * 40, NULL,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        m_ins_dat_mapped_ptr[i] = glMapBufferRange(GL_ARRAY_BUFFER, 0,
            m_gl_instance_size[i] * 40,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }
    else
#endif
    {
        glBufferData(GL_ARRAY_BUFFER, m_gl_instance_size[i] * 40, NULL,
            GL_DYNAMIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (m_vao[i] != 0)
    {
        glDeleteVertexArrays(1, &m_vao[i]);
    }
    glGenVertexArrays(1, &m_vao[i]);
    glBindVertexArray(m_vao[i]);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, pitch, (void*)offset);
    offset += 12;
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4,
        vt_2101010 ? GL_INT_2_10_10_10_REV : GL_HALF_FLOAT,
        vt_2101010 ? GL_TRUE : GL_FALSE, pitch, (void*)offset);
    offset += vt_2101010 ? 4 : 8;
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
        glVertexAttribPointer(5, 4,
            vt_2101010 ? GL_INT_2_10_10_10_REV : GL_HALF_FLOAT,
            vt_2101010 ? GL_TRUE : GL_FALSE, pitch,
            (void*)offset);
        offset += vt_2101010 ? 4 : 8;
    }
    if (m_skinned)
    {
        // 4 Joint indices
        glEnableVertexAttribArray(6);
        glVertexAttribIPointer(6, 4, GL_SHORT, pitch, (void*)offset);
        offset += 8;
        // 4 Joint weights
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_HALF_FLOAT, GL_FALSE, pitch,
            (void*)offset);
        offset += 8; 
    }
    if (CVS->isARBBindlessTextureUsable())
    {
        // 3 * 2 uvec2 for bindless samplers
        glEnableVertexAttribArray(13);
        glVertexAttribIPointer(13, 4, GL_UNSIGNED_INT, pitch, (void*)offset);
        offset += 16;
        glEnableVertexAttribArray(14);
        glVertexAttribIPointer(14, 4, GL_UNSIGNED_INT, pitch, (void*)offset);
        offset += 16;
        glEnableVertexAttribArray(15);
        glVertexAttribIPointer(15, 4, GL_UNSIGNED_INT, pitch, (void*)offset);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ins_array[i]);
    // Origin
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 40, (void*)0);
    glVertexAttribDivisorARB(8, 1);
    // Rotation (quaternion)
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 4, GL_HALF_FLOAT, GL_FALSE, 40, (void*)12);
    glVertexAttribDivisorARB(9, 1);
    // Scale (3 half floats and .w unused for padding)
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 4, GL_HALF_FLOAT, GL_FALSE, 40, (void*)20);
    glVertexAttribDivisorARB(10, 1);
    // Misc data (texture translation and colorization info)
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 4, GL_HALF_FLOAT, GL_FALSE, 40, (void*)28);
    glVertexAttribDivisorARB(11, 1);
    // Skinning offset
    glEnableVertexAttribArray(12);
    glVertexAttribIPointer(12, 1, GL_INT, 40, (void*)36);
    glVertexAttribDivisorARB(12, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}   // uploadGLMesh

// ----------------------------------------------------------------------------
void SPMeshBuffer::uploadInstanceData()
{
#ifndef SERVER_ONLY
    if (!m_init_texture)
    {
        if (CVS->isARBBindlessTextureUsable())
        {
        }
        m_init_texture = true;
    }

    for (unsigned i = 0; i < DCT_FOR_VAO; i++)
    {
        if (m_ins_dat[i].empty())
        {
            continue;
        }
        unsigned new_size = m_gl_instance_size[i];
        while (m_ins_dat[i].size() > new_size)
        {
            // Power of 2 allocation strategy, like std::vector in gcc
            new_size <<= 1;
        }
        if (new_size != m_gl_instance_size[i])
        {
            m_gl_instance_size[i] = new_size;
            recreateVAO(i);
        }
        if (CVS->isARBBufferStorageUsable())
        {
            memcpy(m_ins_dat_mapped_ptr[i], m_ins_dat[i].data(),
                m_ins_dat[i].size() * 40);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_ins_array[i]);
            void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0,
                m_ins_dat[i].size() * 40, GL_MAP_WRITE_BIT |
                GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            memcpy(ptr, m_ins_dat[i].data(), m_ins_dat[i].size() * 40);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
#endif
    m_uploaded_instance = true;
}   // uploadInstanceData

}
