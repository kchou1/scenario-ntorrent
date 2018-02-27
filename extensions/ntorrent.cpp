/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 */

#include "ntorrent.hpp"

namespace ndn {

NTorrent::NTorrent()
{
}

void
NTorrent::setSyncPrefix()
{
}

void
NTorrent::setUserPrefix()
{
}

void
NTorrent::setRoutingPrefix()
{
}

void
NTorrent::delayedInterest()
{
  std::cout << "Delayed Interest\n";
}

void
NTorrent::publishDataPeriodically()
{
}


void
NTorrent::printData()
{
  std::cout << "Data received" << "\n";
}


void
NTorrent::processSyncUpdate()
{
  std::cout << "Process Sync Update \n";
}

void
NTorrent::initializeSync()
{
  std::cout << "NTorrent Instance Initialized \n";
}

void
NTorrent::run()
{
}

void
NTorrent::runPeriodically()
{
}

} // namespace ndn
