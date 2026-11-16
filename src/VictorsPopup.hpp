#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

// Shows the victors (completions) and progress records of one level, fetched live from the list website.
class VictorsPopup : public Popup {
protected:
    std::string m_url;
    async::TaskHolder<web::WebResponse> m_listener;
    ScrollLayer* m_scroll = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_updated = nullptr;
    std::vector<std::string> m_videos;

    bool initPopup(std::string const& url, std::string const& levelName);

    void fetch();
    void onResponse(web::WebResponse const& res);
    void onRefresh(CCObject*);
    void onAutoRefresh(float);
    void onVideo(CCObject* sender);
    void setStatus(std::string const& text);

public:
    static VictorsPopup* create(std::string const& url, std::string const& levelName);
};
