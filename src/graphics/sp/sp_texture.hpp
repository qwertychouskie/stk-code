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

#ifndef HEADER_SP_TEXTURE_HPP
#define HEADER_SP_TEXTURE_HPP

#include "graphics/gl_headers.hpp"
#include "utils/log.hpp"
#include "utils/no_copy.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <string>

namespace irr
{
    namespace video { class IImageLoader; class IImage; }
}

class Material;

using namespace irr;

namespace SP
{

class SPTexture : public NoCopy
{
private:
    std::string m_path;

    video::IImageLoader* m_img_loader = NULL;

    uint64_t m_texture_handle = 0;

    GLuint m_texture_name = 0;

    std::atomic_uint m_width;

    std::atomic_uint m_height;

    bool m_colorized = false;

    bool m_alpha_mask_applied = false;

    // ------------------------------------------------------------------------
    void addTextureHandle();
    // ------------------------------------------------------------------------
    void createTransparent()
    {
#ifndef SERVER_ONLY
        glBindTexture(GL_TEXTURE_2D, m_texture_name);
        static uint32_t data[4] = { 0, 0, 0, 0 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, data);
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 1, 1, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_width.store(2);
        m_height.store(2);
#endif
    }
    // ------------------------------------------------------------------------
    void createWhite(bool private_init = true)
    {
#ifndef SERVER_ONLY
        glBindTexture(GL_TEXTURE_2D, m_texture_name);
        static int32_t data[4] = { -1, -1, -1, -1 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, data);
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 1, 1, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (private_init)
        {
            m_width.store(2);
            m_height.store(2);
        }
        else
        {
            m_width.store(0);
            m_height.store(0);
        }
#endif
    }
    // ------------------------------------------------------------------------
    SPTexture(bool white);

public:
    // ------------------------------------------------------------------------
    static std::shared_ptr<SPTexture> getWhiteTexture()
    {
        SPTexture* tex = new SPTexture(true/*white*/);
        return std::shared_ptr<SPTexture>(tex);
    }
    // ------------------------------------------------------------------------
    static std::shared_ptr<SPTexture> getTransparentTexture()
    {
        SPTexture* tex = new SPTexture(false/*white*/);
        return std::shared_ptr<SPTexture>(tex);
    }
    // ------------------------------------------------------------------------
    SPTexture(const std::string& path);
    // ------------------------------------------------------------------------
    ~SPTexture();
    // ------------------------------------------------------------------------
    const std::string& getPath() const                       { return m_path; }
    // ------------------------------------------------------------------------
    video::IImageLoader* getImageLoader() const        { return m_img_loader; }
    // ------------------------------------------------------------------------
    std::shared_ptr<video::IImage> getImageBuffer() const;
    // ------------------------------------------------------------------------
    GLuint getOpenGLTextureName() const              { return m_texture_name; }
    // ------------------------------------------------------------------------
    uint64_t getTextureHandle() const              { return m_texture_handle; }
    // ------------------------------------------------------------------------
    const uint64_t* getTextureHandlePointer() const
    {
        assert(m_texture_handle != 0);
        return &m_texture_handle;
    }
    // ------------------------------------------------------------------------
    void initMaterial(Material* m);
    // ------------------------------------------------------------------------
    void threadLoaded();

};

}

#endif
