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

#include "graphics/sp/sp_mesh_node.hpp"
#include "graphics/sp/sp_mesh.hpp"
#include "graphics/sp/sp_animation.hpp"
#include "graphics/sp/sp_mesh_buffer.hpp"

#include <algorithm>

namespace SP
{
// ----------------------------------------------------------------------------
SPMeshNode::SPMeshNode(IAnimatedMesh* mesh, ISceneNode* parent,
                       ISceneManager* mgr, s32 id,
                       const std::string& debug_name,
                       const core::vector3df& position,
                       const core::vector3df& rotation,
                       const core::vector3df& scale, RenderInfo* render_info)
          : CAnimatedMeshSceneNode(mesh, parent, mgr, id, position, rotation,
                                   scale)
{
    m_mesh = NULL;
    m_mesh_render_info = render_info;
    m_animated = !m_mesh->isStatic();
    m_skinning_offset = -1;
}   // SPMeshNode

// ----------------------------------------------------------------------------
SPMeshNode::~SPMeshNode()
{
}   // ~SPMeshNode

// ----------------------------------------------------------------------------
void SPMeshNode::setAnimationState(bool val)
{
    if (!m_mesh)
    {
        return;
    }
    m_animated = !m_mesh->isStatic() && val;
}   // setAnimationState

// ----------------------------------------------------------------------------
void SPMeshNode::setMesh(irr::scene::IAnimatedMesh* mesh)
{
    m_mesh = static_cast<SPMesh*>(mesh);
    CAnimatedMeshSceneNode::setMesh(mesh);
    m_joint_nodes.clear();
    if (!m_mesh->isStatic())
    {
        
    }
}   // setMesh

// ----------------------------------------------------------------------------
IBoneSceneNode* SPMeshNode::getJointNode(const c8* joint_name)
{
}   // getJointNode

// ----------------------------------------------------------------------------
void SPMeshNode::OnAnimate(u32 time_ms)
{
    if (m_mesh->isStatic())
    {
        IAnimatedMeshSceneNode::OnAnimate(time_ms);
        return;
    }
    CAnimatedMeshSceneNode::OnAnimate(time_ms);
}   // OnAnimate

// ----------------------------------------------------------------------------
IMesh* SPMeshNode::getMeshForCurrentFrame(SkinningCallback sc,
                                                      int offset)
{

    return m_mesh;

}   // getMeshForCurrentFrame

}
