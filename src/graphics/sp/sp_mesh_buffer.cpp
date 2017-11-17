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
    // layer 1, uv 1 texture (white default if none)
    // layer 2, uv 2 texture (white default if none)
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

    // Default all transparent first
    for (unsigned i = 2; i < video::MATERIAL_MAX_TEXTURES; i++)
    {
        m_material.setTexture(i, stktm->getUnicolorTexture
            (irr::video::SColor(0, 0, 0, 0)));
    }

    // Splatting different case, 3 4 5 6 are 1 2 3 4 splatting detail
    if (m_stk_material->getShaderName() == "splatting")
    {
        TexConfig stc(true/*srgb*/, false/*premul_alpha*/, true/*mesh_tex*/,
            false/*set_material*/);
        m_material.setTexture(3, stktm->getTexture(m_stk_material
            ->getSplatting1(), &stc));
        m_material.setTexture(4, stktm->getTexture(m_stk_material
            ->getSplatting2(), &stc));
        m_material.setTexture(5, stktm->getTexture(m_stk_material
            ->getSplatting3(), &stc));
        m_material.setTexture(6, stktm->getTexture(m_stk_material
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
                m_material.setTexture(3, stktm
                    ->getTexture(m_stk_material->getGlossMap()));
            }
            if (!m_stk_material->getNormalMap().empty())
            {
                TexConfig nmtc(false/*srgb*/, false/*premul_alpha*/,
                    true/*mesh_tex*/, false/*set_material*/,
                    false/*color_mask*/, true/*normal_map*/);
                m_material.setTexture(4, stktm
                    ->getTexture(m_stk_material->getNormalMap(), &nmtc));
            }
        }
        if (m_stk_material->isColorizable())
        {
            TexConfig cmtc(false/*srgb*/, false/*premul_alpha*/,
                true/*mesh_tex*/, false/*set_material*/, true/*color_mask*/);
            if (!m_stk_material->getColorizationMask().empty())
            {
                m_material.setTexture(5, stktm->getTexture(m_stk_material
                    ->getColorizationMask(), &cmtc));
            }
        }
        else
        {
            m_material.setTexture(5, stktm->getUnicolorTexture
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
void SPMeshBuffer::uploadVBOIBO(bool skinned)
{
#ifndef SERVER_ONLY
    bool use_2_uv = m_stk_material->use2UV();
    bool use_tangents = !m_stk_material->getNormalMap().empty();
        const unsigned pitch = getVertexPitch(m_vt) - (use_tangents ? 0 : 8);
#endif
}   // uploadVBOIBO

}
