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

#ifndef HEADER_SP_MESH_BUFFER_HPP
#define HEADER_SP_MESH_BUFFER_HPP

#include "graphics/gl_headers.hpp"
#include "graphics/sp/sp_instanced_data.hpp"
#include "utils/types.hpp"

#include <IMeshBuffer.h>
#include <S3DVertex.h>

#include <string>
#include <vector>

using namespace irr;
using namespace scene;

class Material;

namespace SP
{

class SPMeshBuffer : public IMeshBuffer
{
private:
    video::SMaterial m_material;

    Material* m_stk_material;

    std::vector<video::S3DVertexSkinnedMesh> m_vertices;

    std::vector<uint16_t> m_indices;

    core::aabbox3d<f32> m_bounding_box;

    std::vector<SPInstancedData> m_ins_dat;

    unsigned m_gl_instance_size;

    GLuint m_vao, m_ibo, m_vbo, m_ins_array;

    std::string m_tex_cmp;

    bool m_uploaded;

public:
    SPMeshBuffer()
    {
#ifdef _DEBUG
        setDebugName("SMeshBuffer");
#endif
        m_stk_material = NULL;
        m_gl_instance_size = 1;
#ifndef SERVER_ONLY
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_ibo);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ins_array);
#endif
        m_uploaded = false;
    }
    // ------------------------------------------------------------------------
    ~SPMeshBuffer()
    {
#ifndef SERVER_ONLY
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_ibo);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ins_array);
#endif
    }
    // ------------------------------------------------------------------------
    void bindVAO() const                          { glBindVertexArray(m_vao); }
    // ------------------------------------------------------------------------
    void draw() const
    {
        bindVAO();
        glDrawElementsInstanced(GL_TRIANGLES, getIndexCount(),
            GL_UNSIGNED_SHORT, 0, m_ins_dat.size());
    }
    // ------------------------------------------------------------------------
    void initDrawMaterial();
    // ------------------------------------------------------------------------
    void uploadGLMesh(bool skinned);
    // ------------------------------------------------------------------------
    Material* getSTKMaterial() const                 { return m_stk_material; }
    // ------------------------------------------------------------------------
    const std::string& getTextureCompare() const          { return m_tex_cmp; }
    // ------------------------------------------------------------------------
    void addInstanceData(const SPInstancedData& id)
    {
        if (m_uploaded)
        {
            m_ins_dat.clear();
            m_uploaded = false;
        }
        m_ins_dat.push_back(id);
    }
    // ------------------------------------------------------------------------
    void uploadInstanceData()
    {
#ifndef SERVER_ONLY
        glBindBuffer(GL_ARRAY_BUFFER, m_ins_array);
        if (m_ins_dat.size() > m_gl_instance_size)
        {
            m_gl_instance_size = m_ins_dat.size() * 2;
            glBufferData(GL_ARRAY_BUFFER, m_gl_instance_size * 36, NULL,
                GL_DYNAMIC_DRAW);
        }
        void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0,
            m_ins_dat.size() * 36, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT |
            GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(ptr, m_ins_dat.data(), m_ins_dat.size() * 36);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
        m_uploaded = true;
    }
    // ------------------------------------------------------------------------
    
    // ------------------------------------------------------------------------
    video::S3DVertexSkinnedMesh* getSPMVertex()
    {
        return m_vertices.data();
    }
    // ------------------------------------------------------------------------
    void addSPMVertex(const video::S3DVertexSkinnedMesh& v)
    {
        m_vertices.push_back(v);
    }
    // ------------------------------------------------------------------------
    void setIndices(std::vector<uint16_t>& indices)
    {
        m_indices = std::move(indices);
    }
    // ------------------------------------------------------------------------
    void setMaterial(const video::SMaterial& m)
    {
        m_material = m;
    }
    // ------------------------------------------------------------------------
    virtual const video::SMaterial& getMaterial() const
    {
        return m_material;
    }
    // ------------------------------------------------------------------------
    virtual video::SMaterial& getMaterial()
    {
        return m_material;
    }
    // ------------------------------------------------------------------------
    virtual const void* getVertices() const
    {
        return m_vertices.data();
    }
    // ------------------------------------------------------------------------
    virtual void* getVertices()
    {
        return m_vertices.data();
    }
    // ------------------------------------------------------------------------
    virtual u32 getVertexCount() const
    {
        return m_vertices.size();
    }
    // ------------------------------------------------------------------------
    virtual video::E_INDEX_TYPE getIndexType() const
    {
        return video::EIT_16BIT;
    }
    // ------------------------------------------------------------------------
    virtual const u16* getIndices() const
    {
        return m_indices.data();
    }
    // ------------------------------------------------------------------------
    virtual u16* getIndices()
    {
        return m_indices.data();
    }
    // ------------------------------------------------------------------------
    virtual u32 getIndexCount() const
    {
        return m_indices.size();
    }
    // ------------------------------------------------------------------------
    virtual const core::aabbox3d<f32>& getBoundingBox() const
    {
        return m_bounding_box;
    }
    // ------------------------------------------------------------------------
    virtual void setBoundingBox(const core::aabbox3df& box)
    {
        m_bounding_box = box;
    }
    // ------------------------------------------------------------------------
    virtual void recalculateBoundingBox()
    {
        if (m_vertices.empty())
        {
            m_bounding_box.reset(0.0f, 0.0f, 0.0f);
        }
        else
        {
            m_bounding_box.reset(m_vertices[0].m_position);
            for (u32 i = 1; i < m_vertices.size(); i++)
            {
                m_bounding_box.addInternalPoint(m_vertices[i].m_position);
            }
        }
    }
    // ------------------------------------------------------------------------
    virtual video::E_VERTEX_TYPE getVertexType() const
    {
        return video::EVT_SKINNED_MESH;
    }
    // ------------------------------------------------------------------------
    virtual const core::vector3df& getPosition(u32 i) const
    {
        return m_vertices[i].m_position;
    }
    // ------------------------------------------------------------------------
    virtual core::vector3df& getPosition(u32 i)
    {
        return m_vertices[i].m_position;
    }
    // ------------------------------------------------------------------------
    virtual const core::vector3df& getNormal(u32 i) const
    {
        static core::vector3df unused;
        return unused;
    }
    // ------------------------------------------------------------------------
    virtual core::vector3df& getNormal(u32 i)
    {
        static core::vector3df unused;
        return unused;
    }
    // ------------------------------------------------------------------------
    virtual const core::vector2df& getTCoords(u32 i) const
    {
        static core::vector2df unused;
        return unused;
    }
    // ------------------------------------------------------------------------
    virtual core::vector2df& getTCoords(u32 i)
    {
        static core::vector2df unused;
        return unused;
    }
    // ------------------------------------------------------------------------
    virtual scene::E_PRIMITIVE_TYPE getPrimitiveType() const
    {
        return EPT_TRIANGLES;
    }
    // ------------------------------------------------------------------------
    virtual void append(const void* const vertices, u32 numm_vertices,
                        const u16* const indices, u32 numm_indices) {}
    // ------------------------------------------------------------------------
    virtual void append(const IMeshBuffer* const other) {}
    // ------------------------------------------------------------------------
    virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const
    {
        return EHM_NEVER;
    }
    // ------------------------------------------------------------------------
    virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const
    {
        return EHM_NEVER;
    }
    // ------------------------------------------------------------------------
    virtual void setHardwareMappingHint(E_HARDWARE_MAPPING,
                                        E_BUFFER_TYPE Buffer) {}
    // ------------------------------------------------------------------------
    virtual void setDirty(E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX) {}
    // ------------------------------------------------------------------------
    virtual u32 getChangedID_Vertex() const { return 0; }
    // ------------------------------------------------------------------------
    virtual u32 getChangedID_Index() const { return 0; }

};

}

#endif
