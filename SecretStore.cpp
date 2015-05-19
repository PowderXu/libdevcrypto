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
/** @file SecretStore.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "SecretStore.h"
#include <thread>
#include <mutex>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include <test/JsonSpiritHeaders.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;
namespace fs = boost::filesystem;

SecretStore::SecretStore()
{
	load();
}

SecretStore::~SecretStore()
{
}

bytes SecretStore::secret(h128 const& _uuid, function<std::string()> const& _pass) const
{
	auto rit = m_cached.find(_uuid);
	if (rit != m_cached.end())
		return rit->second;
	auto it = m_keys.find(_uuid);
	if (it == m_keys.end())
		return bytes();
	bytes key = decrypt(it->second.first, _pass());
	if (!key.empty())
		m_cached[_uuid] = key;
	return key;
}

h128 SecretStore::importSecret(bytes const& _s, std::string const& _pass)
{
	h128 r = h128::random();
	m_cached[r] = _s;
	m_keys[r] = make_pair(encrypt(_s, _pass), std::string());
	save();
	return r;
}

void SecretStore::kill(h128 const& _uuid)
{
	m_cached.erase(_uuid);
	if (m_keys.count(_uuid))
	{
		boost::filesystem::remove(m_keys[_uuid].second);
		m_keys.erase(_uuid);
	}
}

void SecretStore::clearCache() const
{
	m_cached.clear();
}

void SecretStore::save(std::string const& _keysPath)
{
	fs::path p(_keysPath);
	boost::filesystem::create_directories(p);
	for (auto& k: m_keys)
	{
		std::string uuid = toUUID(k.first);
		std::string filename = (p / uuid).string() + ".json";
		js::mObject v;
		js::mValue crypto;
		js::read_string(k.second.first, crypto);
		v["crypto"] = crypto;
		v["id"] = uuid;
		v["version"] = 2;
		writeFile(filename, js::write_string(js::mValue(v), true));
		if (!k.second.second.empty() && k.second.second != filename)
			boost::filesystem::remove(k.second.second);
		k.second.second = filename;
	}
}

void SecretStore::load(std::string const& _keysPath)
{
	fs::path p(_keysPath);
	boost::filesystem::create_directories(p);
	js::mValue v;
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
		if (is_regular_file(it->path()))
		{
			cdebug << "Reading" << it->path();
			js::read_string(contentsString(it->path().string()), v);
			if (v.type() == js::obj_type)
			{
				js::mObject o = v.get_obj();
				int version = o.count("Version") ? stoi(o["Version"].get_str()) : o.count("version") ? o["version"].get_int() : 0;
				if (version == 2)
					m_keys[fromUUID(o["id"].get_str())] = make_pair(js::write_string(o["crypto"], false), it->path().string());
				else
					cwarn << "Cannot read key version" << version;
			}
//				else
//					cwarn << "Invalid JSON in key file" << it->path().string();
		}
}

std::string SecretStore::encrypt(bytes const& _v, std::string const& _pass)
{
	js::mObject ret;

	// KDF info
	unsigned dklen = 16;
	unsigned iterations = 262144;
	bytes salt = h256::random().asBytes();
	ret["kdf"] = "pbkdf2";
	{
		js::mObject params;
		params["prf"] = "hmac-sha256";
		params["c"] = (int)iterations;
		params["salt"] = toHex(salt);
		params["dklen"] = (int)dklen;
		ret["kdfparams"] = params;
	}
	bytes derivedKey = pbkdf2(_pass, salt, iterations, dklen);

	// cipher info
	ret["cipher"] = "aes-128-cbc";
	h128 key(sha3(h128(derivedKey, h128::AlignRight)), h128::AlignRight);
	h128 iv = h128::random();
	{
		js::mObject params;
		params["iv"] = toHex(iv.ref());
		ret["cipherparams"] = params;
	}

	// cipher text
	bytes cipherText = encryptSymNoAuth(key, iv, &_v);
	ret["ciphertext"] = toHex(cipherText);

	// and mac.
	h256 mac = sha3(bytesConstRef(&derivedKey).cropped(derivedKey.size() - 16).toBytes() + cipherText);
	ret["mac"] = toHex(mac.ref());

	return js::write_string((js::mValue)ret, true);
}

bytes SecretStore::decrypt(std::string const& _v, std::string const& _pass)
{
	js::mObject o;
	{
		js::mValue ov;
		js::read_string(_v, ov);
		o = ov.get_obj();
	}

	// derive key
	bytes derivedKey;
	if (o["kdf"].get_str() == "pbkdf2")
	{
		auto params = o["kdfparams"].get_obj();
		if (params["prf"].get_str() != "hmac-sha256")
		{
			cwarn << "Unknown PRF for PBKDF2" << params["prf"].get_str() << "not supported.";
			return bytes();
		}
		unsigned iterations = params["c"].get_int();
		bytes salt = fromHex(params["salt"].get_str());
		derivedKey = pbkdf2(_pass, salt, iterations, params["dklen"].get_int());
	}
	else
	{
		cwarn << "Unknown KDF" << o["kdf"].get_str() << "not supported.";
		return bytes();
	}

	bytes cipherText = fromHex(o["ciphertext"].get_str());

	// check MAC
	h256 mac(o["mac"].get_str());
	h256 macExp = sha3(bytesConstRef(&derivedKey).cropped(derivedKey.size() - 16).toBytes() + cipherText);
	if (mac != macExp)
	{
		cwarn << "Invalid key - MAC mismatch; expected" << toString(macExp) << ", got" << toString(mac);
		return bytes();
	}

	// decrypt
	if (o["cipher"].get_str() == "aes-128-cbc")
	{
		auto params = o["cipherparams"].get_obj();
		h128 key(sha3(h128(derivedKey, h128::AlignRight)), h128::AlignRight);
		h128 iv(params["iv"].get_str());
		return decryptSymNoAuth(key, iv, &cipherText);
	}
	else
	{
		cwarn << "Unknown cipher" << o["cipher"].get_str() << "not supported.";
		return bytes();
	}
}
