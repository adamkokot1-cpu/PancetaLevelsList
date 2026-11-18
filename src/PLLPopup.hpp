#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

struct ListLevel {
    int siteId = 0;         // id on the list website (used for /levels/{id})
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
    std::string difficulty; // "easy" ... "extreme-demon"
    std::string rating;     // "rate", "feature", "epic", "legendary", "mythic"
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
    std::string signature;  // used to detect changes between refreshes
    std::string fetchedAt;
};

enum class ListTab { PLL = 0, GPLL = 1 };

class PLLPopup : public Popup {
protected:
    ListTab m_tab = ListTab::PLL;
    async::TaskHolder<web::WebResponse> m_listener;

    ScrollLayer* m_scroll = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_updated = nullptr;
    ButtonSprite* m_pllTabSpr = nullptr;
    ButtonSprite* m_gpllTabSpr = nullptr;
    std::vector<std::string> m_videos;
    std::vector<ListLevel> m_rowLevels;
    std::string m_shownSignature;
    int m_retry = 0;

    bool initPopup();

    void onTab(CCObject* sender);
    void onRefresh(CCObject*);
    void onAutoRefresh(float);
    void onRetry(float);
    void onView(CCObject* sender);
    void onVideo(CCObject* sender);
    void onVictors(CCObject* sender);

    void showTab();
    void fetch();
    void onResponse(int tab, web::WebResponse const& res);
    void populate(ListData const& data);
    void setStatus(std::string const& text);
    void setUpdated(std::string const& text);
    void clearList();

    CCNode* createHeader(std::string const& text, float width);
    CCNode* createRow(ListLevel const& level, float width, bool alt);

public:
    static PLLPopup* create();
    // Download the PLL list in the background (called from the main menu)
    static void prefetch();
};
