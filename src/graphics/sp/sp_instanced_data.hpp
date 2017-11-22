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
#include <cassert>
#include <matrix4.h>

using namespace irr;

namespace SP
{

class SPInstancedData
{
private:
    char m_data[32];

public:
    // ------------------------------------------------------------------------
    SPInstancedData(const core::matrix4& model_mat,
                    const core::vector2df& texture_trans, float hue,
                    float min_sat, int skinning_offset)
    {
        static_assert(sizeof(SPInstancedData) == 32, "Size is not 32");
        float position[3] = { model_mat[12], model_mat[13], model_mat[14] };
        core::quaternion rotation(0.0f, 0.0f, 0.0f, 1.0f);
        core::vector3df scale = model_mat.getScale();
        if (scale.X != 0.0f && scale.Y != 0.0f && scale.Z != 0.0f)
        {
            core::matrix4 local_mat = model_mat;
            local_mat[0] = local_mat[0] / scale.X;
            local_mat[1] = local_mat[1] / scale.X;
            local_mat[2] = local_mat[2] / scale.X;
            local_mat[4] = local_mat[4] / scale.Y;
            local_mat[5] = local_mat[5] / scale.Y;
            local_mat[6] = local_mat[6] / scale.Y;
            local_mat[8] = local_mat[8] / scale.Z;
            local_mat[9] = local_mat[9] / scale.Z;
            local_mat[10] = local_mat[10] / scale.Z;
            rotation = core::quaternion(local_mat);
            // Conjugated quaternion in glsl
            rotation.X = -rotation.X;
            rotation.Y = -rotation.Y;
            rotation.Z = -rotation.Z;
        }
        using namespace MiniGLM;
        /* testsuite
        core::matrix4 lm, sm, rm;
        lm.setTranslation(core::vector3df(position[0], position[1],
            position[2]));
        sm.setScale(scale);
        rotation.getMatrix(rm);
        core::matrix4 new_mat = lm * rm * sm;
        core::vector3df test(4.0f, 4.0f, 4.0f);
        core::vector3df test1(4.0f, 4.0f, 4.0f);
        new_mat.transformVect(test);
        model_mat.transformVect(test1);
        core::vector3df ret = test - test1;
        ret.X = fabsf(ret.X);
        ret.Y = fabsf(ret.Y);
        ret.Z = fabsf(ret.Z);
        if (ret.getLengthSQ() > 0.001f)
        {
            printf("%f %f %f <<< new", test.X, test.Y, test.Z);
            printf("%f %f %f\n", test1.X, test1.Y, test1.Z);
        }*/
        memcpy(m_data, position, 12);
        uint32_t q = compressQuaternion(rotation);
        memcpy(m_data + 12, &q, 4);
        short s[4] = { toFloat16(scale.X), toFloat16(scale.Y),
            toFloat16(scale.Z), toFloat16(rotation.W)};
        memcpy(m_data + 16, s, 8);
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
