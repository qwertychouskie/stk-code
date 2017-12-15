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
#include "graphics/sp/sp_texture_manager.hpp"
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
SPTexture::SPTexture(const std::string& path)
         : m_path(path), m_width(0), m_height(0)
{
#ifndef SERVER_ONLY
    glGenTextures(1, &m_texture_name);
#endif
    createWhite(false/*private_init*/);

    m_img_loader = irr_driver->getVideoDriver()->getImageLoaderForFile
        (path.c_str());
    if (m_img_loader == NULL)
    {
        Log::error("SPTexture", "No image loader for %s", path.c_str());
    }
}   // SPTexture

// ----------------------------------------------------------------------------
SPTexture::SPTexture(bool white)
         : m_width(0), m_height(0)
{
#ifndef SERVER_ONLY
    glGenTextures(1, &m_texture_name);
    if (white)
    {
        createWhite();
    }
    else
    {
        createTransparent();
    }
    addTextureHandle();
#endif
}   // SPTexture

// ----------------------------------------------------------------------------
SPTexture::~SPTexture()
{
#ifndef SERVER_ONLY
#ifndef USE_GLES2
    if (m_texture_handle != 0 && CVS->isARBBindlessTextureUsable())
    {
        glMakeTextureHandleNonResidentARB(m_texture_handle);
    }
#endif
    if (m_texture_name != 0)
    {
        glDeleteTextures(1, &m_texture_name);
    }
#endif
}   // ~SPTexture

// ----------------------------------------------------------------------------
void SPTexture::addTextureHandle()
{
#ifndef USE_GLES2
    if (CVS->isARBBindlessTextureUsable())
    {
        m_texture_handle = glGetTextureSamplerHandleARB(m_texture_name,
            getSampler(ST_TRILINEAR));
        glMakeTextureHandleResidentARB(m_texture_handle);
    }
#endif
}   // addTextureHandle

// ----------------------------------------------------------------------------
std::shared_ptr<video::IImage> SPTexture::getImageBuffer() const
{
#ifndef SERVER_ONLY
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

    core::dimension2du img_size = image->getDimension();
    core::dimension2du tex_size = img_size.getOptimalSize
        (!irr_driver->getVideoDriver()->queryFeature(video::EVDF_TEXTURE_NPOT));
    const core::dimension2du& max_size = irr_driver->getVideoDriver()
        ->getDriverAttributes().getAttributeAsDimension2d("MAX_TEXTURE_SIZE");

    if (tex_size.Width > max_size.Width)
    {
        tex_size.Width = max_size.Width;
    }
    if (tex_size.Height > max_size.Height)
    {
        tex_size.Height = max_size.Height;
    }
    if (image->getColorFormat() != video::ECF_A8R8G8B8 ||
        tex_size != img_size)
    {
        video::IImage* new_texture = irr_driver
            ->getVideoDriver()->createImage(video::ECF_A8R8G8B8, tex_size);
        if (tex_size != img_size)
            image->copyToScaling(new_texture);
        else
            image->copyTo(new_texture);
        image->drop();
        image = new_texture;
    }
    assert(image->getReferenceCount() == 1);
    return std::shared_ptr<video::IImage>(image);
#endif
}   // getImageBuffer

// ----------------------------------------------------------------------------
void SPTexture::initMaterial(Material* m)
{
}

// ----------------------------------------------------------------------------
void SPTexture::threadLoaded()
{
    std::shared_ptr<video::IImage> image = getImageBuffer();
    SPTextureManager::get()->increaseGLCommandFunctionCount(1);
    SPTextureManager::get()->addGLCommandFunction([this, image]()->bool
        {
            if (image)
            {
                glBindTexture(GL_TEXTURE_2D, m_texture_name);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    image->getDimension().Width, image->getDimension().Height,
                    0, GL_BGRA, GL_UNSIGNED_BYTE, image->lock());
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            addTextureHandle();
            if (image)
            {
                m_width.store(image->getDimension().Width);
                m_height.store(image->getDimension().Height);
            }
            else
            {
                m_width.store(2);
                m_height.store(2);
            }
            return true;
        });
}   // threadLoaded

}
