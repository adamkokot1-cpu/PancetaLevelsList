#include "PLLPopup.hpp"

#include <array>
#include <optional>
#include <algorithm>

namespace {
    constexpr float POPUP_W = 440.f;
    constexpr float POPUP_H = 290.f;
    constexpr float SCROLL_W = 400.f;
    constexpr float SCROLL_H = 192.f;
    constexpr float ROW_H = 42.f;
    constexpr float HEADER_H = 26.f;
    constexpr float GAP = 3.f;

    // Cached list per tab, so reopening the popup does not refetch.
    std::array<std::optional<ListData>, 2> s_cache;

    std::string jstr(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return "";
        return v[key].asString().unwrapOr("");
    }

    int jint(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return 0;
        return static_cast<int>(v[key].asInt().unwrapOr(0));
    }

    double jnum(matjson::Value const& v, std::string_view key) {
        if (!v.contains(key)) return 0.0;
        return v[key].asDouble().unwrapOr(0.0);
    }

    ListData parseList(matjson::Value const& json) {
        ListData data;

        if (json.contains("categories") && json["categories"].isArray()) {
            for (auto const& c : json["categories"]) {
                data.categories.push_back({
                    jstr(c, "key"),
                    jstr(c, "name"),
                    jint(c, "from"),
                    jint(c, "to"),
                });
            }
        }

        if (json.contains("levels") && json["levels"].isArray()) {
            for (auto const& l : json["levels"]) {
                ListLevel lvl;
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
                if (!lvl.name.empty()) data.levels.push_back(std::move(lvl));
            }
        }

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
    float scrollY = 18.f;

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

    this->load(false);
    return true;
}

void PLLPopup::onTab(CCObject* sender) {
    auto tab = static_cast<ListTab>(static_cast<CCNode*>(sender)->getTag());
    if (tab == m_tab) return;
    m_tab = tab;

    m_pllTabSpr->updateBGImage(tab == ListTab::PLL ? "GJ_button_01.png" : "GJ_button_04.png");
    m_gpllTabSpr->updateBGImage(tab == ListTab::GPLL ? "GJ_button_01.png" : "GJ_button_04.png");

    this->load(false);
}

void PLLPopup::onRefresh(CCObject*) {
    this->load(true);
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

void PLLPopup::setStatus(std::string const& text) {
    m_status->setString(text.c_str());
    m_status->setVisible(!text.empty());
}

void PLLPopup::clearList() {
    m_scroll->m_contentLayer->removeAllChildren();
    m_scroll->m_contentLayer->setContentSize({ SCROLL_W, SCROLL_H });
    m_videos.clear();
}

void PLLPopup::load(bool force) {
    auto idx = static_cast<size_t>(m_tab);

    if (!force && s_cache[idx]) {
        this->populate(*s_cache[idx]);
        return;
    }

    auto url = Mod::get()->getSettingValue<std::string>(
        m_tab == ListTab::PLL ? "pll-api-url" : "gpll-api-url"
    );

    this->clearList();
    m_listener.cancel();

    if (url.empty()) {
        this->setStatus(m_tab == ListTab::GPLL
            ? "GPLL is coming soon!\nNo levels yet."
            : "No PLL API URL set\n(check mod settings).");
        return;
    }

    this->setStatus("Loading...");

    auto req = web::WebRequest();
    req.userAgent("PancetaLevelsList Geode mod");

    auto tab = static_cast<int>(m_tab);
    m_listener.spawn(
        req.get(url),
        [this, tab](web::WebResponse res) {
            this->onResponse(tab, res);
        }
    );
}

void PLLPopup::onResponse(int tab, web::WebResponse const& res) {
    bool current = static_cast<int>(m_tab) == tab;

    if (!res.ok()) {
        if (current) {
            if (res.code() == 0) {
                this->setStatus("Network error.\nPress refresh to try again.");
            } else {
                this->setStatus(fmt::format("Failed to load list (HTTP {}).", res.code()));
            }
        }
        return;
    }

    auto parsed = res.json();
    if (parsed.isErr()) {
        if (current) this->setStatus("Could not read list data.");
        return;
    }

    s_cache[static_cast<size_t>(tab)] = parseList(parsed.unwrap());
    if (current) this->populate(*s_cache[static_cast<size_t>(tab)]);
}

void PLLPopup::populate(ListData const& data) {
    this->clearList();

    std::vector<ListLevel const*> mainLevels;
    std::vector<ListLevel const*> upllLevels;
    for (auto const& lvl : data.levels) {
        if (lvl.position > 0) mainLevels.push_back(&lvl);
        else if (lvl.list == "upll") upllLevels.push_back(&lvl);
    }
    std::sort(mainLevels.begin(), mainLevels.end(), [](ListLevel const* a, ListLevel const* b) { return a->position < b->position; });
    std::sort(upllLevels.begin(), upllLevels.end(), [](ListLevel const* a, ListLevel const* b) { return a->upllPosition < b->upllPosition; });

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
        this->setStatus("No levels on this list yet.");
        return;
    }
    this->setStatus("");

    float total = GAP;
    for (auto n : nodes) total += n->getContentSize().height + GAP;
    float height = std::max(total, SCROLL_H);

    auto content = m_scroll->m_contentLayer;
    content->setContentSize({ SCROLL_W, height });

    float y = height - GAP;
    for (auto n : nodes) {
        auto h = n->getContentSize().height;
        n->setPosition({ SCROLL_W / 2.f, y - h / 2.f });
        content->addChild(n);
        y -= h + GAP;
    }

    m_scroll->moveToTop();
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

    // Points + stats
    double pts = upllOnly ? level.upllPoints : level.points;
    auto points = CCLabelBMFont::create(fmt::format("{:.2f} pts", pts).c_str(), "goldFont.fnt");
    points->setAnchorPoint({ 1.f, .5f });
    points->setScale(.45f);
    points->setPosition({ w - 80.f, ROW_H * .66f });
    node->addChild(points);

    std::string statText = level.requirement > 0
        ? fmt::format("List% {}%  |  {} victors", level.requirement, level.completions)
        : fmt::format("{} victors", level.completions);
    auto stats = CCLabelBMFont::create(statText.c_str(), "chatFont.fnt");
    stats->setAnchorPoint({ 1.f, .5f });
    stats->setScale(.45f);
    stats->limitLabelWidth(100.f, .45f, .15f);
    stats->setColor({ 200, 200, 200 });
    stats->setPosition({ w - 80.f, ROW_H * .27f });
    node->addChild(stats);

    // Buttons
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    menu->setContentSize({ w, ROW_H });
    node->addChild(menu);

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
