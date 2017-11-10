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

#ifndef HEADER_SP_MESH_HPP
#define HEADER_SP_MESH_HPP


#include <ISkinnedMesh.h>
#include <vector>

using namespace irr;
using namespace scene;

class SPMeshBuffer;
class SPMeshLoader;

class SPMesh : public ISkinnedMesh
{
friend class SPMeshLoader;
private:
    std::vector<SPMeshBuffer*> m_buffer;

    core::aabbox3d<f32> m_bounding_box;

    float m_fps;

    float m_last_frame;

    bool m_skinned_last_frame;

    unsigned m_bind_frame, m_total_joints, m_joint_using, m_frame_count;

    std::vector<Armature> m_all_armatures;

public:
    // ------------------------------------------------------------------------
    SPMesh();
    // ------------------------------------------------------------------------
    virtual ~SPMesh();
    // ------------------------------------------------------------------------
    virtual u32 getFrameCount() const;
    // ------------------------------------------------------------------------
    virtual f32 getAnimationSpeed() const;
    // ------------------------------------------------------------------------
    virtual void setAnimationSpeed(f32 fps);
    // ------------------------------------------------------------------------
    virtual IMesh* getMesh(s32 frame, s32 detailLevel=255,
                           s32 startFrameLoop=-1, s32 endFrameLoop=-1)
                                                               { return this; }
    // ------------------------------------------------------------------------
    virtual void animateMesh(f32 frame, f32 blend);
    // ------------------------------------------------------------------------
    virtual void skinMesh(f32 strength=1.f, SkinningCallback sc = NULL,
                          int offset = -1) {}
    // ------------------------------------------------------------------------
    virtual u32 getMeshBufferCount() const;
    // ------------------------------------------------------------------------
    virtual IMeshBuffer* getMeshBuffer(u32 nr) const;
    // ------------------------------------------------------------------------
    virtual IMeshBuffer* getMeshBuffer( const video::SMaterial &material) const;
    // ------------------------------------------------------------------------
    virtual const core::aabbox3d<f32>& getBoundingBox() const;
    // ------------------------------------------------------------------------
    virtual void setBoundingBox( const core::aabbox3df& box);
    // ------------------------------------------------------------------------
    virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue) {}
    // ------------------------------------------------------------------------
    virtual void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint,
                                        E_BUFFER_TYPE buffer) {}
    // ------------------------------------------------------------------------
    virtual void setDirty(E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX) {}
    // ------------------------------------------------------------------------
    virtual E_ANIMATED_MESH_TYPE getMeshType() const { }
    // ------------------------------------------------------------------------
    virtual u32 getJointCount() const;
    // ------------------------------------------------------------------------
    virtual const c8* getJointName(u32 number) const;
    // ------------------------------------------------------------------------
    virtual s32 getJointNumber(const c8* name) const;
    // ------------------------------------------------------------------------
    virtual bool useAnimationFrom(const ISkinnedMesh *mesh) { return true; }
    // ------------------------------------------------------------------------
    virtual void updateNormalsWhenAnimating(bool on) {}
    // ------------------------------------------------------------------------
    virtual void setInterpolationMode(E_INTERPOLATION_MODE mode) {}
    // ------------------------------------------------------------------------
    virtual void convertMeshToTangents(bool(*predicate)(IMeshBuffer*)) {}
    // ------------------------------------------------------------------------
    virtual bool isStatic() { return false; }
    // ------------------------------------------------------------------------
    virtual bool setHardwareSkinning(bool on) { return true; }
    // ------------------------------------------------------------------------
    virtual core::array<SSkinMeshBuffer*> &getMeshBuffers()
                              { return *(core::array<SSkinMeshBuffer*>*)NULL; }
    // ------------------------------------------------------------------------
    virtual core::array<SJoint*> &getAllJoints();
    // ------------------------------------------------------------------------
    virtual const core::array<SJoint*> &getAllJoints() const;
    // ------------------------------------------------------------------------
    virtual void finalize() {}
    // ------------------------------------------------------------------------
    virtual SSkinMeshBuffer *addMeshBuffer() { return NULL; }
    // ------------------------------------------------------------------------
    virtual SJoint *addJoint(SJoint *parent) { return NULL; }
    // ------------------------------------------------------------------------
    virtual SPositionKey *addPositionKey(SJoint *joint) { return NULL; }
    // ------------------------------------------------------------------------
    virtual SRotationKey *addRotationKey(SJoint *joint) { return NULL; }
    // ------------------------------------------------------------------------
    virtual SScaleKey *addScaleKey(SJoint *joint) { return NULL; }
    // ------------------------------------------------------------------------
    virtual SWeight *addWeight(SJoint *joint) { return NULL; }
    // ------------------------------------------------------------------------
    virtual void updateBoundingBox(void);
    // ------------------------------------------------------------------------
    void recoverJointsFromMesh(core::array<IBoneSceneNode*> &jointChildSceneNodes);
    // ------------------------------------------------------------------------
    void transferJointsToMesh(const core::array<IBoneSceneNode*> &jointChildSceneNodes);
    // ------------------------------------------------------------------------
    void transferOnlyJointsHintsToMesh(const core::array<IBoneSceneNode*> &jointChildSceneNodes);
    // ------------------------------------------------------------------------
    void addJoints(core::array<IBoneSceneNode*> &jointChildSceneNodes,
                   IAnimatedMeshSceneNode* node,
                   ISceneManager* smgr);
    // ------------------------------------------------------------------------

};
#endif
