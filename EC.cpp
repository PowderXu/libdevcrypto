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
/** @file EC.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * Shared EC classes and functions.
 */

#pragma warning(push)
#pragma warning(disable:4100 4244)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wextra"
#include <files.h>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "CryptoPP.h"
#include "SHA3.h"
#include "EC.h"

// CryptoPP and dev conflict so dev and pp namespace are used explicitly
using namespace std;
using namespace dev;
using namespace dev::crypto;
using namespace CryptoPP;

void crypto::encrypt(Public const& _key, bytes& io_cipher)
{
	ECIES<ECP>::Encryptor e;
	pp::initializeEncryptor(_key, e);
	size_t plen = io_cipher.size();
	bytes c;
	c.resize(e.CiphertextLength(plen));
	// todo: use StringSource with _plain as input and output.
	e.Encrypt(pp::PRNG, io_cipher.data(), plen, c.data());
	bzero(io_cipher.data(), io_cipher.size());
	io_cipher = std::move(c);
}

void crypto::decrypt(Secret const& _k, bytes& io_text)
{
	CryptoPP::ECIES<CryptoPP::ECP>::Decryptor d;
	pp::initializeDecryptor(_k, d);
	size_t clen = io_text.size();
	bytes p;
	p.resize(d.MaxPlaintextLength(io_text.size()));
	// todo: use StringSource with _c as input and output.
	DecodingResult r = d.Decrypt(pp::PRNG, io_text.data(), clen, p.data());
	assert(r.messageLength);
	io_text.resize(r.messageLength);
	io_text = std::move(p);
}

Signature crypto::sign(Secret const& _k, bytesConstRef _message)
{
	ECDSA<ECP, SHA3_256>::Signer signer;
	pp::initializeSigner(_k, signer);

	string sigstr;
	StringSource s(_message.toString(), true, new SignerFilter(pp::PRNG, signer, new StringSink(sigstr)));
	FixedHash<sizeof(Signature)> retsig((byte const*)sigstr.data(), Signature::ConstructFromPointer);
	
	/// eth signature: 65 bytes: r: [0, 32), s: [32, 64), v: 64.
	return std::move(retsig);
}

//Public crypto::recover(Signature _sig, bytesConstRef _message)
//{
//	
//}

bool crypto::verify(Public _p, Signature _sig, bytesConstRef _message)
{
	ECDSA<ECP, SHA3_256>::Verifier verifier;
	pp::initializeVerifier(_p, verifier);
	// cryptopp signatures are 64 bytes
	static_assert(sizeof(Signature) == 65, "Expected 65-byte signature.");
	return verifier.VerifyMessage(_message.data(), _message.size(), _sig.data(), sizeof(Signature) - 1);
}


