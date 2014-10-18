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
/** @file ECIES.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * ECIES Encrypt/Decrypt
 */

#pragma once

#pragma warning(push)
#pragma warning(disable:4100 4244)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wextra"
#include <eccrypto.h>
#include <ecp.h>
#include <files.h>
#include <filters.h>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include <libdevcore/Exceptions.h>
#include "Common.h"

namespace dev
{
namespace crypto
{

/**
 * @brief ECIES Encryption
 */
class ECIESEncryptor
{
public:
	ECIESEncryptor(ECKeyPair* _k);

	/// Encrypt _message. (object will be resized and replaced with cipher)
	void encrypt(bytes& _message);

private:
	CryptoPP::ECIES<CryptoPP::ECP>::Encryptor m_encryptor;
};

/**
 * @brief ECIES Decryption
 */
class ECIESDecryptor
{
public:
	ECIESDecryptor(ECKeyPair* _k);

	/// Decrypt cipher to plain.
	bytes decrypt(bytesConstRef& _c);

private:
	CryptoPP::ECIES<CryptoPP::ECP>::Decryptor m_decryptor;
};

}
}