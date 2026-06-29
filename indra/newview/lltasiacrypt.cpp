/**
 * TasiaCrypt — E2E encrypted IM between Tasia viewers
 * X25519 key agreement + AES-256-GCM
 */
#include "lltasiacrypt.h"
#include "lluuid.h"
#include "lldir.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llerror.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

static const char TASIACRYPT_MAGIC[] = "TC01";

static std::string bin_to_base64(const unsigned char* data, size_t len)
{
    if (!data || len == 0) return "";
    // Simple base64 using OpenSSL
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);
    BIO_write(b64, data, (int)len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length - 1); // remove newline
    BIO_free_all(b64);
    return result;
}

static std::vector<unsigned char> base64_to_bin(const std::string& input)
{
    if (input.empty()) return {};
    BIO *bio, *b64;
    bio = BIO_new_mem_buf(input.data(), (int)input.size());
    b64 = BIO_new(BIO_f_base64());
    BIO_push(b64, bio);
    std::vector<unsigned char> result(input.size(), 0);
    int len = BIO_read(b64, result.data(), (int)result.size());
    BIO_free_all(b64);
    if (len > 0) {
        result.resize(len);
        return result;
    }
    return {};
}

static std::string hex_encode(const unsigned char* data, size_t len)
{
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

static std::vector<unsigned char> hex_decode(const std::string& hex)
{
    std::vector<unsigned char> result(hex.size() / 2);
    for (size_t i = 0; i < result.size(); ++i)
    {
        unsigned int byte;
        sscanf(hex.c_str() + i * 2, "%02x", &byte);
        result[i] = (unsigned char)byte;
    }
    return result;
}

LLTasiaCrypt& LLTasiaCrypt::instance()
{
    static LLTasiaCrypt inst;
    return inst;
}

LLTasiaCrypt::LLTasiaCrypt()
    : mKeysInitialized(false)
{
    std::string user_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "");
    mKeysPath = user_path + gDirUtilp->getDirDelimiter() + "tasiacrypt_keys.xml";
    loadKeys();

    if (!mKeysInitialized)
    {
        generateKeypair();
        mKeysInitialized = true;
        saveKeys();
    }
}

void LLTasiaCrypt::generateKeypair()
{
    // Use X25519 key generation via EVP
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!ctx) {
        LL_WARNS("TasiaCrypt") << "Failed to create X25519 context" << LL_ENDL;
        return;
    }
    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_keygen_init(ctx) <= 0 || EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        LL_WARNS("TasiaCrypt") << "X25519 keygen failed" << LL_ENDL;
        EVP_PKEY_CTX_free(ctx);
        return;
    }
    size_t pub_len = 32, priv_len = 32;
    EVP_PKEY_get_raw_public_key(pkey, mOurPub, &pub_len);
    EVP_PKEY_get_raw_private_key(pkey, mOurPriv, &priv_len);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    LL_INFOS("TasiaCrypt") << "Generated X25519 keypair" << LL_ENDL;
}

void LLTasiaCrypt::deriveSharedKey(const unsigned char* their_pub, unsigned char* shared_key)
{
    EVP_PKEY* our_pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, mOurPriv, 32);
    EVP_PKEY* their_pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, their_pub, 32);
    if (!our_pkey || !their_pkey) {
        EVP_PKEY_free(our_pkey);
        EVP_PKEY_free(their_pkey);
        return;
    }
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(our_pkey, NULL);
    if (!ctx || EVP_PKEY_derive_init(ctx) <= 0 || EVP_PKEY_derive_set_peer(ctx, their_pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(our_pkey);
        EVP_PKEY_free(their_pkey);
        return;
    }
    size_t key_len = 32;
    EVP_PKEY_derive(ctx, shared_key, &key_len);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(our_pkey);
    EVP_PKEY_free(their_pkey);
}

bool LLTasiaCrypt::hasKeyFor(const LLUUID& agent_id) const
{
    auto it = mSessions.find(agent_id);
    return it != mSessions.end() && it->second.valid;
}

bool LLTasiaCrypt::ensureKeyFor(const LLUUID& agent_id)
{
    return hasKeyFor(agent_id);
}

void LLTasiaCrypt::handlePublicKey(const LLUUID& agent_id, const std::string& public_key_base64)
{
    auto their_pub = base64_to_bin(public_key_base64);
    if (their_pub.size() != 32) {
        LL_WARNS("TasiaCrypt") << "Invalid public key from " << agent_id << LL_ENDL;
        return;
    }

    SessionKey sk;
    memcpy(sk.their_pub, their_pub.data(), 32);
    deriveSharedKey(sk.their_pub, sk.key);
    sk.valid = true;
    mSessions[agent_id] = sk;
    saveKeys();
    LL_INFOS("TasiaCrypt") << "Established shared key with " << agent_id << LL_ENDL;
}

std::string LLTasiaCrypt::encrypt(const LLUUID& agent_id, const std::string& plaintext)
{
    auto it = mSessions.find(agent_id);
    if (!hasKeyFor(agent_id)) return "";

    // AES-256-GCM
    unsigned char iv[12];
    RAND_bytes(iv, 12);
    unsigned char tag[16];

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, it->second.key, iv);

    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    int out_len = 0;
    EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, (const unsigned char*)plaintext.data(), (int)plaintext.size());
    int final_len = 0;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &final_len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);

    // Format: MAGIC + IV(12) + TAG(16) + CIPHERTEXT
    std::string result = TASIACRYPT_MAGIC;
    result += bin_to_base64(iv, 12);
    result += "|";
    result += bin_to_base64(tag, 16);
    result += "|";
    result += bin_to_base64(ciphertext.data(), out_len + final_len);

    return result;
}

std::string LLTasiaCrypt::decrypt(const LLUUID& agent_id, const std::string& ciphertext)
{
    auto it = mSessions.find(agent_id);
    if (!hasKeyFor(agent_id)) return "";

    // Verify magic
    if (ciphertext.compare(0, 4, TASIACRYPT_MAGIC) != 0) return "";

    // Parse: MAGIC + IV(12) + "|" + TAG(16) + "|" + DATA
    std::string rest = ciphertext.substr(4);
    size_t pipe1 = rest.find('|');
    size_t pipe2 = rest.rfind('|');
    if (pipe1 == std::string::npos || pipe2 == std::string::npos || pipe1 == pipe2) return "";

    auto iv = base64_to_bin(rest.substr(0, pipe1));
    auto tag = base64_to_bin(rest.substr(pipe1 + 1, pipe2 - pipe1 - 1));
    auto data = base64_to_bin(rest.substr(pipe2 + 1));

    if (iv.size() != 12 || tag.size() != 16 || data.empty()) return "";

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, it->second.key, iv.data());

    std::vector<unsigned char> plaintext(data.size());
    int out_len = 0;
    EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, data.data(), (int)data.size());

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data());
    int final_len = 0;
    int rc = EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &final_len);
    EVP_CIPHER_CTX_free(ctx);

    if (rc <= 0) {
        LL_WARNS("TasiaCrypt") << "Decryption failed (tampered? wrong key?)" << LL_ENDL;
        return "";
    }

    return std::string((const char*)plaintext.data(), out_len + final_len);
}

std::string LLTasiaCrypt::getPublicKeyBase64() const
{
    return bin_to_base64(mOurPub, 32);
}

bool LLTasiaCrypt::isTasiaCryptMessage(const std::string& message)
{
    return message.compare(0, 4, TASIACRYPT_MAGIC) == 0;
}

void LLTasiaCrypt::loadKeys()
{
    LLSD data;
    llifstream file(mKeysPath.c_str());
    if (!file.is_open()) return;
    LLSDSerialize::fromXML(data, file);
    file.close();

    if (data.has("priv") && data.has("pub"))
    {
        auto priv = hex_decode(data["priv"].asString());
        auto pub = hex_decode(data["pub"].asString());
        if (priv.size() == 32 && pub.size() == 32)
        {
            memcpy(mOurPriv, priv.data(), 32);
            memcpy(mOurPub, pub.data(), 32);
            mKeysInitialized = true;
        }
    }

    if (data.has("sessions"))
    {
        LLSD sessions = data["sessions"];
        for (LLSD::map_iterator it = sessions.beginMap(); it != sessions.endMap(); ++it)
        {
            LLUUID agent_id(it->first);
            std::string their_pub_hex = it->second["pub"].asString();
            auto their_pub = hex_decode(their_pub_hex);
            if (their_pub.size() == 32)
            {
                SessionKey sk;
                memcpy(sk.their_pub, their_pub.data(), 32);
                // Re-derive shared key from our private key and their public key
                deriveSharedKey(sk.their_pub, sk.key);
                sk.valid = true;
                mSessions[agent_id] = sk;
            }
        }
    }
}

void LLTasiaCrypt::saveKeys() const
{
    LLSD data;
    data["priv"] = hex_encode(mOurPriv, 32);
    data["pub"] = hex_encode(mOurPub, 32);

    LLSD sessions;
    for (const auto& [id, sk] : mSessions)
    {
        if (sk.valid)
        {
            sessions[id.asString()]["pub"] = hex_encode(sk.their_pub, 32);
        }
    }
    data["sessions"] = sessions;

    llofstream file(mKeysPath.c_str());
    if (file.is_open())
    {
        LLSDSerialize::toPrettyXML(data, file);
        file.close();
    }
}
