#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/general.hpp>
#include <ctime>

namespace pll {
    inline std::string jstr(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return "";
        return v[key].asString().unwrapOr("");
    }

    inline int jint(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return 0;
        return static_cast<int>(v[key].asInt().unwrapOr(0));
    }

    inline double jnum(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return 0.0;
        return v[key].asDouble().unwrapOr(0.0);
    }

    inline bool jbool(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return false;
        return v[key].asBool().unwrapOr(false);
    }

    // Base API URL for a tab, e.g. "https://pll-plus.levelslist.workers.dev/api" (no trailing slash)
    inline std::string apiBase(bool gpll) {
        auto url = geode::Mod::get()->getSettingValue<std::string>(gpll ? "gpll-api-base" : "pll-api-base");
        while (!url.empty() && url.back() == '/') url.pop_back();
        return url;
    }

    inline std::string nowString() {
        std::tm t = geode::localtime(std::time(nullptr));
        return fmt::format("{:02}:{:02}:{:02}", t.tm_hour, t.tm_min, t.tm_sec);
    }
}
