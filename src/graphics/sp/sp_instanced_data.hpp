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

#ifndef HEADER_SP_INSTANCED_DATA_HPP
#define HEADER_SP_INSTANCED_DATA_HPP

#include "utils/mini_glm.hpp"
#include "utils/vec3.hpp"

#include <matrix4.h>

#include <cassert>

using namespace irr;

namespace SP
{

class SPInstancedData
{
private:
    char m_data[32];

    // ------------------------------------------------------------------------
    SPInstancedData(const core::matrix4& model_mat,
                    const core::vector2df& texture_trans, float hue,
                    float min_sat, int skinning_offset)
    {
        static_assert(sizeof(SPInstancedData) == 32, "Size is not 32");
        float position[3] = { model_mat[12], model_mat[13], model_mat[14] };
        btQuaternion rotation(0.0f, 0.0f, 0.0f, 1.0f);
        Vec3 scale(1.0f, 1.0f, 1.0f);
        Vec3 new_scale = model_mat.getScale();
        if (new_scale.x() != 0.0f && new_scale.y() != 0.0f &&
            new_scale.z() != 0.0f)
        {
            scale = new_scale;
            Vec3 v1 = Vec3(model_mat[0], model_mat[1], model_mat[2]) / scale;
            Vec3 v2 = Vec3(model_mat[4], model_mat[5], model_mat[6]) / scale;
            Vec3 v3 = Vec3(model_mat[8], model_mat[9], model_mat[10]) / scale;
            btMatrix3x3 m(v1.x(), v1.y(), v1.z(), v2.x(), v2.y(), v2.z(),
                 v3.x(), v3.y(), v3.z());
            m.getRotation(rotation);
            rotation = rotation.normalize();
            if (rotation.w() < 0.0f)
            {
                rotation.setX(-rotation.x());
                rotation.setY(-rotation.y());
                rotation.setZ(-rotation.z());
                rotation.setW(-rotation.w());
            }
        }
        using namespace MiniGLM;
        memcpy(m_data, position, 12);
        uint32_t q = compressbtQuaternion(rotation);
        memcpy(m_data + 12, &q, 4);
        short s[4] = { toFloat16(scale.x()), toFloat16(scale.y()),
            toFloat16(scale.z()), 0};
        memcpy(m_data + 16, &s, 8);
        m_data[24] = core::clamp((char)(texture_trans.X *
            (texture_trans.X >= 0.0f ? 127.0f : 128.0f)), (char)-128,
            (char)127);
        m_data[25] = core::clamp((char)(texture_trans.Y *
            (texture_trans.Y >= 0.0f ? 127.0f : 128.0f)), (char)-128,
            (char)127);
        m_data[26] = (char)(fminf(hue, 1.0f) * 127.0f);
        m_data[27] = (char)(fminf(min_sat, 1.0f) * 127.0f);
        memcpy(m_data + 28, &skinning_offset, 4);
    }
    // ------------------------------------------------------------------------
    const void* getData() const { return m_data; }
};



}

#endif
