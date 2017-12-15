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

#ifndef HEADER_SP_TEXTURE_MANAGER_HPP
#define HEADER_SP_TEXTURE_MANAGER_HPP

#include "graphics/gl_headers.hpp"
#include "utils/log.hpp"
#include "utils/no_copy.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>

namespace SP
{
class SPTexture;

class SPTextureManager : public NoCopy
{
private:
    static SPTextureManager* m_sptm;

    std::unordered_map<std::string, std::shared_ptr<SPTexture> > m_textures;

    std::atomic_uint m_max_threaded_load_obj;

    std::atomic_int m_subimage_function_count;

    std::list<std::function<void()> > m_threaded_functions;

    std::list<std::function<void()> > m_subimage_functions;

    std::mutex m_thread_obj_mutex, m_subimage_mutex;

    std::condition_variable m_thread_obj_cv;

    std::list<std::thread> m_threaded_load_obj;

public:
    // ------------------------------------------------------------------------
    static SPTextureManager* get()
    {
        if (m_sptm == NULL)
        {
            m_sptm = new SPTextureManager();
        }
        return m_sptm;
    }
    // ------------------------------------------------------------------------
    static void destroy()
    {
        delete m_sptm;
        m_sptm = NULL;
    }
    // ------------------------------------------------------------------------
    SPTextureManager();
    // ------------------------------------------------------------------------
    ~SPTextureManager();
    // ------------------------------------------------------------------------
    void removeUnusedTextures()
    {
        for (auto it = m_textures.begin(); it != m_textures.end();)
        {
            if (it->second.use_count() == 1)
            {
                it = m_textures.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
    // ------------------------------------------------------------------------
    void addThreadedFunction(std::function<void()> threaded_function)
    {
        std::lock_guard<std::mutex> lock(m_thread_obj_mutex);
        m_threaded_functions.push_back(threaded_function);
        m_thread_obj_cv.notify_one();
    }
    // ------------------------------------------------------------------------
    void addSubImageFunction(std::function<void()> function)
    {
        std::lock_guard<std::mutex> lock(m_subimage_mutex);
        m_subimage_functions.push_back(function);
    }
    // ------------------------------------------------------------------------
    void addSubImageFunctionCount(int count)
                                { m_subimage_function_count.fetch_add(count); }
    // ------------------------------------------------------------------------
    void checkForSubImage(bool before_scene = false);
    // ------------------------------------------------------------------------
    std::shared_ptr<SPTexture> getTexture(const std::string& path);
};

}

#endif
