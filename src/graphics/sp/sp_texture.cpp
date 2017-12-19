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
#include "graphics/material.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"

namespace SP
{
// ----------------------------------------------------------------------------
SPTexture::SPTexture(const std::string& path, Material* m, bool undo_srgb)
         : m_path(path), m_width(0), m_height(0), m_material(m),
           m_undo_srgb(undo_srgb)
{
#ifndef SERVER_ONLY
    glGenTextures(1, &m_texture_name);
#endif
    createWhite(false/*private_init*/);
}   // SPTexture

// ----------------------------------------------------------------------------
SPTexture::SPTexture(bool white)
         : m_width(0), m_height(0), m_undo_srgb(false)
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
std::shared_ptr<video::IImage> SPTexture::getImageFromPath
                                                (const std::string& path) const
{
    video::IImageLoader* img_loader =
        irr_driver->getVideoDriver()->getImageLoaderForFile(path.c_str());
    if (img_loader == NULL)
    {
        Log::error("SPTexture", "No image loader for %s", path.c_str());
    }

    io::IReadFile* file = irr::io::createReadFile(path.c_str());
    video::IImage* image = img_loader->loadImage(file);
    if (image == NULL || image->getDimension().Width == 0 ||
        image->getDimension().Height == 0)
    {
        Log::error("SPTexture", "Failed to load image %s", path.c_str());
        if (image)
        {
            image->drop();
        }
        if (file)
        {
            file->drop();
        }
        return NULL;
    }
    file->drop();
    assert(image->getReferenceCount() == 1);
    return std::shared_ptr<video::IImage>(image);
}   // getImagefromPath

// ----------------------------------------------------------------------------
std::shared_ptr<video::IImage> SPTexture::getTextureImage() const
{
    std::shared_ptr<video::IImage> image;
#ifndef SERVER_ONLY
    image = getImageFromPath(m_path);
    if (!image)
    {
        return NULL;
    }
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
        {
            image->copyToScaling(new_texture);
        }
        else
        {
            image->copyTo(new_texture);
        }
        assert(new_texture->getReferenceCount() == 1);
        image.reset(new_texture);
    }

    uint8_t* data = (uint8_t*)image->lock();
    for (unsigned int i = 0; i < image->getDimension().Width *
        image->getDimension().Height; i++)
    {
#ifdef USE_GLES2
        uint8_t tmp_val = data[i * 4];
        data[i * 4] = data[i * 4 + 2];
        data[i * 4 + 2] = tmp_val;
#endif
        if (m_undo_srgb)
        {
            data[i * 4] = srgbToLinear(data[i * 4] / 255.0f);
            data[i * 4 + 1] = srgbToLinear(data[i * 4 + 1] / 255.0f);
            data[i * 4 + 2] = srgbToLinear(data[i * 4 + 2] / 255.0f);
        }
    }
#endif
    return image;
}   // getTextureImage

// ----------------------------------------------------------------------------
bool SPTexture::threadedLoad()
{
#ifndef SERVER_ONLY
    std::shared_ptr<video::IImage> image = getTextureImage();
    std::shared_ptr<video::IImage> mask = getMask(image->getDimension());
    if (mask)
    {
        applyMask(image.get(), mask.get());
    }
    SPTextureManager::get()->increaseGLCommandFunctionCount(1);
    SPTextureManager::get()->addGLCommandFunction([this, image]()->bool
        {
            if (image)
            {
                glBindTexture(GL_TEXTURE_2D, m_texture_name);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    image->getDimension().Width, image->getDimension().Height,
                    0,
#ifdef USE_GLES2
                    GL_RGBA,
#else
                    GL_BGRA,
#endif
                    GL_UNSIGNED_BYTE, image->lock());
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
#endif
    return true;
}   // threadedLoad

// ----------------------------------------------------------------------------
std::shared_ptr<video::IImage>
    SPTexture::getMask(const core::dimension2du& s) const
{
#ifndef SERVER_ONLY
    if (!m_material)
    {
        return NULL;
    }
    const unsigned total_size = s.Width * s.Height;
    if (!m_material->getColorizationMask().empty() ||
        m_material->getColorizationFactor() > 0.0f ||
        m_material->isColorizable())
    {
        // Load colorization mask
        std::shared_ptr<video::IImage> mask;
        if (SP::getSPShader(m_material->getShaderName())->useAlphaChannel())
        {
            Log::debug("SPTexture", "Don't use colorization mask or factor"
                " with shader using alpha channel for %s", m_path.c_str());
            // Shader using alpha channel will be colorized as a whole
            return NULL;
        }

        uint8_t colorization_factor_encoded = uint8_t
            (irr::core::clamp(
            int(m_material->getColorizationFactor() * 0.4f * 255.0f), 0, 255));

        if (!m_material->getColorizationMask().empty())
        {
            // Assume all maskes are in the same directory
            std::string mask_path = StringUtils::getPath(m_path) + "/" +
                m_material->getColorizationMask();
            mask = getImageFromPath(mask_path);
            if (!mask)
            {
                return NULL;
            }
            core::dimension2du img_size = mask->getDimension();
            if (mask->getColorFormat() != video::ECF_A8R8G8B8 ||
                s != img_size)
            {
                video::IImage* new_mask = irr_driver
                    ->getVideoDriver()->createImage(video::ECF_A8R8G8B8, s);
                if (s != img_size)
                {
                    mask->copyToScaling(new_mask);
                }
                else
                {
                    mask->copyTo(new_mask);
                }
                assert(new_mask->getReferenceCount() == 1);
                mask.reset(new_mask);
            }
        }
        else
        {
            video::IImage* tmp = irr_driver
                ->getVideoDriver()->createImage(video::ECF_A8R8G8B8, s);
            memset(tmp->lock(), 0, total_size * 4);
            assert(tmp->getReferenceCount() == 1);
            mask.reset(tmp);
        }
        uint8_t* data = (uint8_t*)mask->lock();
        for (unsigned int i = 0; i < total_size; i++)
        {
            if (!m_material->getColorizationMask().empty()
                && data[i * 4 + 3] > 127)
            {
                continue;
            }
            data[i * 4 + 3] = colorization_factor_encoded;
        }
        return mask;
    }
    else if (!m_material->getAlphaMask().empty())
    {
        std::string mask_path = StringUtils::getPath(m_path) + "/" +
            m_material->getAlphaMask();
        std::shared_ptr<video::IImage> mask = getImageFromPath(mask_path);
        if (!mask)
        {
            return NULL;
        }
        core::dimension2du img_size = mask->getDimension();
        if (mask->getColorFormat() != video::ECF_A8R8G8B8 ||
            s != img_size)
        {
            video::IImage* new_mask = irr_driver
                ->getVideoDriver()->createImage(video::ECF_A8R8G8B8, s);
            if (s != img_size)
            {
                mask->copyToScaling(new_mask);
            }
            else
            {
                mask->copyTo(new_mask);
            }
            assert(new_mask->getReferenceCount() == 1);
            mask.reset(new_mask);
        }
        uint8_t* data = (uint8_t*)mask->lock();
        for (unsigned int i = 0; i < total_size; i++)
        {
            // Red channel to alpha channel
            data[i * 4 + 3] = data[i * 4];
        }
        return mask;
    }
#endif
    return NULL;
}   // getMask

// ----------------------------------------------------------------------------
void SPTexture::applyMask(video::IImage* texture, video::IImage* mask)
{
    assert(texture->getDimension() == mask->getDimension());
    const core::dimension2du& dim = texture->getDimension();
    for (unsigned int x = 0; x < dim.Width; x++)
    {
        for (unsigned int y = 0; y < dim.Height; y++)
        {
            video::SColor col = texture->getPixel(x, y);
            video::SColor alpha = mask->getPixel(x, y);
            col.setAlpha(alpha.getAlpha());
            texture->setPixel(x, y, col, false);
        }
    }
}   // applyMask

// ----------------------------------------------------------------------------
bool SPTexture::initialized() const
{
#ifndef SERVER_ONLY
    if (CVS->isARBBindlessTextureUsable())
    {
        return m_texture_handle != 0;
    }
#endif
    return m_width.load() != 0 && m_height.load() != 0;
}   // initialized

}
