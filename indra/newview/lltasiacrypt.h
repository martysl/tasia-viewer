/**
 * TasiaCrypt — E2E encrypted IM between Tasia viewers
 * Uses X25519 key exchange + AES-256-GCM
 */
#ifndef LL_TASIACRYPT_H
#define LL_TASIACRYPT_H

#include "lluuid.h"
#include <string>
#include <map>

class LLTasiaCrypt
{
public:
    static LLTasiaCrypt& instance();

    // Check if a user has Tasia and we have a shared key
    bool hasKeyFor(const LLUUID& agent_id) const;

    // Get or create shared key for a user
    bool ensureKeyFor(const LLUUID& agent_id);

    // Encrypt plaintext -> base64 ciphertext
    // Returns empty string on failure
    std::string encrypt(const LLUUID& agent_id, const std::string& plaintext);

    // Decrypt base64 ciphertext -> plaintext
    // Returns empty string on failure
    std::string decrypt(const LLUUID& agent_id, const std::string& ciphertext);

    // Handle received public key from another Tasia user
    void handlePublicKey(const LLUUID& agent_id, const std::string& public_key_base64);

    // Get our public key to send
    std::string getPublicKeyBase64() const;

    // Save/load keys from disk
    void loadKeys();
    void saveKeys() const;

    // Check if a message looks TasiaCrypt encrypted
    static bool isTasiaCryptMessage(const std::string& message);

private:
    LLTasiaCrypt();
    ~LLTasiaCrypt() = default;

    struct SessionKey {
        unsigned char key[32];       // AES-256 key
        unsigned char their_pub[32]; // Their public key (for storage)
        bool valid;
    };

    std::string mKeysPath;
    unsigned char mOurPriv[32];
    unsigned char mOurPub[32];
    bool mKeysInitialized;
    std::map<LLUUID, SessionKey> mSessions;

    void generateKeypair();
    void deriveSharedKey(const unsigned char* their_pub, unsigned char* shared_key);
};

#endif
