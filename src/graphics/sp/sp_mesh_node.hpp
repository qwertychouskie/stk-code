//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2014-2015 SuperTuxKart-Team
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

#ifndef HEADER_SP_MESH_NODE_HPP
#define HEADER_SP_MESH_NODE_HPP

#include "../lib/irrlicht/source/Irrlicht/CAnimatedMeshSceneNode.h"
#include <cassert>
#include <string>
#include <vector>
#include <unordered_map>

using namespace irr;
using namespace scene;
class RenderInfo;

namespace SP
{
class SPMesh;

class SPMeshNode : public irr::scene::CAnimatedMeshSceneNode
{
private:
    std::vector<RenderInfo*> m_static_render_info;

    std::unordered_map<std::string, IBoneSceneNode*> m_joint_nodes;

    RenderInfo* m_mesh_render_info;

    SPMesh* m_mesh;

    int m_skinning_offset;

    bool m_animated;
public:
    // ------------------------------------------------------------------------
    SPMeshNode(IAnimatedMesh* mesh, ISceneNode* parent, ISceneManager* mgr,
               s32 id, const std::string& debug_name,
               const core::vector3df& position = core::vector3df(),
               const core::vector3df& rotation = core::vector3df(),
               const core::vector3df& scale = core::vector3df(1, 1, 1),
               RenderInfo* render_info = NULL);
    // ------------------------------------------------------------------------
    ~SPMeshNode();
    // ------------------------------------------------------------------------
    virtual void render() {}
    // ------------------------------------------------------------------------
    virtual void setMesh(irr::scene::IAnimatedMesh* mesh);
    // ------------------------------------------------------------------------
    virtual void OnAnimate(u32 time_ms);
    // ------------------------------------------------------------------------
    virtual irr::scene::IMesh* getMeshForCurrentFrame(SkinningCallback sc = NULL,
                                                      int offset = -1);
    // ------------------------------------------------------------------------
    virtual IBoneSceneNode* getJointNode(const c8* joint_name);
    // ------------------------------------------------------------------------
    virtual IBoneSceneNode* getJointNode(u32 joint_id)
    {
        // SPM uses joint_name only
        assert(false);
        return NULL;
    }
    // ------------------------------------------------------------------------
    virtual void setJointMode(E_JOINT_UPDATE_ON_RENDER mode) {}
    // ------------------------------------------------------------------------
    int getTotalJoints() const;
    // ------------------------------------------------------------------------
    void setSkinningOffset(int offset)  { m_skinning_offset = offset; }
    // ------------------------------------------------------------------------
    void setAnimationState(bool val);
};

}

#endif
