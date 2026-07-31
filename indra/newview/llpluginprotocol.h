/**
 * @file llpluginprotocol.h
 * @brief Plugin communication protocol (JSON-RPC 2.0 style)
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINPROTOCOL_H
#define LL_LLPLUGINPROTOCOL_H

#include <string>
#include <functional>
#include <map>
#include "llsd.h"

/// Protocol version constant
constexpr S32 LLPLUGIN_PROTOCOL_VERSION = 1;
const std::string LLPLUGIN_PROTOCOL_SCHEME = "tasia://plugin-api/v1";

/// Maximum message size (256KB)
constexpr S32 LLPLUGIN_MAX_MESSAGE_BYTES = 1024 * 256;

/// Maximum method name length
constexpr S32 LLPLUGIN_MAX_METHOD_LENGTH = 128;

/// Maximum parameter nesting depth
constexpr S32 LLPLUGIN_MAX_PARAM_DEPTH = 16;

/**
 * A single JSON-RPC 2.0 style message in the plugin protocol.
 * Supports requests, responses, notifications, and events.
 */
class LLPluginMessage
{
public:
    LLPluginMessage();

    /// Factory: create a request message
    static LLPluginMessage request(const std::string& method, const LLSD& params = LLSD());

    /// Factory: create a success response
    static LLPluginMessage response(S64 id, const LLSD& result = LLSD());

    /// Factory: create an error response
    static LLPluginMessage error(S64 id, S64 code, const std::string& message, const LLSD& data = LLSD());

    /// Factory: create an event notification
    static LLPluginMessage event(const std::string& event_name, const LLSD& data = LLSD());

    /// Parse from JSON string
    bool parse(const std::string& json);

    /// Serialize to JSON string
    std::string serialize() const;

    // Accessors
    S64          getId()     const { return mId; }
    std::string  getMethod() const { return mMethod; }
    LLSD         getParams() const { return mParams; }
    LLSD         getResult() const { return mResult; }
    S64          getErrorCode() const { return mErrorCode; }
    std::string  getErrorMessage() const { return mErrorMessage; }
    LLSD         getErrorData() const { return mErrorData; }
    bool         isRequest()  const { return mIsRequest; }
    bool         isResponse() const { return mIsResponse; }
    bool         isError()    const { return mIsError; }
    bool         isEvent()    const { return mIsEvent; }
    bool         isValid()    const { return mValid; }

    /// Validate method name
    static bool isValidMethod(const std::string& method);

private:
    S64     mId = 0;
    std::string mMethod;
    LLSD    mParams;
    LLSD    mResult;
    S64     mErrorCode = 0;
    std::string mErrorMessage;
    LLSD    mErrorData;
    bool    mIsRequest  = false;
    bool    mIsResponse = false;
    bool    mIsError    = false;
    bool    mIsEvent    = false;
    bool    mValid      = false;
};

// Standard JSON-RPC error codes
namespace LLPluginError
{
    constexpr S64 PARSE_ERROR      = -32700;
    constexpr S64 INVALID_REQUEST  = -32600;
    constexpr S64 METHOD_NOT_FOUND = -32601;
    constexpr S64 INVALID_PARAMS   = -32602;
    constexpr S64 INTERNAL_ERROR   = -32603;

    // Plugin-specific errors (positive codes)
    constexpr S64 PERMISSION_DENIED  = 1;
    constexpr S64 RUNTIME_ERROR      = 2;
    constexpr S64 TIMEOUT            = 3;
    constexpr S64 NOT_IMPLEMENTED    = 4;
    constexpr S64 RATE_LIMITED       = 5;
    constexpr S64 MESSAGE_TOO_LARGE  = 6;
};

/// Transport abstraction - allows future serialization changes (MessagePack etc.)
class LLPluginTransport
{
public:
    virtual ~LLPluginTransport() = default;

    /// Serialize a message to wire format
    virtual std::string serialize(const LLPluginMessage& msg) = 0;

    /// Deserialize from wire format
    virtual LLPluginMessage deserialize(const std::string& data) = 0;

    /// Get content type identifier
    virtual std::string contentType() const = 0;
};

/// JSON transport implementation
class LLPluginJSONTransport : public LLPluginTransport
{
public:
    std::string serialize(const LLPluginMessage& msg) override;
    LLPluginMessage deserialize(const std::string& data) override;
    std::string contentType() const override { return "application/json"; }
};

#endif // LL_LLPLUGINPROTOCOL_H
