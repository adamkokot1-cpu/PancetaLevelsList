#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

struct ListLevel {
    int position = 0;       // main/extended position, 0 if UPLL-only
    int upllPosition = 0;
    std::string name;
    std::string publisher;
    std::string verifier;
    std::string creators;
    std::string list;       // "main", "extended", "upll"
    std::string category;   // "hardest", "high", "mid", "low", "extended", "upll"
    std::string video;
    int gdId = 0;
    double points = 0;
    double upllPoints = 0;
    int requirement = 0;
    int completions = 0;
};

struct ListCategory {
    std::string key;
    std::string name;
    int from = 0;
    int to = 0;
};

struct ListData {
    std::vector<ListLevel> levels;
    std::vector<ListCategory> categories;
};

enum class ListTab { PLL = 0, GPLL = 1 };

class PLLPopup : public Popup {
protected:
    ListTab m_tab = ListTab::PLL;
    async::TaskHolder<web::WebResponse> m_listener;

    ScrollLayer* m_scroll = nullptr;
    CCLabelBMFont* m_status = nullptr;
    ButtonSprite* m_pllTabSpr = nullptr;
    ButtonSprite* m_gpllTabSpr = nullptr;
    std::vector<std::string> m_videos;

    bool initPopup();

    void onTab(CCObject* sender);
    void onRefresh(CCObject*);
    void onView(CCObject* sender);
    void onVideo(CCObject* sender);

    void load(bool force);
    void onResponse(int tab, web::WebResponse const& res);
    void populate(ListData const& data);
    void setStatus(std::string const& text);
    void clearList();

    CCNode* createHeader(std::string const& text, float width);
    CCNode* createRow(ListLevel const& level, float width, bool alt);

public:
    static PLLPopup* create();
};
