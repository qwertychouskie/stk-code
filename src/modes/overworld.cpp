//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 SuperTuxKart-Team
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

#include "modes/overworld.hpp"


//-----------------------------------------------------------------------------
OverWorld::OverWorld() : LinearWorld()
{
}

// ----------------------------------------------------------------------------
/** Actually initialises the world, i.e. creates all data structures to
 *  for all karts etc. In init functions can be called that use
 *  World::getWorld().
 */
void OverWorld::init()
{
    LinearWorld::init();
    m_display_rank = false;
}   // init

//-----------------------------------------------------------------------------
OverWorld::~OverWorld()
{
}   // ~OverWorld

//-----------------------------------------------------------------------------
/** General update function called once per frame.
 *  \param dt Time step size.
 */
void OverWorld::update(float dt)
{
    LinearWorld::update(dt);
}   // update

//-----------------------------------------------------------------------------
/** Override the base class method to change behavior. We don't want wrong
 *  direction messages in the overworld since there is no direction there.
 *  \param i Kart id.
 */
void OverWorld::checkForWrongDirection(unsigned int i)
{
}   // checkForWrongDirection

//-----------------------------------------------------------------------------
