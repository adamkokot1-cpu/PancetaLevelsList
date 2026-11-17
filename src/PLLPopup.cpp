#include "PLLPopup.hpp"
#include "VictorsPopup.hpp"
#include "Json.hpp"

#include <array>
#include <optional>
#include <algorithm>

using namespace pll;

namespace {
    constexpr float POPUP_W = 440.f;
    constexpr float POPUP_H = 290.f;
    constexpr float SCROLL_W = 400.f;
    constexpr float SCROLL_H = 186.f;
    constexpr float ROW_H = 42.f;
    constexpr float HEADER_H = 26.f;
    constexpr float GAP = 3.f;

    // Last fetched list per tab. Shown instantly while fresh data is downloading.
    std::array<std::optional<ListData>, 2> s_cache;

    ListData parseList(matjson::Value const& json) {
        ListData data;

        if (json.contains("categories") && json["categories"].isArray()) {
            for (auto const& c : json["categories"]) {
                data.categories.push_back({ jstr(c, "key"), jstr(c, "name"), jint(c, "from"), jint(c, "to") });
            }
        }

        if (json.contains("levels") && json["levels"].isArray()) {
            for (auto const& l : json["levels"]) {
                ListLevel lvl;
                lvl.siteId = jint(l, "id");
                lvl.position = jint(l, "position");
                lvl.upllPosition = jint(l, "upll_position");
                lvl.name = jstr(l, "name");
                lvl.publisher = jstr(l, "publisher");
                lvl.verifier = jstr(l, "verifier");
                lvl.creators = jstr(l, "creators");
                lvl.list = jstr(l, "list");
                lvl.category = jstr(l, "category");
                lvl.video = jstr(l, "video_url");
                lvl.gdId = jint(l, "gd_level_id");
                lvl.points = jnum(l, "points");
                lvl.upllPoints = jnum(l, "upll_points");
                lvl.requirement = jint(l, "requirement");
                lvl.completions = jint(l, "completions");
                if (lvl.name.empty()) continue;

                data.signature += fmt::format("{}|{}|{}|{}|{:.2f};", lvl.siteId, lvl.name, lvl.position, lvl.completions, lvl.points);
                data.levels.push_back(std::move(lvl));
            }
        }

        data.fetchedAt = nowString();
        return data;
    }

    ListCategory const* findCategory(ListData const& data, std::string const& key) {
        for (auto const& c : data.categories) {
            if (c.key == key) return &c;
        }
        return nullptr;
    }

    ccColor3B rankColor(int pos) {
        switch (pos) {
            case 1: return { 255, 215, 0 };
            case 2: return { 210, 215, 230 };
            case 3: return { 215, 140, 70 };
            default: return { 255, 255, 255 };
        }
    }

    bool byPosition(ListLevel const* a, ListLevel const* b) { return a->position < b->position; }
    bool byUpllPosition(ListLevel const* a, ListLevel const* b) { return a->upllPosition < b->upllPosition; }

    constexpr int MAX_RETRIES = 4;
    bool s_prefetching = false;
}

void PLLPopup::prefetch() {
    if (s_cache[0] || s_prefetching) return;
    auto base = apiBase(false);
    if (base.empty()) return;

    s_prefetching = true;
    auto req = web::WebRequest();
    req.userAgent("PancetaLevelsList Geode mod");
    async::spawn(
        req.get(base + "/levels"),
        [](web::WebResponse res) {
            s_prefetching = false;
            if (!res.ok()) return;
            auto parsed = res.json();
            if (parsed.isErr()) return;
            if (!s_cache[0]) s_cache[0] = parseList(parsed.unwrap());
        }
    );
}

PLLPopup* PLLPopup::create() {
    auto ret = new PLLPopup();
    if (ret->initPopup()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PLLPopup::initPopup() {
    if (!Popup::init(POPUP_W, POPUP_H)) return false;

    this->setTitle("Panceta Levels List");

    auto size = m_mainLayer->getContentSize();

    // --- Tabs + refresh ---
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    menu->setContentSize(size);
    menu->setID("tabs-menu"_spr);
    m_mainLayer->addChild(menu);

    m_pllTabSpr = ButtonSprite::create("PLL", "bigFont.fnt", "GJ_button_01.png", .7f);
    m_pllTabSpr->setScale(.75f);
    auto pllBtn = CCMenuItemSpriteExtra::create(m_pllTabSpr, this, menu_selector(PLLPopup::onTab));
    pllBtn->setTag(static_cast<int>(ListTab::PLL));
    pllBtn->setID("pll-tab"_spr);
    pllBtn->setPosition({ size.width / 2 - 45.f, size.height - 52.f });
    menu->addChild(pllBtn);

    m_gpllTabSpr = ButtonSprite::create("GPLL", "bigFont.fnt", "GJ_button_04.png", .7f);
    m_gpllTabSpr->setScale(.75f);
    auto gpllBtn = CCMenuItemSpriteExtra::create(m_gpllTabSpr, this, menu_selector(PLLPopup::onTab));
    gpllBtn->setTag(static_cast<int>(ListTab::GPLL));
    gpllBtn->setID("gpll-tab"_spr);
    gpllBtn->setPosition({ size.width / 2 + 45.f, size.height - 52.f });
    menu->addChild(gpllBtn);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(.6f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(PLLPopup::onRefresh));
    refreshBtn->setID("refresh-button"_spr);
    refreshBtn->setPosition({ size.width - 30.f, size.height - 52.f });
    menu->addChild(refreshBtn);

    // --- List area ---
    float scrollX = (size.width - SCROLL_W) / 2.f;
    float scrollY = 24.f;

    auto bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(80);
    bg->setContentSize({ SCROLL_W + 6.f, SCROLL_H + 6.f });
    bg->setPosition({ size.width / 2.f, scrollY + SCROLL_H / 2.f });
    m_mainLayer->addChild(bg);

    m_scroll = ScrollLayer::create({ SCROLL_W, SCROLL_H });
    m_scroll->setPosition({ scrollX, scrollY });
    m_scroll->setID("level-list"_spr);
    m_mainLayer->addChild(m_scroll);

    auto scrollbar = Scrollbar::create(m_scroll);
    scrollbar->setPosition({ scrollX + SCROLL_W + 10.f, scrollY + SCROLL_H / 2.f });
    m_mainLayer->addChild(scrollbar);

    m_status = CCLabelBMFont::create("", "bigFont.fnt");
    m_status->setScale(.5f);
    m_status->setAlignment(kCCTextAlignmentCenter);
    m_status->setPosition({ size.width / 2.f, scrollY + SCROLL_H / 2.f });
    m_mainLayer->addChild(m_status, 10);

    m_updated = CCLabelBMFont::create("", "chatFont.fnt");
    m_updated->setScale(.45f);
    m_updated->setColor({ 200, 200, 200 });
    m_updated->setPosition({ size.width / 2.f, 12.f });
    m_mainLayer->addChild(m_updated);

    this->showTab();

    // Live updates while the popup is open
    auto interval = Mod::get()->getSettingValue<int64_t>("refresh-interval");
    if (interval > 0) {
        this->schedule(schedule_selector(PLLPopup::onAutoRefresh), static_cast<float>(interval));
    }
    return true;
}

void PLLPopup::onTab(CCObject* sender) {
    auto tab = static_cast<ListTab>(static_cast<CCNode*>(sender)->getTag());
    if (tab == m_tab) return;
    m_tab = tab;

    m_pllTabSpr->updateBGImage(tab == ListTab::PLL ? "GJ_button_01.png" : "GJ_button_04.png");
    m_gpllTabSpr->updateBGImage(tab == ListTab::GPLL ? "GJ_button_01.png" : "GJ_button_04.png");

    this->showTab();
}

void PLLPopup::onRefresh(CCObject*) {
    m_retry = 0;
    this->fetch();
}

void PLLPopup::onRetry(float) {
    this->fetch();
}

void PLLPopup::onAutoRefresh(float) {
    this->fetch();
}

void PLLPopup::onView(CCObject* sender) {
    int id = static_cast<CCNode*>(sender)->getTag();
    if (id <= 0) return;
    auto search = GJSearchObject::create(SearchType::Search, std::to_string(id));
    CCDirector::get()->pushScene(CCTransitionFade::create(.5f, LevelBrowserLayer::scene(search)));
}

void PLLPopup::onVideo(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_videos.size())) return;
    web::openLinkInBrowser(m_videos[idx]);
}

void PLLPopup::onVictors(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_rowLevels.size())) return;
    auto const& lvl = m_rowLevels[idx];
    if (lvl.siteId <= 0) return;

    auto url = fmt::format("{}/levels/{}", apiBase(m_tab == ListTab::GPLL), lvl.siteId);
    if (auto popup = VictorsPopup::create(url, lvl.name)) popup->show();
}

void PLLPopup::setStatus(std::string const& text) {
    m_status->setString(text.c_str());
    m_status->setVisible(!text.empty());
}

void PLLPopup::setUpdated(std::string const& text) {
    m_updated->setString(text.c_str());
}

void PLLPopup::clearList() {
    m_scroll->m_contentLayer->removeAllChildren();
    m_scroll->m_contentLayer->setContentSize({ SCROLL_W, SCROLL_H });
    m_videos.clear();
    m_rowLevels.clear();
    m_shownSignature.clear();
}

// Show cached data for the current tab right away, then fetch fresh data.
void PLLPopup::showTab() {
    m_listener.cancel();
    m_retry = 0;
    this->clearList();

    auto idx = static_cast<size_t>(m_tab);
    if (s_cache[idx]) {
        this->populate(*s_cache[idx]);
    } else {
        this->setStatus("Loading...");
    }
    this->fetch();
}

void PLLPopup::fetch() {
    auto base = apiBase(m_tab == ListTab::GPLL);

    if (base.empty()) {
        this->clearList();
        this->setUpdated("");
        this->setStatus(m_tab == ListTab::GPLL
            ? "GPLL is coming soon!\nNo levels yet."
            : "No PLL API URL set\n(check mod settings).");
        return;
    }

    this->setUpdated("Updating...");

    auto req = web::WebRequest();
    req.userAgent("PancetaLevelsList Geode mod");

    auto tab = static_cast<int>(m_tab);
    m_listener.spawn(
        req.get(base + "/levels"),
        [this, tab](web::WebResponse res) {
            this->onResponse(tab, res);
        }
    );
}

void PLLPopup::onResponse(int tab, web::WebResponse const& res) {
    if (static_cast<int>(m_tab) != tab) return;
    auto idx = static_cast<size_t>(tab);
    bool hasData = s_cache[idx].has_value();

    if (!res.ok()) {
        // code() < 0 is a connection error (e.g. -28 = timeout). Geode only waits 2.5s
        // to connect, so slow first connections are common: retry a few times.
        if (res.code() <= 0 && m_retry < MAX_RETRIES) {
            m_retry++;
            this->setUpdated(fmt::format("Slow connection, retrying ({}/{})...", m_retry, MAX_RETRIES));
            if (!hasData) this->setStatus("Connecting...");
            this->scheduleOnce(schedule_selector(PLLPopup::onRetry), 1.5f * m_retry);
            return;
        }

        std::string err = res.code() <= 0
            ? fmt::format("Connection error: {}", res.errorMessage())
            : fmt::format("Error HTTP {}", res.code());
        if (hasData) {
            this->setUpdated(fmt::format("{} - showing data from {}", err, s_cache[idx]->fetchedAt));
        } else {
            this->setUpdated("");
            this->setStatus(err + "\nPress refresh to try again.");
        }
        return;
    }
    m_retry = 0;

    auto parsed = res.json();
    if (parsed.isErr()) {
        if (!hasData) this->setStatus("Could not read list data.");
        this->setUpdated("Could not read list data");
        return;
    }

    s_cache[idx] = parseList(parsed.unwrap());
    this->populate(*s_cache[idx]);
}

void PLLPopup::populate(ListData const& data) {
    auto interval = Mod::get()->getSettingValue<int64_t>("refresh-interval");
    this->setUpdated(interval > 0
        ? fmt::format("Live - updated {} (every {}s)", data.fetchedAt, interval)
        : fmt::format("Updated {}", data.fetchedAt));

    // Nothing changed on the website: keep the list (and scroll position) as is.
    if (!m_shownSignature.empty() && data.signature == m_shownSignature) return;

    auto content = m_scroll->m_contentLayer;
    bool first = m_shownSignature.empty();
    float oldH = content->getContentSize().height;
    float offset = content->getPositionY() - (SCROLL_H - oldH);

    content->removeAllChildren();
    m_videos.clear();
    m_rowLevels.clear();
    m_shownSignature = data.signature;

    std::vector<ListLevel const*> mainLevels;
    std::vector<ListLevel const*> upllLevels;
    for (auto const& lvl : data.levels) {
        if (lvl.position > 0) mainLevels.push_back(&lvl);
        else if (lvl.list == "upll") upllLevels.push_back(&lvl);
    }
    std::sort(mainLevels.begin(), mainLevels.end(), byPosition);
    std::sort(upllLevels.begin(), upllLevels.end(), byUpllPosition);

    bool showUpll = Mod::get()->getSettingValue<bool>("show-upll");

    std::vector<CCNode*> nodes;
    std::string lastCategory = "\x01";
    int rowIdx = 0;

    for (auto lvl : mainLevels) {
        if (lvl->category != lastCategory) {
            lastCategory = lvl->category;
            std::string title;
            if (auto cat = findCategory(data, lvl->category)) {
                title = fmt::format("{}  (#{}-{})", cat->name, cat->from, cat->to);
            } else {
                title = lvl->category.empty() ? "Other" : lvl->category;
            }
            nodes.push_back(this->createHeader(title, SCROLL_W));
        }
        nodes.push_back(this->createRow(*lvl, SCROLL_W, rowIdx++ % 2 == 1));
    }

    if (showUpll && !upllLevels.empty()) {
        std::string title = "UPLL only";
        if (auto cat = findCategory(data, "upll")) title = cat->name;
        nodes.push_back(this->createHeader(title, SCROLL_W));
        for (auto lvl : upllLevels) {
            nodes.push_back(this->createRow(*lvl, SCROLL_W, rowIdx++ % 2 == 1));
        }
    }

    if (nodes.empty()) {
        content->setContentSize({ SCROLL_W, SCROLL_H });
        this->setStatus("No levels on this list yet.");
        return;
    }
    this->setStatus("");

    float total = GAP;
    for (auto n : nodes) total += n->getContentSize().height + GAP;
    float height = std::max(total, SCROLL_H);
    content->setContentSize({ SCROLL_W, height });

    float y = height - GAP;
    for (auto n : nodes) {
        auto h = n->getContentSize().height;
        n->setPosition({ SCROLL_W / 2.f, y - h / 2.f });
        content->addChild(n);
        y -= h + GAP;
    }

    if (first) {
        m_scroll->moveToTop();
    } else {
        float minY = SCROLL_H - height;
        content->setPositionY(std::clamp(minY + offset, minY, 0.f));
    }
}

CCNode* PLLPopup::createHeader(std::string const& text, float width) {
    auto node = CCNode::create();
    node->setContentSize({ width, HEADER_H });
    node->setAnchorPoint({ .5f, .5f });

    auto label = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");
    label->setScale(.6f);
    label->limitLabelWidth(width - 20.f, .6f, .2f);
    label->setPosition({ width / 2.f, HEADER_H / 2.f });
    node->addChild(label);

    return node;
}

CCNode* PLLPopup::createRow(ListLevel const& level, float width, bool alt) {
    auto node = CCNode::create();
    node->setContentSize({ width - 8.f, ROW_H });
    node->setAnchorPoint({ .5f, .5f });
    float w = width - 8.f;

    auto bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(alt ? 70 : 110);
    bg->setContentSize({ w, ROW_H });
    bg->setPosition({ w / 2.f, ROW_H / 2.f });
    node->addChild(bg);

    // Rank
    bool upllOnly = level.position <= 0;
    auto rankText = upllOnly ? fmt::format("U{}", level.upllPosition) : fmt::format("#{}", level.position);
    auto rank = CCLabelBMFont::create(rankText.c_str(), "bigFont.fnt");
    rank->setScale(.5f);
    rank->limitLabelWidth(40.f, .5f, .2f);
    rank->setAnchorPoint({ 0.f, .5f });
    rank->setPosition({ 7.f, ROW_H / 2.f });
    rank->setColor(upllOnly ? ccColor3B { 255, 90, 90 } : rankColor(level.position));
    node->addChild(rank);

    // Name
    auto name = CCLabelBMFont::create(level.name.c_str(), "bigFont.fnt");
    name->setAnchorPoint({ 0.f, .5f });
    name->setScale(.45f);
    name->limitLabelWidth(175.f, .45f, .15f);
    name->setPosition({ 50.f, ROW_H * .66f });
    node->addChild(name);

    // Publisher / verifier
    std::string subText = fmt::format("by {}", level.publisher.empty() ? level.creators : level.publisher);
    if (!level.verifier.empty()) subText += fmt::format(", verified by {}", level.verifier);
    auto sub = CCLabelBMFont::create(subText.c_str(), "chatFont.fnt");
    sub->setAnchorPoint({ 0.f, .5f });
    sub->setScale(.5f);
    sub->limitLabelWidth(195.f, .5f, .15f);
    sub->setColor({ 200, 200, 200 });
    sub->setPosition({ 50.f, ROW_H * .27f });
    node->addChild(sub);

    // Points
    double pts = upllOnly ? level.upllPoints : level.points;
    auto points = CCLabelBMFont::create(fmt::format("{:.2f} pts", pts).c_str(), "goldFont.fnt");
    points->setAnchorPoint({ 1.f, .5f });
    points->setScale(.45f);
    points->setPosition({ w - 80.f, ROW_H * .66f });
    node->addChild(points);

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    menu->setContentSize({ w, ROW_H });
    node->addChild(menu);

    // Stats: clickable, opens the victors list
    std::string statText = level.requirement > 0
        ? fmt::format("List% {}%  |  {} victors >", level.requirement, level.completions)
        : fmt::format("{} victors >", level.completions);
    auto stats = CCLabelBMFont::create(statText.c_str(), "chatFont.fnt");
    stats->setScale(.45f);
    stats->limitLabelWidth(110.f, .45f, .15f);
    stats->setColor({ 120, 200, 255 });
    auto statsBtn = CCMenuItemSpriteExtra::create(stats, this, menu_selector(PLLPopup::onVictors));
    statsBtn->setTag(static_cast<int>(m_rowLevels.size()));
    m_rowLevels.push_back(level);
    float statsW = stats->getScaledContentSize().width;
    statsBtn->setPosition({ w - 80.f - statsW / 2.f, ROW_H * .27f });
    menu->addChild(statsBtn);

    if (!level.video.empty()) {
        auto ytSpr = CCSprite::createWithSpriteFrameName("gj_ytIcon_001.png");
        ytSpr->setScale(.55f);
        auto ytBtn = CCMenuItemSpriteExtra::create(ytSpr, this, menu_selector(PLLPopup::onVideo));
        ytBtn->setTag(static_cast<int>(m_videos.size()));
        m_videos.push_back(level.video);
        ytBtn->setPosition({ w - 62.f, ROW_H / 2.f });
        menu->addChild(ytBtn);
    }

    auto viewSpr = ButtonSprite::create("View", "bigFont.fnt", "GJ_button_01.png", .6f);
    viewSpr->setScale(.55f);
    auto viewBtn = CCMenuItemSpriteExtra::create(viewSpr, this, menu_selector(PLLPopup::onView));
    viewBtn->setTag(level.gdId);
    viewBtn->setPosition({ w - 25.f, ROW_H / 2.f });
    viewBtn->setEnabled(level.gdId > 0);
    menu->addChild(viewBtn);

    return node;
}
