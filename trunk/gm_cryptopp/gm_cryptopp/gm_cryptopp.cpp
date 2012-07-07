#include "gm_cryptopp.h"

// server's private key, for decryption of a certain steam feature
//int *serverprivatekey_length = (int *)0x30291394;
//byte *serverprivatekey = *(byte **)0x30291398;

using namespace CryptoPP;

GMOD_MODULE(Start, Close)

template<class T>
class Hash_Boilerplate
{
public:
	Hash_Boilerplate(ILuaInterface *gLua)
	{

		gLua->CheckType(1, GLua::TYPE_STRING);


		byte Digest[T::DIGESTSIZE];
		T().CalculateDigest((byte *)&Digest, (const byte *)gLua->GetString(1), gLua->StringLength(1));

		gLua->Push("Test");

		std::string output;
		ArraySource(Digest, sizeof(Digest), true, new HexEncoder(new StringSink(output)));

		gLua->Push(output.c_str());
	}
};

LUA_FUNCTION(crypto_md5)
{
	Hash_Boilerplate<Weak1::MD5> hash(Lua());
	return 1;
}

LUA_FUNCTION(crypto_sha1)
{
	Hash_Boilerplate<SHA1> hash(Lua());
	return 1;
}

LUA_FUNCTION(crypto_sha256)
{
	Hash_Boilerplate<SHA256> hash(Lua());
	return 1;
}

int Start(lua_State *L)
{
	ILuaInterface *gLua = Lua();

/*
	RSA::PrivateKey privKey;
	StringSource privateKey( serverprivatekey, *serverprivatekey_length, true);

	try {
		privKey.BERDecode(privateKey);
	} catch(...)
	{
		std::cerr << "Private key error" << std::endl;
	}
*/
	ILuaObject *table = gLua->GetNewTable();
		table->SetMember("md5", crypto_md5);
		table->SetMember("sha1", crypto_sha1);
		table->SetMember("sha256", crypto_sha256);
	gLua->SetGlobal("crypto", table);

	return 0;
}

int Close(lua_State *L)
{
	ILuaInterface *gLua = Lua();

	return 0;
}