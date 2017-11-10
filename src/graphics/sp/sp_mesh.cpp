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

#include "graphics/sp_mesh_loader.hpp"

#include "graphics/sp/sp_mesh.hpp"
#include "graphics/sp/sp_mesh_buffer.hpp"
#include "utils/mini_glm.hpp"

// ----------------------------------------------------------------------------
SPMesh::SPMesh()
{

}   // SPMesh
// ----------------------------------------------------------------------------
SPMesh::~SPMesh()
{
    for (unsigned i=0; i < m_buffer.size(); i++)
    {
        if (m_buffer[i])
        {
            m_buffer[i]->drop();
        }
    }
}   // ~SPMesh
