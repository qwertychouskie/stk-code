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

#ifndef HEADER_SP_BASE_HPP
#define HEADER_SP_BASE_HPP

#include "graphics/gl_headers.hpp"
#include "utils/no_copy.hpp"

#include <functional>
#include <ostream>
#include <memory>
#include <string>
#include <vector>

namespace irr
{
    namespace scene { class ICameraSceneNode; }
    namespace video { class ITexture; }
}


class ShadowMatrices;

namespace SP
{

enum DrawCallType: unsigned int
{
    DCT_NORMAL = 0,
    DCT_TRANSPARENT,
    DCT_SHADOW1,
    DCT_SHADOW2,
    DCT_SHADOW3,
    DCT_SHADOW4,
    DCT_RSM,
    DCT_GLOW,
    DCT_COUNT
};

inline std::ostream& operator<<(std::ostream& os, const DrawCallType& dct)
{
    switch (dct)
    {
        case DCT_NORMAL:
            return os << "normal";
        case DCT_TRANSPARENT:
            return os << "transparent";
        case DCT_SHADOW1:
            return os << "shadow cam 1";
        case DCT_SHADOW2:
            return os << "shadow cam 2";
        case DCT_SHADOW3:
            return os << "shadow cam 3";
        case DCT_SHADOW4:
            return os << "shadow cam 4";
        case DCT_RSM:
            return os << "reflective shadow map";
        case DCT_GLOW:
            return os << "glow";
        default:
            return os;
    }
}

enum SamplerType: unsigned int;
enum RenderPass: unsigned int;
class SPDynamicDrawCall;
class SPMaterial;
class SPMeshNode;
class SPShader;
class SPMeshBuffer;

extern GLuint sp_mat_ubo;
extern GLuint sp_fog_ubo;
extern std::vector<GLuint> sp_prefilled_tex;
extern unsigned sp_solid_poly_count;
extern unsigned sp_shadow_poly_count;
extern unsigned sp_draw_call_count;
extern int sp_cur_shadow_cascade;
extern bool sp_vc_srgb_cor;
extern bool sp_null_device;
extern std::function<void(SPMeshBuffer*)> sp_mb_upload_cb;
extern bool sp_culling;

// ----------------------------------------------------------------------------
inline void setPrefilledTextures(const std::vector<GLuint>& tex)
{
    sp_prefilled_tex[0] = tex[0];
    sp_prefilled_tex[1] = tex[1];
    sp_prefilled_tex[2] = tex[2];
    sp_prefilled_tex[3] = tex[3];
}
// ----------------------------------------------------------------------------
inline void setVertexColorSRGBCorrection(bool val)
{
    sp_vc_srgb_cor = val;
}
// ----------------------------------------------------------------------------
void init();
// ----------------------------------------------------------------------------
void addShader(SPShader*);
// ----------------------------------------------------------------------------
void destroy();
// ----------------------------------------------------------------------------
GLuint getSampler(SamplerType);
// ----------------------------------------------------------------------------
std::shared_ptr<SPMaterial> addSPMaterial(const std::shared_ptr<SPMaterial>&);
// ----------------------------------------------------------------------------
void destroySPMaterials();
// ----------------------------------------------------------------------------
void initNullSPMaterial();
// ----------------------------------------------------------------------------
SPShader* getGlowShader();
// ----------------------------------------------------------------------------
SPShader* getSPShader(const std::string& name);
// ----------------------------------------------------------------------------
std::shared_ptr<SPMaterial> getSPMaterial(irr::video::ITexture* l1,
                                          irr::video::ITexture* l2 = NULL);
// ----------------------------------------------------------------------------
std::shared_ptr<SPMaterial> getSPNullMaterial();
// ----------------------------------------------------------------------------
SPMaterial* getSPMaterial(const std::string& name);
// ----------------------------------------------------------------------------
void prepareDrawCalls(const irr::scene::ICameraSceneNode*,
                      bool shadow = false);
// ----------------------------------------------------------------------------
void draw(RenderPass, DrawCallType dct = DCT_NORMAL);
// ----------------------------------------------------------------------------
void addGlobalSector();
// ----------------------------------------------------------------------------
void destroySectors();
// ----------------------------------------------------------------------------
bool destroyingSectors();
// ----------------------------------------------------------------------------
void drawBoundingBoxes();
// ----------------------------------------------------------------------------
void addObject(SPMeshNode*, bool to_global_sector = false);
// ----------------------------------------------------------------------------
void removeObject(SPMeshNode*);
// ----------------------------------------------------------------------------
void removeAllObjects();
// ----------------------------------------------------------------------------
void cleanAllMeshBuffer();
// ----------------------------------------------------------------------------
void updateTransformation();
// ----------------------------------------------------------------------------
void initSTKShadowMatrices(ShadowMatrices*);
// ----------------------------------------------------------------------------
void prepareScene();
// ----------------------------------------------------------------------------
void unsynchronisedUpdate();
// ----------------------------------------------------------------------------
void addDynamicDrawCall(SPDynamicDrawCall*);
// ----------------------------------------------------------------------------
void removeDynamicDrawCall(SPDynamicDrawCall*);
// ----------------------------------------------------------------------------
void update();

}


#endif
