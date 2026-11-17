#include "VictorsPopup.hpp"
#include "Json.hpp"

namespace {
    constexpr float POPUP_W = 330.f;
    constexpr float POPUP_H = 250.f;
    constexpr float SCROLL_W = 290.f;
    constexpr float SCROLL_H = 160.f;
    constexpr float ROW_H = 26.f;
    constexpr float HEADER_H = 22.f;
    constexpr float GAP = 2.f;
}

VictorsPopup* VictorsPopup::create(std::string const& url, std::string const& levelName) {
    auto ret = new VictorsPopup();
    if (ret->initPopup(url, levelName)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool VictorsPopup::initPopup(std::string const& url, std::string const& levelName) {
    if (!Popup::init(POPUP_W, POPUP_H)) return false;
    m_url = url;

    this->setTitle(levelName.c_str());

    auto size = m_mainLayer->getContentSize();
    float scrollX = (size.width - SCROLL_W) / 2.f;
    float scrollY = 30.f;

    auto bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(80);
    bg->setContentSize({ SCROLL_W + 6.f, SCROLL_H + 6.f });
    bg->setPosition({ size.width / 2.f, scrollY + SCROLL_H / 2.f });
    m_mainLayer->addChild(bg);

    m_scroll = ScrollLayer::create({ SCROLL_W, SCROLL_H });
    m_scroll->setPosition({ scrollX, scrollY });
    m_mainLayer->addChild(m_scroll);

    m_status = CCLabelBMFont::create("Loading...", "bigFont.fnt");
    m_status->setScale(.45f);
    m_status->setAlignment(kCCTextAlignmentCenter);
    m_status->setPosition({ size.width / 2.f, scrollY + SCROLL_H / 2.f });
    m_mainLayer->addChild(m_status, 10);

    m_updated = CCLabelBMFont::create("", "chatFont.fnt");
    m_updated->setScale(.45f);
    m_updated->setColor({ 200, 200, 200 });
    m_updated->setPosition({ size.width / 2.f, 15.f });
    m_mainLayer->addChild(m_updated);

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    menu->setContentSize(size);
    m_mainLayer->addChild(menu);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(.5f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(VictorsPopup::onRefresh));
    refreshBtn->setPosition({ size.width - 22.f, 18.f });
    menu->addChild(refreshBtn);

    this->fetch();

    auto interval = Mod::get()->getSettingValue<int64_t>("refresh-interval");
    if (interval > 0) {
        this->schedule(schedule_selector(VictorsPopup::onAutoRefresh), static_cast<float>(interval));
    }
    return true;
}

void VictorsPopup::setStatus(std::string const& text) {
    m_status->setString(text.c_str());
    m_status->setVisible(!text.empty());
}

void VictorsPopup::onRefresh(CCObject*) {
    m_retry = 0;
    this->fetch();
}

void VictorsPopup::onRetry(float) {
    this->fetch();
}

void VictorsPopup::onAutoRefresh(float) {
    this->fetch();
}

void VictorsPopup::onVideo(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_videos.size())) return;
    web::openLinkInBrowser(m_videos[idx]);
}

void VictorsPopup::fetch() {
    m_updated->setString("Updating...");
    auto req = web::WebRequest();
    req.userAgent("PancetaLevelsList Geode mod");
    m_listener.spawn(
        req.get(m_url),
        [this](web::WebResponse res) {
            this->onResponse(res);
        }
    );
}

void VictorsPopup::onResponse(web::WebResponse const& res) {
    if (!res.ok()) {
        if (res.code() <= 0 && m_retry < 4) {
            m_retry++;
            m_updated->setString(fmt::format("Slow connection, retrying ({}/4)...", m_retry).c_str());
            this->scheduleOnce(schedule_selector(VictorsPopup::onRetry), 1.5f * m_retry);
            return;
        }
        std::string err = res.code() <= 0
            ? fmt::format("Connection error: {}", res.errorMessage())
            : fmt::format("Error HTTP {}", res.code());
        m_updated->setString(err.c_str());
        if (m_scroll->m_contentLayer->getChildrenCount() == 0) {
            this->setStatus("Could not load victors.\nPress refresh to try again.");
        }
        return;
    }
    m_retry = 0;
    auto parsed = res.json();
    if (parsed.isErr()) {
        m_updated->setString("Could not read data");
        return;
    }
    auto json = parsed.unwrap();

    // Build rows
    struct Entry { std::string name, country, video, extra; bool verifier = false; bool header = false; };
    std::vector<Entry> entries;

    if (json.contains("completions") && json["completions"].isArray()) {
        int count = 0;
        for (auto const& c : json["completions"]) { (void)c; count++; }
        entries.push_back({ fmt::format("Victors ({})", count), "", "", "", false, true });
        for (auto const& c : json["completions"]) {
            Entry e;
            e.name = pll::jstr(c, "name");
            e.country = pll::jstr(c, "country");
            e.video = pll::jstr(c, "video");
            e.verifier = pll::jbool(c, "verifier");
            if (!e.name.empty()) entries.push_back(std::move(e));
        }
    }

    if (json.contains("progress") && json["progress"].isArray()) {
        std::vector<Entry> prog;
        for (auto const& p : json["progress"]) {
            Entry e;
            e.name = pll::jstr(p, "name");
            e.country = pll::jstr(p, "country");
            e.video = pll::jstr(p, "video");
            int pct = pll::jint(p, "percent");
            if (pct == 0) pct = pll::jint(p, "progress");
            if (pct == 0) pct = pll::jint(p, "percentage");
            if (pct > 0) e.extra = fmt::format("{}%", pct);
            if (!e.name.empty()) prog.push_back(std::move(e));
        }
        if (!prog.empty()) {
            entries.push_back({ fmt::format("Progress ({})", prog.size()), "", "", "", false, true });
            for (auto& e : prog) entries.push_back(std::move(e));
        }
    }

    // Keep scroll offset across refreshes
    auto content = m_scroll->m_contentLayer;
    float oldH = content->getContentSize().height;
    float offset = content->getPositionY() - (SCROLL_H - oldH);
    bool first = content->getChildrenCount() == 0;

    content->removeAllChildren();
    m_videos.clear();

    if (entries.size() <= 1) {
        this->setStatus("No victors yet.");
    } else {
        this->setStatus("");
    }

    float total = GAP;
    for (auto const& e : entries) total += (e.header ? HEADER_H : ROW_H) + GAP;
    float height = std::max(total, SCROLL_H);
    content->setContentSize({ SCROLL_W, height });

    float y = height - GAP;
    bool alt = false;
    for (auto const& e : entries) {
        float h = e.header ? HEADER_H : ROW_H;
        auto node = CCNode::create();
        node->setContentSize({ SCROLL_W - 8.f, h });
        node->setAnchorPoint({ .5f, .5f });
        node->setPosition({ SCROLL_W / 2.f, y - h / 2.f });
        float w = SCROLL_W - 8.f;

        if (e.header) {
            auto label = CCLabelBMFont::create(e.name.c_str(), "goldFont.fnt");
            label->setScale(.55f);
            label->setPosition({ w / 2.f, h / 2.f });
            node->addChild(label);
        } else {
            auto bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
            bg->setColor({ 0, 0, 0 });
            bg->setOpacity(alt ? 70 : 110);
            bg->setContentSize({ w, h });
            bg->setPosition({ w / 2.f, h / 2.f });
            node->addChild(bg);
            alt = !alt;

            auto name = CCLabelBMFont::create(e.name.c_str(), "bigFont.fnt");
            name->setAnchorPoint({ 0.f, .5f });
            name->setScale(.4f);
            name->limitLabelWidth(150.f, .4f, .15f);
            name->setPosition({ 8.f, h / 2.f });
            if (e.verifier) name->setColor({ 255, 215, 0 });
            node->addChild(name);

            std::string right = e.verifier ? "Verifier" : e.extra;
            if (!e.country.empty()) right = right.empty() ? e.country : fmt::format("{}  {}", right, e.country);
            auto info = CCLabelBMFont::create(right.c_str(), "chatFont.fnt");
            info->setAnchorPoint({ 1.f, .5f });
            info->setScale(.5f);
            info->setColor({ 200, 200, 200 });
            info->setPosition({ w - 38.f, h / 2.f });
            node->addChild(info);

            if (!e.video.empty()) {
                auto menu = CCMenu::create();
                menu->setPosition({ 0.f, 0.f });
                menu->setContentSize({ w, h });
                node->addChild(menu);

                auto ytSpr = CCSprite::createWithSpriteFrameName("gj_ytIcon_001.png");
                ytSpr->setScale(.45f);
                auto ytBtn = CCMenuItemSpriteExtra::create(ytSpr, this, menu_selector(VictorsPopup::onVideo));
                ytBtn->setTag(static_cast<int>(m_videos.size()));
                m_videos.push_back(e.video);
                ytBtn->setPosition({ w - 16.f, h / 2.f });
                menu->addChild(ytBtn);
            }
        }

        content->addChild(node);
        y -= h + GAP;
    }

    if (first) {
        m_scroll->moveToTop();
    } else {
        float minY = SCROLL_H - height;
        content->setPositionY(std::clamp(minY + offset, minY, 0.f));
    }

    m_updated->setString(fmt::format("Updated {}", pll::nowString()).c_str());
}
