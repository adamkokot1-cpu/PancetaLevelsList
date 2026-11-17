#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>

#include "PLLPopup.hpp"

using namespace geode::prelude;

static CCMenuItemSpriteExtra* createPLLButton(CCObject* target, SEL_MenuHandler selector) {
    auto label = CCLabelBMFont::create("PLL", "bigFont.fnt");
    label->setScale(.55f);
    auto spr = CircleButtonSprite::create(label, CircleBaseColor::Green, CircleBaseSize::MediumAlt);
    auto btn = CCMenuItemSpriteExtra::create(spr, target, selector);
    btn->setID("pll-button"_spr);
    return btn;
}

// Button on the main menu (bottom row)
class $modify(PLLMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Warm up the connection and download the list in the background
        PLLPopup::prefetch();

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(createPLLButton(this, menu_selector(PLLMenuLayer::onPLL)));
            menu->updateLayout();
        }
        return true;
    }

    void onPLL(CCObject*) {
        if (auto popup = PLLPopup::create()) popup->show();
    }
};

// Button in the Create/online tab (next to the other lists)
class $modify(PLLCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        if (auto menu = this->getChildByID("bottom-left-menu")) {
            menu->addChild(createPLLButton(this, menu_selector(PLLCreatorLayer::onPLL)));
            menu->updateLayout();
        }
        return true;
    }

    void onPLL(CCObject*) {
        if (auto popup = PLLPopup::create()) popup->show();
    }
};
