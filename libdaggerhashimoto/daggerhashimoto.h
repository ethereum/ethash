// -*-c-*-
/*
  This file is part of cpp-ethereum.

  cpp-ethereum is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  cpp-ethereum is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file daggerhashimoto.h
 * @author Matthew Wampler-Doty <matt@w-d.org>
 * @date 2014
 */

#ifndef DAGGERHASHIMOTO_H
#define DAGGERHASHIMOTO_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <ForceFeedback/ForceFeedback.h>

typedef struct {
    uint64_t n;              // Size of the dataset
    unsigned char diff[256]; // Difficulty (adjusted during block evaluation)
    int epoch_time;          // Length of an epoch in blocks (how often the dataset is updated)
    int n_inc;               // Increment in value of n per period epoch
    uint64_t cache_size;          // How big should the light client's cache be?
    int w;                   // Work factor for memory free mining
    int width;               // How much memory state to use in hashimoto
    int accesses;            // Number of dataset accesses during hashimoto
    int trials;              // Number of times to run hashimoto
  } parameters;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DAGGERHASHIMOTO_H
