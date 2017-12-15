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

#include "graphics/sp/sp_texture_manager.hpp"
#include "graphics/sp/sp_base.hpp"
#include "graphics/sp/sp_texture.hpp"
#include "graphics/central_settings.hpp"
#include "utils/string_utils.hpp"
#include "utils/vs.hpp"

#include <ITexture.h>
#include <string>

namespace SP
{
SPTextureManager* SPTextureManager::m_sptm = NULL;
// ----------------------------------------------------------------------------
SPTextureManager::SPTextureManager()
                : m_max_threaded_load_obj
                  ((unsigned)std::thread::hardware_concurrency()),
                  m_gl_cmd_function_count(0)
{
    if (m_max_threaded_load_obj.load() == 0)
    {
        m_max_threaded_load_obj.store(2);
    }
    for (unsigned i = 0; i < m_max_threaded_load_obj; i++)
    {
        m_threaded_load_obj.emplace_back(
            [this, i]()->void
            {
                using namespace StringUtils;
                VS::setThreadName((toString(i) + "SPTM").c_str());
                while (true)
                {
                    std::unique_lock<std::mutex> ul(m_thread_obj_mutex);
                    m_thread_obj_cv.wait(ul, [this]
                        {
                            return !m_threaded_functions.empty();
                        });
                    if (m_max_threaded_load_obj == 0)
                    {
                        return;
                    }
                    std::function<void()> copied =
                        m_threaded_functions.front();
                    m_threaded_functions.pop_front();
                    ul.unlock();
                    copied();
                }
            });
    }
    m_textures["unicolor_white"] = SPTexture::getWhiteTexture();
    m_textures[""] = SPTexture::getTransparentTexture();
}   // SPTextureManager

// ----------------------------------------------------------------------------
SPTextureManager::~SPTextureManager()
{
    m_max_threaded_load_obj.store(0);
    std::unique_lock<std::mutex> ul(m_thread_obj_mutex);
    m_threaded_functions.push_back([]()->void {});
    m_thread_obj_cv.notify_all();
    ul.unlock();
    for (std::thread& t : m_threaded_load_obj)
    {
        t.join();
    }
    m_threaded_load_obj.clear();
}   // ~SPTextureManager

// ----------------------------------------------------------------------------
void SPTextureManager::checkForGLCommand(bool before_scene)
{
    if (m_gl_cmd_function_count.load() == 0)
    {
        return;
    }
    while (true)
    {
        std::unique_lock<std::mutex> ul(m_gl_cmd_mutex);
        if (m_gl_cmd_functions.empty())
        {
            if (before_scene && m_gl_cmd_function_count.load() != 0)
            {
                ul.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            else
            {
                return;
            }
        }
        std::function<bool()> gl_cmd = m_gl_cmd_functions.front();
        m_gl_cmd_functions.pop_front();
        ul.unlock();
        // if return false, re-added it to the back
        if (gl_cmd() == false)
        {
            std::lock_guard<std::mutex> lock(m_gl_cmd_mutex);
            m_gl_cmd_functions.push_back(gl_cmd);
            if (!before_scene)
            {
                return;
            }
        }
        else
        {
            m_gl_cmd_function_count.fetch_sub(1);
        }
    }
}   // checkForGLCommand

// ----------------------------------------------------------------------------
std::shared_ptr<SPTexture> SPTextureManager::getTexture(const std::string& p)
{
    checkForGLCommand();
    auto ret = m_textures.find(p);
    if (ret != m_textures.end())
    {
        return ret->second;
    }
    std::shared_ptr<SPTexture> t = std::make_shared<SPTexture>(p);
    addThreadedFunction(std::bind(&SPTexture::threadLoaded, t));
    m_textures[p] = t;
    return t;
}   // getTexture

}