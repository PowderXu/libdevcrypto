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
/** @file CryptoPP.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * CryptoPP headers and helper methods
 */

#pragma once

// need to leave this one disabled
//#pragma GCC diagnostic ignored "-Wunused-function"
#pragma warning(push)
#pragma warning(disable:4100 4244)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wextra"
#include <sha.h>
#include <sha3.h>
#include <ripemd.h>
#include <aes.h>
#include <pwdbased.h>
#include <modes.h>
#include <filters.h>
#include <eccrypto.h>
#include <ecp.h>
#include <files.h>
#include <osrng.h>
#include <oids.h>
#include <secp256k1/secp256k1.h>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "Common.h"

namespace dev
{
namespace crypto
{

namespace pp
{
/// RNG used by CryptoPP
inline CryptoPP::AutoSeededRandomPool& PRNG() { static CryptoPP::AutoSeededRandomPool prng; return prng; }

/// EC curve used by CryptoPP
inline CryptoPP::OID const& secp256k1() { static CryptoPP::OID curve = CryptoPP::ASN1::secp256k1(); return curve; }

/// Conversion from bytes to cryptopp point
CryptoPP::ECP::Point PointFromPublic(Public const& _p);

/// Conversion from bytes to cryptopp exponent
CryptoPP::Integer ExponentFromSecret(Secret const& _s);
	
/// Conversion from cryptopp exponent Integer to bytes
void PublicFromExponent(CryptoPP::Integer const& _k, Public& _s);
	
/// Conversion from cryptopp public key to bytes
void PublicFromDL_PublicKey_EC(CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> const& _k, Public& _p);
	
/// Conversion from cryptopp private key to bytes
void SecretFromDL_PrivateKey_EC(CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> const& _k, Secret& _s);

}
}
}

