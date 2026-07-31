/**
 * @file llpluginprotocol.cpp
 * @brief Plugin communication protocol implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginprotocol.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llstring.h"
#include <regex>
#include <sstream>

LLPluginMessage::LLPluginMessage()
{
}

LLPluginMessage LLPluginMessage::request(const std::string& method, const LLSD& params)
{
    LLPluginMessage msg;
    msg.mMethod = method;
    msg.mParams = params;
    msg.mIsRequest = true;
    msg.mValid = true;
    return msg;
}

LLPluginMessage LLPluginMessage::response(S64 id, const LLSD& result)
{
    LLPluginMessage msg;
    msg.mId = id;
    msg.mResult = result;
    msg.mIsResponse = true;
    msg.mValid = true;
    return msg;
}

LLPluginMessage LLPluginMessage::error(S64 id, S64 code, const std::string& message, const LLSD& data)
{
    LLPluginMessage msg;
    msg.mId = id;
    msg.mErrorCode = code;
    msg.mErrorMessage = message;
    msg.mErrorData = data;
    msg.mIsError = true;
    msg.mValid = true;
    return msg;
}

LLPluginMessage LLPluginMessage::event(const std::string& event_name, const LLSD& data)
{
    LLPluginMessage msg;
    msg.mMethod = "event";
    LLSD params;
    params["name"] = event_name;
    params["data"] = data;
    msg.mParams = params;
    msg.mIsEvent = true;
    msg.mValid = true;
    return msg;
}

bool LLPluginMessage::parse(const std::string& json)
{
    LLSD data;
    std::istringstream stream(json);
    if (LLSDSerialize::fromJSON(data, stream, LLPLUGIN_MAX_MESSAGE_BYTES) == LLSDParser::PARSE_FAILURE)
    {
        return false;
    }

    // Check JSON-RPC version
    if (data.has("jsonrpc") && data["jsonrpc"].asString() != "2.0")
    {
        return false;
    }

    mMethod = data["method"].asString();
    mParams = data["params"];

    if (data.has("id"))
    {
        mId = data["id"].asInteger();
    }

    if (data.has("result"))
    {
        mResult = data["result"];
        mIsResponse = true;
    }
    else if (data.has("error"))
    {
        mErrorCode = data["error"]["code"].asInteger();
        mErrorMessage = data["error"]["message"].asString();
        mErrorData = data["error"]["data"];
        mIsError = true;
    }
    else if (!mMethod.empty())
    {
        if (data.has("id"))
        {
            mIsRequest = true;
        }
        else
        {
            if (mMethod == "event")
            {
                mIsEvent = true;
            }
            else
            {
                mIsRequest = true; // notification (no id)
            }
        }
    }

    mValid = true;
    return true;
}

std::string LLPluginMessage::serialize() const
{
    LLSD data;
    data["jsonrpc"] = "2.0";

    if (mIsRequest)
    {
        data["id"] = mId;
        data["method"] = mMethod;
        data["params"] = mParams;
    }
    else if (mIsResponse)
    {
        data["id"] = mId;
        data["result"] = mResult;
    }
    else if (mIsError)
    {
        data["id"] = mId;
        data["error"]["code"] = mErrorCode;
        data["error"]["message"] = mErrorMessage;
        if (!mErrorData.isUndefined())
        {
            data["error"]["data"] = mErrorData;
        }
    }
    else if (mIsEvent)
    {
        data["method"] = "event";
        data["params"]["name"] = mParams["name"];
        data["params"]["data"] = mParams["data"];
    }

    std::ostringstream stream;
    LLSDSerialize::toJSON(data, stream);
    return stream.str();
}

bool LLPluginMessage::isValidMethod(const std::string& method)
{
    if (method.empty() || method.length() > LLPLUGIN_MAX_METHOD_LENGTH)
        return false;

    // Must be dot-separated identifiers
    static const std::regex method_regex(R"(^[a-zA-Z][a-zA-Z0-9]*(\.[a-zA-Z][a-zA-Z0-9]*)*$)");
    return std::regex_match(method, method_regex);
}

// ---- LLPluginJSONTransport ----

std::string LLPluginJSONTransport::serialize(const LLPluginMessage& msg)
{
    return msg.serialize();
}

LLPluginMessage LLPluginJSONTransport::deserialize(const std::string& data)
{
    LLPluginMessage msg;
    msg.parse(data);
    return msg;
}
