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

#ifndef HEADER_SP_ANIMATION_HPP
#define HEADER_SP_ANIMATION_HPP

#include "graphics/render_info.hpp"
#include "utils/mini_glm.hpp"
#include "utils/vec3.hpp"

#include <matrix4.h>

#include <cassert>
#include <array>

using namespace irr;

namespace SP
{

class SPInstancedData
{
private:
    std::array<short, 16> m_data;

    // ------------------------------------------------------------------------
    SPInstancedData(const core::matrix4& model_mat,
                    const core::vector2df& texture_trans, RenderInfo* ri,
                    short skinning_offset)
    {
        core::vector3df position = model_mat.getTranslation();
        btQuaternion rotation(0.0f, 0.0f, 0.0f, 1.0f);
        Vec3 scale(1.0f, 1.0f, 1.0f);
        Vec3 new_scale = model_matrix.getScale();
        if (new_scale.x() != 0.0f && new_scale.y() != 0.0f &&
            new_scale.z() != 0.0f)
        {
            scale = new_scale;
            btMatrix3x3 m(
                Vec3(model_mat[0], model_mat[1], model_mat[2]) / scale,
                Vec3(model_mat[4], model_mat[5], model_mat[6]) / scale,
                Vec3(model_mat[8], model_mat[9], model_mat[10]) / scale);
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
        
    }
    // ------------------------------------------------------------------------
    const void* getData() const { return m_data.data(); }
};



}

#endif
