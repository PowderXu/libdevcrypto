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
/** @file Common.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <libdevcore/Guards.h>  // <boost/thread> conflicts with <thread>
#include "Common.h"
#include <secp256k1-vrf.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>
#include <hash_impl.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/RLP.h>
using namespace std;
using namespace dev;

namespace
{

secp256k1_context const* getCtx()
{
    static std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)> s_ctx{
        secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
        &secp256k1_context_destroy
    };
    return s_ctx.get();
}

template <std::size_t KeySize>
bool toPublicKey(Secret const& _secret, unsigned _flags, array<byte, KeySize>& o_serializedPubkey)
{
    auto* ctx = getCtx();
    secp256k1_pubkey rawPubkey;
    // Creation will fail if the secret key is invalid.
    if (!secp256k1_ec_pubkey_create(ctx, &rawPubkey, _secret.data()))
        return false;
    size_t serializedPubkeySize = o_serializedPubkey.size();
    secp256k1_ec_pubkey_serialize(
        ctx, o_serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, _flags);
    assert(serializedPubkeySize == o_serializedPubkey.size());
    return true;
}
}

bool dev::SignatureStruct::isValid() const noexcept
{
    static const h256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"};
    static const h256 s_zero;

    return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
}

Public dev::toPublic(Secret const& _secret)
{
	std::array<byte, 65> serializedPubkey;
	if (!toPublicKey(_secret, SECP256K1_EC_UNCOMPRESSED, serializedPubkey))
		return{};

	// Expect single byte header of value 0x04 -- uncompressed public key.
	assert(serializedPubkey[0] == 0x04);

	// Create the Public skipping the header.
	return Public{ &serializedPubkey[1], Public::ConstructFromPointer };
}

Public dev::toPublic(PublicCompressed const& _publicCompressed)
{
	auto* ctx = getCtx();

	secp256k1_pubkey rawPubkey;
	if (!secp256k1_ec_pubkey_parse(
		ctx, &rawPubkey, _publicCompressed.data(), PublicCompressed::size))
		return{};

	std::array<byte, 65> serializedPubkey;
	auto serializedPubkeySize = serializedPubkey.size();
	secp256k1_ec_pubkey_serialize(
		ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
	assert(serializedPubkeySize == serializedPubkey.size());
	// Expect single byte header of value 0x04 -- uncompressed public key.
	assert(serializedPubkey[0] == 0x04);
	// Create the Public skipping the header.
	return Public{ &serializedPubkey[1], Public::ConstructFromPointer };
}

PublicCompressed dev::toPublicCompressed(Secret const& _secret)
{
	PublicCompressed serializedPubkey;
	if (!toPublicKey(_secret, SECP256K1_EC_COMPRESSED, serializedPubkey.asArray()))
		return{};

	// Expect single byte header of value 0x02 or 0x03 -- compressed public key.
	assert(serializedPubkey[0] == 0x02 || serializedPubkey[0] == 0x03);

	return serializedPubkey;
}

secp256k1_pubkey dev::toPubkey(Signature const& _sig, h256 const& _message)
{
    int v = _sig[64];
    if (v > 3)
        return {};

    auto* ctx = getCtx();
    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rawSig, _sig.data(), v))
        return {};

    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig, _message.data()))
        return {};

    return rawPubkey;
}

Address dev::toAddress(Public const& _public)
{
	return right160(sha3(_public.ref()));
}

Address dev::toAddress(Secret const& _secret)
{
	return toAddress(toPublic(_secret));
}

Address dev::toAddress(Address const& _from, u256 const& _nonce)
{
	return right160(sha3(rlpList(_from, _nonce)));
}

Public dev::recover(Signature const& _sig, h256 const& _message)
{
    int v = _sig[64];
    if (v > 3)
        return {};

    auto* ctx = getCtx();
    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rawSig, _sig.data(), v))
        return {};

    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig, _message.data()))
        return {};

    std::array<byte, 65> serializedPubkey;
    size_t serializedPubkeySize = serializedPubkey.size();
    secp256k1_ec_pubkey_serialize(
            ctx, serializedPubkey.data(), &serializedPubkeySize,
            &rawPubkey, SECP256K1_EC_UNCOMPRESSED
    );
    assert(serializedPubkeySize == serializedPubkey.size());
    // Expect single byte header of value 0x04 -- uncompressed public key.
    assert(serializedPubkey[0] == 0x04);
    // Create the Public skipping the header.
    return Public{&serializedPubkey[1], Public::ConstructFromPointer};
}

static const u256 c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");

Signature dev::sign(Secret const& _k, h256 const& _hash)
{
    auto* ctx = getCtx();
    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig, _hash.data(), _k.data(), nullptr, nullptr))
        return {};

    Signature s;
    int v = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, s.data(), &v, &rawSig);

    SignatureStruct& ss = *reinterpret_cast<SignatureStruct*>(&s);
    ss.v = static_cast<byte>(v);
    if (ss.s > c_secp256k1n / 2)
    {
        ss.v = static_cast<byte>(ss.v ^ 1);
        ss.s = h256(c_secp256k1n - u256(ss.s));
    }
    assert(ss.s <= c_secp256k1n / 2);
    return s;
}

bool dev::verify(Public const& _p, Signature const& _s, h256 const& _hash)
{
    // TODO: Verify w/o recovery (if faster).
    if (!_p)
        return false;
    return _p == recover(_s, _hash);
}

bool dev::verify(PublicCompressed const& _key, h512 const& _signature, h256 const& _hash)
{
    auto* ctx = getCtx();

    secp256k1_ecdsa_signature rawSig;
    if (!secp256k1_ecdsa_signature_parse_compact(ctx, &rawSig, _signature.data()))
        return false;

    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ec_pubkey_parse(ctx, &rawPubkey, _key.data(), PublicCompressed::size))
        return false;  // Invalid public key.

    return secp256k1_ecdsa_verify(ctx, &rawSig, _hash.data(), &rawPubkey);
}
