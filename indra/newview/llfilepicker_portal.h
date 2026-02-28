#pragma once

#include "llfilepicker_etype.h"

#include <dbus/dbus.h>

#include <cstdint>
#include <string>
#include <vector>

class LLPortalFileChooser
{
public:
    struct FilterItem
    {
        std::uint32_t _kind;
        std::string _pattern;
    };

    struct Filter
    {
        std::string _label;
        std::vector<FilterItem> _items;
    };

    struct Options
    {
        std::string _title;
        bool _allowMultiple{false};
        bool _selectDirectory{false};
        std::uint64_t _x11WindowId{0};
        std::string _acceptLabel;
        std::string _currentName;
        std::vector<Filter> _filters;
        std::optional<LLPortalFileChooser::Filter> _currentFilter;
    };

    struct Result
    {
        bool _accepted{false};
        std::vector<std::string> _paths;
    };

    struct PortalState
    {
        std::mutex _mutex;
        std::condition_variable _cv;
        std::atomic<bool> _done{false};
        LLPortalFileChooser::Result _result;
    };


    static bool IsPortalAvailable();
    static Result OpenFile(const Options& options, bool blocking);
    static Result SaveFile(const Options& options, bool blocking);

    static Result OpenForLoad(ELoadFilter filter,
                           bool allowMultiple,
                           std::uint64_t x11WindowId,
                           const std::string &title,
                           bool blocking);

    static Result OpenForSave(ESaveFilter filter,
                           std::uint64_t x11WindowId,
                           const std::string &title,
                           const std::string &suggestedName,
                           bool blocking);

    static std::string checkDefaultExtension(std::string finalPath, int32_t filter);

private:
    static bool sDbusThreadsInitDone;

    static void appendFiltersOption(DBusMessageIter* dict, const std::vector<LLPortalFileChooser::Filter>& filters);
    static void appendStringOption(DBusMessageIter* dict, const char* key, const char* value);
    static void appendBooleanOption(DBusMessageIter* dict, const char* key, dbus_bool_t value);
    static bool busHasOwner(DBusConnection* connection, const char* name);
    static DBusConnection* getSessionBus(DBusError& error);
    static std::string fileUriToPath(const char* uri);
    static std::string percentDecode(const std::string& input);
    static std::string makeParentHandleX11(std::uint64_t x11WindowId);
    static void ensureDbusThreadingInitialized();
    static std::vector<LLPortalFileChooser::Filter> buildFiltersForLoad(ELoadFilter loadFilter);
    static std::vector<LLPortalFileChooser::Filter> buildFiltersForSave(ESaveFilter saveFilter, std::optional<LLPortalFileChooser::Filter>& currentOut);
    static bool callPortalMethod(const char* method,
                                                        const LLPortalFileChooser::Options& options,
                                                        LLPortalFileChooser::Result& outResult);
    static bool hasUserExtension(const std::string& path);
    static LLPortalFileChooser::Result doPortalCall(const char* method,
                                                    const LLPortalFileChooser::Options& options,
                                                    bool blocking);
    static std::string appendDefaultExtensionIfMissing(const std::string& path, const std::string& defaultExtWithDot);
    static void appendCurrentFilterOption(DBusMessageIter* dict, const LLPortalFileChooser::Filter& f);
};