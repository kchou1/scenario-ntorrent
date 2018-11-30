/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "broadcast-strategy.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

const Name BroadcastStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/broadcast/%FD%01");
NFD_LOG_INIT("BroadcastStrategy");
NFD_REGISTER_STRATEGY(BroadcastStrategy);

BroadcastStrategy::BroadcastStrategy(Forwarder& forwarder, const Name& name)
    :Strategy(forwarder, name)
{
}

const Name&
BroadcastStrategy::getStrategyName()
{
  return STRATEGY_NAME;
}

void
BroadcastStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  std::string interestType = interest.getName().get(0).toUri();

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    // Choose Probabilty to forward (/beacon, /bitmap, /movie1)
    unsigned int forwarding = rand() % 100 + 1;
    Face& outFace = it->getFace();
    if (interestType == "beacon" || interestType == "bitmap" || interestType == "movie1" || interestType == "movie2") {
      if (!wouldViolateScope(inFace, interest, outFace) && forwarding > 50) {
        NFD_LOG_DEBUG("Forwarding Interest!! Probability Chosen: " << forwarding);
        this->sendInterest(pitEntry, outFace, interest);
      }
    }
  }

  NFD_LOG_DEBUG("NOT Forwarding!!");
  if (!hasPendingOutRecords(*pitEntry)) {
    this->rejectPendingInterest(pitEntry);
  }
}

} // namespace fw
} // namespace nfd
