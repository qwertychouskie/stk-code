//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2018 SuperTuxKart-Team
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

#include "graphics/sp/sp_texture.hpp"
#include "graphics/sp/sp_base.hpp"
#include "graphics/sp/sp_shader.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/irr_driver.hpp"
#include "utils/string_utils.hpp"

#include <ITexture.h>
#include <string>

namespace SP
{
// ----------------------------------------------------------------------------
SPTexture::SPTexture(const std::string& path) : m_path(path)
{
#ifndef SERVER_ONLY
    glGenTextures(1, &m_texture_name);
#endif
    createWhite();
    m_img_loader = irr_driver->getVideoDriver()->getImageLoaderForFile
        (path.c_str());
    if (m_img_loader == NULL)
    {
        Log::error("SPTexture", "No image loader for %s", path.c_str());
    }
}   // SPTexture

// ----------------------------------------------------------------------------
SPTexture::~SPTexture()
{
#ifndef SERVER_ONLY
    if (m_texture_name != 0)
    {
        glDeleteTextures(1, &m_texture_name);
    }
#ifndef USE_GLES2
    if (m_texture_handle != 0)
    {
        glMakeTextureHandleNonResidentARB(m_texture_handle);
    }
#endif
#endif
}   // ~SPTexture

// ----------------------------------------------------------------------------
uint64_t SPTexture::getTextureHandle()
{
#ifndef USE_GLES2
    if (m_texture_handle == 0)
    {
        m_texture_handle = glGetTextureSamplerHandleARB(m_texture_name,
            getSampler(ST_TRILINEAR));
    }
#endif
    return m_texture_handle;
}   // getTextureHandle

// ----------------------------------------------------------------------------
std::shared_ptr<video::IImage> SPTexture::getImageBuffer() const
{
    if (m_img_loader == NULL)
    {
        return NULL;
    }
    io::IReadFile* file = irr::io::createReadFile(m_path.c_str());
    video::IImage* image = m_img_loader->loadImage(file);
    if (image == NULL || image->getDimension().Width == 0 ||
        image->getDimension().Height == 0)
    {
        Log::error("SPTexture", "Failed to load image %s", m_path.c_str());
        if (image)
        {
            image->drop();
        }
        file->drop();
        return NULL;
    }
    file->drop();
    assert(image->getReferenceCount() == 1);
    return std::shared_ptr<video::IImage>(image);
}   // getImageBuffer

}
