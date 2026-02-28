#include "llfilepicker_portal.h"
#include "lltrans.h"
#include "llwindowsdl.h"

#include <cstdio>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <unistd.h>

bool LLPortalFileChooser::sDbusThreadsInitDone = false;

void LLPortalFileChooser::ensureDbusThreadingInitialized()
{
    if (!sDbusThreadsInitDone)
    {
        dbus_threads_init_default();
        sDbusThreadsInitDone = true;
    }
}

std::string LLPortalFileChooser::makeParentHandleX11(std::uint64_t x11WindowId)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "x11:0x%llx", static_cast<unsigned long long>(x11WindowId));
    return { buffer };
}

std::string LLPortalFileChooser::percentDecode(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == '%' && i + 2 < input.size())
        {
            char  hex[3] = { input[i + 1], input[i + 2], 0 };
            char* endPtr = nullptr;
            long  v      = std::strtol(hex, &endPtr, 16);
            if (endPtr && *endPtr == 0)
            {
                output.push_back(static_cast<char>(v));
                i += 2;
            }
            else
            {
                output.push_back(input[i]);
            }
        }
        else
        {
            output.push_back(input[i]);
        }
    }
    return output;
}

std::string LLPortalFileChooser::fileUriToPath(const char* uri)
{
    if (!uri)
        return {};
    const char* prefix = "file://";
    if (std::strncmp(uri, prefix, 7) == 0)
    {
        // RFC 8089: after file:// it can be host or empty. We assume local.
        return percentDecode(std::string(uri + 7));
    }
    return { uri };
}

DBusConnection* LLPortalFileChooser::getSessionBus(DBusError& error)
{
    dbus_error_init(&error);
    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (!connection || dbus_error_is_set(&error))
    {
        if (error.message)
            std::fprintf(stderr, "D-Bus connect error: %s\n", error.message);
        dbus_error_free(&error);
        return nullptr;
    }
    return connection;
}

bool LLPortalFileChooser::busHasOwner(DBusConnection* connection, const char* name)
{
    DBusError error;
    dbus_error_init(&error);
    dbus_bool_t owned = dbus_bus_name_has_owner(connection, name, &error);
    if (dbus_error_is_set(&error))
    {
        if (error.message)
            std::fprintf(stderr, "D-Bus owner check error: %s\n", error.message);
        dbus_error_free(&error);
    }
    return owned != 0;
}

static bool isNameActivatable(DBusConnection* connection, const char* name)
{
    DBusMessage* call = dbus_message_new_method_call(
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "ListActivatableNames"
    );
    if (!call)
    {
        return false;
    }

    DBusPendingCall* pending = nullptr;
    if (!dbus_connection_send_with_reply(connection, call, &pending, 3000) || !pending)
    {
        dbus_message_unref(call);
        return false;
    }
    dbus_message_unref(call);

    dbus_pending_call_block(pending);
    DBusMessage* reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);
    if (!reply)
    {
        return false;
    }

    bool activatable = false;

    DBusMessageIter it;
    if (dbus_message_iter_init(reply, &it) && dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_ARRAY)
    {
        DBusMessageIter arr;
        dbus_message_iter_recurse(&it, &arr);
        while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING)
        {
            const char* entry = nullptr;
            dbus_message_iter_get_basic(&arr, &entry);
            if (entry && std::strcmp(entry, name) == 0)
            {
                activatable = true;
                break;
            }
            dbus_message_iter_next(&arr);
        }
    }

    dbus_message_unref(reply);
    return activatable;
}

bool LLPortalFileChooser::hasUserExtension(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
    {
        return false;
    }
    if (slash != std::string::npos && dot < slash)
    {
        return false;
    }
    // Treat trailing dot as “no extension”
    if (dot == path.size() - 1)
    {
        return false;
    }
    return true;
}

void LLPortalFileChooser::appendBooleanOption(DBusMessageIter* dict, const char* key, dbus_bool_t value)
{
    DBusMessageIter entry;
    DBusMessageIter variant;

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

void LLPortalFileChooser::appendStringOption(DBusMessageIter* dict, const char* key, const char* value)
{
    DBusMessageIter entry;
    DBusMessageIter variant;

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

void LLPortalFileChooser::appendFiltersOption(DBusMessageIter* dict, const std::vector<LLPortalFileChooser::Filter>& filters)
{
    if (filters.empty())
    {
        return;
    }

    const char*     key = "filters";
    DBusMessageIter entry;
    DBusMessageIter variant;
    DBusMessageIter arrayOfFilters;

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "a(sa(us))", &variant);
    dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "(sa(us))", &arrayOfFilters);

    for (const LLPortalFileChooser::Filter& f : filters)
    {
        DBusMessageIter filterTuple;
        DBusMessageIter itemsArray;

        dbus_message_iter_open_container(&arrayOfFilters, DBUS_TYPE_STRUCT, nullptr, &filterTuple);

        const char* labelCStr = f._label.c_str();
        dbus_message_iter_append_basic(&filterTuple, DBUS_TYPE_STRING, &labelCStr);

        dbus_message_iter_open_container(&filterTuple, DBUS_TYPE_ARRAY, "(us)", &itemsArray);

        for (const LLPortalFileChooser::FilterItem& it : f._items)
        {
            DBusMessageIter itemTuple;
            dbus_message_iter_open_container(&itemsArray, DBUS_TYPE_STRUCT, nullptr, &itemTuple);

            std::uint32_t kind    = it._kind; // 0 = glob, 1 = mime
            const char*   pattern = it._pattern.c_str();
            dbus_message_iter_append_basic(&itemTuple, DBUS_TYPE_UINT32, &kind);
            dbus_message_iter_append_basic(&itemTuple, DBUS_TYPE_STRING, &pattern);

            dbus_message_iter_close_container(&itemsArray, &itemTuple);
        }

        dbus_message_iter_close_container(&filterTuple, &itemsArray);
        dbus_message_iter_close_container(&arrayOfFilters, &filterTuple);
    }

    dbus_message_iter_close_container(&variant, &arrayOfFilters);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

static bool sendWithReply(DBusConnection* connection,
                          DBusMessage* call,
                          DBusPendingCall** outPending,
                          const std::int32_t timeoutMs)
{
    if (!dbus_connection_send_with_reply(connection, call, outPending, timeoutMs))
    {
        return false;
    }
    return *outPending != nullptr;
}

bool LLPortalFileChooser::callPortalMethod(const char*                         method,
                                           const LLPortalFileChooser::Options& options,
                                           LLPortalFileChooser::Result&        outResult)
{
    ensureDbusThreadingInitialized();

    DBusError       error;
    DBusConnection* connection = getSessionBus(error);
    if (!connection)
    {
        return false;
    }

    DBusMessage* call = dbus_message_new_method_call("org.freedesktop.portal.Desktop",
                                                     "/org/freedesktop/portal/desktop",
                                                     "org.freedesktop.portal.FileChooser",
                                                     method);
    if (!call)
    {
        return false;
    }

    std::string parent     = makeParentHandleX11(options._x11WindowId);
    const char* parentCStr = parent.c_str();
    const char* titleCStr  = options._title.empty() ? "Select a file" : options._title.c_str();

    DBusMessageIter args;
    dbus_message_iter_init_append(call, &args);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentCStr);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &titleCStr);

    DBusMessageIter dict;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);

    appendBooleanOption(&dict, "modal", 1);

    if (std::strcmp(method, "OpenFile") == 0)
    {
        appendBooleanOption(&dict, "multiple", options._allowMultiple ? 1 : 0);
        appendBooleanOption(&dict, "directory", options._selectDirectory ? 1 : 0);
    }
    else
    {
        if (!options._currentName.empty())
        {
            appendStringOption(&dict, "current_name", options._currentName.c_str());
        }
    }

    if (!options._acceptLabel.empty())
    {
        appendStringOption(&dict, "accept_label", options._acceptLabel.c_str());
    }

    appendFiltersOption(&dict, options._filters);

    if (std::strcmp(method, "SaveFile") == 0 && options._currentFilter.has_value())
    {
        appendCurrentFilterOption(&dict, options._currentFilter.value());
    }

    {
        char tokenBuf[64];
        std::snprintf(tokenBuf, sizeof(tokenBuf), "llportal_%u", static_cast<unsigned>(::getpid()));
        appendStringOption(&dict, "handle_token", tokenBuf);
    }

    dbus_message_iter_close_container(&args, &dict);

    DBusPendingCall* pending = nullptr;
    if (!sendWithReply(connection, call, &pending, 10000))
    {
        dbus_message_unref(call);
        return false;
    }
    dbus_message_unref(call);
    dbus_connection_flush(connection);

    dbus_pending_call_block(pending);
    DBusMessage* reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);
    if (!reply)
    {
        return false;
    }

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
    {
        dbus_message_unref(reply);
        return false;
    }

    const char*     handlePath = nullptr;
    DBusMessageIter replyIter;
    dbus_message_iter_init(reply, &replyIter);
    if (DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&replyIter))
    {
        dbus_message_iter_get_basic(&replyIter, &handlePath);
    }
    dbus_message_unref(reply);
    if (!handlePath)
    {
        return false;
    }

    // Listen for Response on that handle
    char matchRule[512];
    std::snprintf(matchRule, sizeof(matchRule), "type='signal',interface='org.freedesktop.portal.Request',member='Response',path='%s'",
                  handlePath);
    DBusError matchError;
    dbus_error_init(&matchError);
    dbus_bus_add_match(connection, matchRule, &matchError);
    dbus_connection_flush(connection);

    int responseCode = 2;
    bool gotResponse = false;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(30000);

    while (std::chrono::steady_clock::now() < deadline)
    {
        dbus_connection_read_write(connection, 100);
        DBusMessage* sig = dbus_connection_pop_message(connection);
        if (!sig)
        {
            continue;
        }

        if (dbus_message_is_signal(sig, "org.freedesktop.portal.Request", "Response"))
        {
            DBusMessageIter sit;
            dbus_message_iter_init(sig, &sit);

            if (DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&sit))
            {
                std::uint32_t code = 2;
                dbus_message_iter_get_basic(&sit, &code);
                responseCode = static_cast<int>(code);
                dbus_message_iter_next(&sit);
            }

            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&sit))
            {
                DBusMessageIter dictIter;
                dbus_message_iter_recurse(&sit, &dictIter);

                while (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY)
                {
                    DBusMessageIter entryIter;
                    dbus_message_iter_recurse(&dictIter, &entryIter);

                    const char* key = nullptr;
                    dbus_message_iter_get_basic(&entryIter, &key);
                    dbus_message_iter_next(&entryIter);

                    if (key && dbus_message_iter_get_arg_type(&entryIter) == DBUS_TYPE_VARIANT)
                    {
                        if (std::strcmp(key, "uris") == 0)
                        {
                            DBusMessageIter varIter;
                            dbus_message_iter_recurse(&entryIter, &varIter);

                            if (dbus_message_iter_get_arg_type(&varIter) == DBUS_TYPE_ARRAY)
                            {
                                DBusMessageIter arrIter;
                                dbus_message_iter_recurse(&varIter, &arrIter);
                                while (dbus_message_iter_get_arg_type(&arrIter) == DBUS_TYPE_STRING)
                                {
                                    const char* uri = nullptr;
                                    dbus_message_iter_get_basic(&arrIter, &uri);
                                    std::string path = fileUriToPath(uri);
                                    if (!path.empty())
                                    {
                                        outResult._paths.push_back(path);
                                    }
                                    dbus_message_iter_next(&arrIter);
                                }
                            }
                        }
                        else if (std::strcmp(key, "uri") == 0)
                        {
                            DBusMessageIter varIter;
                            dbus_message_iter_recurse(&entryIter, &varIter);
                            if (dbus_message_iter_get_arg_type(&varIter) == DBUS_TYPE_STRING)
                            {
                                const char* uri = nullptr;
                                dbus_message_iter_get_basic(&varIter, &uri);
                                std::string path = fileUriToPath(uri);
                                if (!path.empty())
                                {
                                    outResult._paths.push_back(path);
                                }
                            }
                        }
                    }

                    dbus_message_iter_next(&dictIter);
                }
            }

            dbus_message_unref(sig);
            gotResponse = true;
            break;
        }

        dbus_message_unref(sig);
    }

    outResult._accepted = gotResponse && (responseCode == 0) && !outResult._paths.empty();
    return outResult._accepted;
}

LLPortalFileChooser::Result LLPortalFileChooser::doPortalCall([[maybe_unused]] const char*        method,
                                                              const LLPortalFileChooser::Options& options,
                                                              bool                                blocking)
{
    PortalState state;

    std::thread worker(
        [&state, method, options]()
        {
            LLPortalFileChooser::Result result;
            bool                        ok = callPortalMethod(method, options, result);

            {
                std::lock_guard<std::mutex> lock(state._mutex);
                state._result           = result;
                state._result._accepted = ok ? result._accepted : false;
                state._done             = true;
            }
            state._cv.notify_one();
        });

    if (blocking)
    {
        std::unique_lock<std::mutex> lock(state._mutex);
        state._cv.wait(lock, [&state]() { return state._done.load(); });
        if (worker.joinable())
        {
            worker.join();
        }
        return state._result;
    }
    else
    {
        worker.detach();
        // Caller can’t observe async completion via this API, so we return “no selection yet.”
        return LLPortalFileChooser::Result{};
    }
}

bool LLPortalFileChooser::IsPortalAvailable()
{
    ensureDbusThreadingInitialized();
    DBusError       error;
    DBusConnection* connection = getSessionBus(error);
    if (!connection)
    {
        return false;
    }

    if (isNameActivatable(connection, "org.freedesktop.portal.Desktop"))
    {
        return true;
    }
    return busHasOwner(connection, "org.freedesktop.portal.Desktop");
}

LLPortalFileChooser::Result LLPortalFileChooser::OpenFile(const Options& options, bool blocking)
{
    return doPortalCall("OpenFile", options, blocking);
}

LLPortalFileChooser::Result LLPortalFileChooser::SaveFile(const Options& options, bool blocking)
{
    return doPortalCall("SaveFile", options, blocking);
}

LLPortalFileChooser::Result LLPortalFileChooser::OpenForLoad(ELoadFilter filter,
                                                             bool               allowMultiple,
                                                             std::uint64_t      x11WindowId,
                                                             const std::string& title,
                                                             bool               blocking)
{
    LLPortalFileChooser::Options options;
    options._title           = title;
    options._x11WindowId     = x11WindowId;
    options._allowMultiple   = allowMultiple;
    options._selectDirectory = false;
    options._filters         = buildFiltersForLoad(filter);
    return OpenFile(options, blocking);
}

LLPortalFileChooser::Result LLPortalFileChooser::OpenForSave(ESaveFilter filter,
                                                             std::uint64_t      x11WindowId,
                                                             const std::string& title,
                                                             const std::string& suggestedName,
                                                             bool               blocking)
{
    LLPortalFileChooser::Options options;
    options._title           = title;
    options._x11WindowId     = x11WindowId;
    options._allowMultiple   = false;
    options._selectDirectory = false;
    options._currentName     = suggestedName;

    std::optional<LLPortalFileChooser::Filter> current;
    options._filters = buildFiltersForSave(filter, current);
    options._currentFilter = current;

    return SaveFile(options, blocking);
}
std::vector<LLPortalFileChooser::Filter> LLPortalFileChooser::buildFiltersForLoad(ELoadFilter loadFilter)
{
    std::vector<LLPortalFileChooser::Filter> filters;

    auto makeFilter = [](const std::string&                                            label,
                         const std::initializer_list<LLPortalFileChooser::FilterItem>& items) -> LLPortalFileChooser::Filter
    {
        LLPortalFileChooser::Filter f;
        f._label = label;
        f._items = items;
        return f;
    };

    switch (loadFilter)
    {
        case FFLOAD_WAV:
        {
            // 0 = glob, 1 = mime (portal expects (us) pairs)
            filters.push_back(makeFilter("Sounds (*.wav)", { { 0u, "*.wav" }, { 1u, "audio/x-wav" }, { 1u, "audio/wav" } }));
            break;
        }
        case FFLOAD_IMAGE:
        {
            filters.push_back(makeFilter("Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)",
                                         { { 0u, "*.tga" },
                                           { 0u, "*.bmp" },
                                           { 0u, "*.jpg" },
                                           { 0u, "*.jpeg" },
                                           { 0u, "*.png" },
                                           { 1u, "image/bmp" },
                                           { 1u, "image/jpeg" },
                                           { 1u, "image/png" },
                                           { 1u, "image/x-tga" } }));
            break;
        }
        case FFLOAD_ANIM:
        {
            filters.push_back(makeFilter("Animations (*.bvh; *.anim)", { { 0u, "*.bvh" }, { 0u, "*.anim" } }));
            break;
        }
        case FFLOAD_GLTF:
        {
            filters.push_back(makeFilter("glTF (*.gltf; *.glb)",
                                         { { 0u, "*.gltf" }, { 0u, "*.glb" }, { 1u, "model/gltf+json" }, { 1u, "model/gltf-binary" } }));
            break;
        }
        case FFLOAD_COLLADA:
        {
            filters.push_back(makeFilter("Scene (*.dae)", { { 0u, "*.dae" } }));
            break;
        }
        case FFLOAD_SCRIPT:
        {
            filters.push_back(makeFilter("Script files (*.lsl)", { { 0u, "*.lsl" }, { 1u, "text/plain" } }));
            break;
        }
        case FFLOAD_DICTIONARY:
        {
            filters.push_back(makeFilter("Dictionary files (*.dic; *.xcu)", { { 0u, "*.dic" }, { 0u, "*.xcu" }, { 1u, "text/plain" } }));
            break;
        }
        case FFLOAD_RAW:
        {
            filters.push_back(makeFilter("RAW files (*.raw)", { { 0u, "*.raw" } }));
            break;
        }
        case FFLOAD_MATERIAL:
        case FFLOAD_MATERIAL_TEXTURE:
        {
            filters.push_back(makeFilter("GLTF (*.gltf; *.glb)", { { 0u, "*.gltf" }, { 0u, "*.glb" } }));
            if (loadFilter == FFLOAD_MATERIAL_TEXTURE)
            {
                filters.back()._items.push_back({ 0u, "*.tga" });
                filters.back()._items.push_back({ 0u, "*.bmp" });
                filters.back()._items.push_back({ 0u, "*.jpg" });
                filters.back()._items.push_back({ 0u, "*.jpeg" });
                filters.back()._items.push_back({ 0u, "*.png" });
            }
            break;
        }
        case FFLOAD_HDRI:
        {
            filters.push_back(makeFilter("HDRI Files (*.exr)", { { 0u, "*.exr" }, { 1u, "image/exr" }, { 1u, "image/x-exr" } }));
            break;
        }
        case FFLOAD_ALL:
        case FFLOAD_EXE:
        case FFLOAD_SLOBJECT:
        case FFLOAD_MODEL:
        case FFLOAD_DIRECTORY:
        case FFLOAD_IMPORT:
        default:
        {
            // Fall back to “All files”
            LLPortalFileChooser::Filter f;
            f._label = "All Files (*.*)";
            f._items.push_back({ 0u, "*" });
            filters.push_back(f);
            break;
        }
    }

    return filters;
}

void LLPortalFileChooser::appendCurrentFilterOption(DBusMessageIter* dict, const LLPortalFileChooser::Filter& filter)
{
    const char* key = "current_filter";

    DBusMessageIter entry;
    DBusMessageIter variant;
    DBusMessageIter filterTuple;
    DBusMessageIter itemsArray;

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "(sa(us))", &variant);

    dbus_message_iter_open_container(&variant, DBUS_TYPE_STRUCT, nullptr, &filterTuple);
    const char* labelCStr = filter._label.c_str();
    dbus_message_iter_append_basic(&filterTuple, DBUS_TYPE_STRING, &labelCStr);

    dbus_message_iter_open_container(&filterTuple, DBUS_TYPE_ARRAY, "(us)", &itemsArray);
    for (const LLPortalFileChooser::FilterItem& item : filter._items)
    {
        DBusMessageIter itemTuple;
        dbus_message_iter_open_container(&itemsArray, DBUS_TYPE_STRUCT, nullptr, &itemTuple);

        std::uint32_t kind = item._kind;
        const char* pattern = item._pattern.c_str();
        dbus_message_iter_append_basic(&itemTuple, DBUS_TYPE_UINT32, &kind);
        dbus_message_iter_append_basic(&itemTuple, DBUS_TYPE_STRING, &pattern);

        dbus_message_iter_close_container(&itemsArray, &itemTuple);
    }
    dbus_message_iter_close_container(&filterTuple, &itemsArray);

    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}
std::vector<LLPortalFileChooser::Filter> LLPortalFileChooser::buildFiltersForSave(ESaveFilter                   saveFilter,
                                                                                  std::optional<LLPortalFileChooser::Filter>& currentOut)
{
    std::vector<LLPortalFileChooser::Filter> filters;

    auto makeFilter = [](const std::string& label,
                         const std::initializer_list<LLPortalFileChooser::FilterItem>& items) -> LLPortalFileChooser::Filter
    {
        LLPortalFileChooser::Filter f;
        f._label = label;
        f._items = items;
        return f;
    };

    LLPortalFileChooser::Filter chosen;

    switch (saveFilter)
    {
        case FFSAVE_PNG:
        {
            chosen = makeFilter("PNG Images (*.png)",
                                {
                                    { 0u, "*.png" },
                                    { 1u, "image/png" }
                                });
            break;
        }
        case FFSAVE_TGA:
        {
            chosen = makeFilter("Targa Images (*.tga)",
                                {
                                    { 0u, "*.tga" },
                                    { 1u, "image/x-tga" }
                                });
            break;
        }
        case FFSAVE_BMP:
        {
            chosen = makeFilter("Bitmap Images (*.bmp)",
                                {
                                    { 0u, "*.bmp" },
                                    { 1u, "image/bmp" }
                                });
            break;
        }
        case FFSAVE_JPEG:
        {
            chosen = makeFilter("JPEG Images (*.jpg; *.jpeg)",
                                {
                                    { 0u, "*.jpg" },
                                    { 0u, "*.jpeg" },
                                    { 1u, "image/jpeg" }
                                });
            break;
        }
        case FFSAVE_WAV:
        {
            chosen = makeFilter("WAV Sounds (*.wav)",
                                {
                                    { 0u, "*.wav" },
                                    { 1u, "audio/wav" },
                                    { 1u, "audio/x-wav" }
                                });
            break;
        }
        case FFSAVE_GLTF:
        {
            chosen = makeFilter("glTF Asset File (*.gltf)",
                                {
                                    { 0u, "*.gltf" },
                                    { 1u, "model/gltf+json" }
                                });
            break;
        }
        case FFSAVE_COLLADA:
        {
            chosen = makeFilter("COLLADA File (*.dae)",
                                {
                                    { 0u, "*.dae" }
                                });
            break;
        }
        case FFSAVE_XML:
        case FFSAVE_BEAM:
        {
            chosen = makeFilter("XML File (*.xml)",
                                {
                                    { 0u, "*.xml" },
                                    { 1u, "application/xml" },
                                    { 1u, "text/xml" }
                                });
            break;
        }
        case FFSAVE_RAW:
        {
            chosen = makeFilter("RAW files (*.raw)",
                                {
                                    { 0u, "*.raw" }
                                });
            break;
        }
        case FFSAVE_J2C:
        {
            chosen = makeFilter("Compressed Images (*.j2c)",
                                {
                                    { 0u, "*.j2c" }
                                });
            break;
        }
        case FFSAVE_SCRIPT:
        {
            chosen = makeFilter("LSL Files (*.lsl)",
                                {
                                    { 0u, "*.lsl" },
                                    { 1u, "text/plain" }
                                });
            break;
        }
        case FFSAVE_AVI:
        {
            chosen = makeFilter("AVI Movie File (*.avi)",
                                {
                                    { 0u, "*.avi" },
                                    { 1u, "video/x-msvideo" }
                                });
            break;
        }
        case FFSAVE_TGAPNG:
        {
            LLPortalFileChooser::Filter png = makeFilter("PNG Images (*.png)", { { 0u, "*.png" }, { 1u, "image/png" } });
            LLPortalFileChooser::Filter tga = makeFilter("Targa Images (*.tga)", { { 0u, "*.tga" }, { 1u, "image/x-tga" } });
            filters.push_back(png);
            filters.push_back(tga);
            currentOut = png;
            filters.push_back(makeFilter("All Files (*.*)", { { 0u, "*" } }));
            return filters;
        }
        case FFSAVE_EXPORT:
        {
            chosen = makeFilter("OXP Backup Files (*.oxp)",
                                {
                                    { 0u, "*.oxp" }
                                });
            break;
        }
        case FFSAVE_CSV:
        {
            chosen = makeFilter("CSV Files (*.csv)",
                                {
                                    { 0u, "*.csv" },
                                    { 1u, "text/csv" }
                                });
            break;
        }
        case FFSAVE_ALL:
        default:
        {
            chosen = makeFilter("All Files (*.*)", { { 0u, "*" } });
            break;
        }
    }

    filters.push_back(chosen);
    filters.push_back(makeFilter("All Files (*.*)", { { 0u, "*" } }));

    currentOut = chosen;
    return filters;
}
std::string LLPortalFileChooser::appendDefaultExtensionIfMissing(const std::string& path, const std::string& defaultExtWithDot)
{
    if (hasUserExtension(path))
    {
        return path;
    }
    return path + defaultExtWithDot;
}

std::string LLPortalFileChooser::checkDefaultExtension(std::string finalPath, int32_t filter)
{
    switch (static_cast<ESaveFilter>(filter))
    {
        case FFSAVE_PNG:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".png");
            break;
        }
        case FFSAVE_TGA:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".tga");
            break;
        }
        case FFSAVE_BMP:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".bmp");
            break;
        }
        case FFSAVE_JPEG:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".jpeg");
            break;
        }
        case FFSAVE_WAV:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".wav");
            break;
        }
        case FFSAVE_GLTF:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".gltf");
            break;
        }
        case FFSAVE_COLLADA:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".dae");
            break;
        }
        case FFSAVE_XML:
        case FFSAVE_BEAM:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".xml");
            break;
        }
        case FFSAVE_RAW:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".raw");
            break;
        }
        case FFSAVE_J2C:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".j2c");
            break;
        }
        case FFSAVE_SCRIPT:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".lsl");
            break;
        }
        case FFSAVE_AVI:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".avi");
            break;
        }
        case FFSAVE_TGAPNG:
        {
            finalPath = appendDefaultExtensionIfMissing(finalPath, ".png");
            break;
        }
        default:
            break;
    }
    return finalPath;
}