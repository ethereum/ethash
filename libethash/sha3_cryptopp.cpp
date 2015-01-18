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
/** @file sha3_cryptopp.h
 * @author Tim Hughes <tim@ethdev.org>
 * @date 2015
 */
#pragma once
#include "cryptopp/sha3.h"
#include "sha3_cryptopp.h"

void sha3_256(void* ret, void const* data, uint32_t size)
{
	CryptoPP::SHA3_256().CalculateDigest((uint8_t*)ret, (uint8_t const*)data, size);
}

void sha3_512(void* ret, void const* data, uint32_t size)
{
	CryptoPP::SHA3_512().CalculateDigest((uint8_t*)ret, (uint8_t const*)data, size);
}
