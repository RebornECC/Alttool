#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class QuickPanel : public CCLayer {
protected:
    static constexpr int   COLS   = 4;
    static constexpr int   ROWS   = 4;
    static constexpr float CELL_W = 80.f;
    static constexpr float CELL_H = 40.f;
    static constexpr float PAD    = 8.f;
    static constexpr float HEAD_H = 36.f;

    EditorUI*       m_ui        = nullptr;
    CCScale9Sprite* m_bg        = nullptr;
    CCLabelBMFont*  m_title     = nullptr;
    CCMenu*         m_header    = nullptr;
    CCMenu*         m_body      = nullptr;
    ButtonSprite*   m_toggleSpr = nullptr;
    CCMenuItemSpriteExtra* m_toggleBtn = nullptr;
    bool    m_collapsed = false;
    bool    m_dragging  = false;
    CCPoint m_grab      = {0, 0};

    bool init(EditorUI* ui) {
        if (!CCLayer::init()) return false;
        m_ui = ui;

        auto mod = Mod::get();
        float scale = static_cast<float>(mod->getSettingValue<double>("panel-scale"));
        int opacity = static_cast<int>(mod->getSettingValue<int64_t>("panel-opacity"));
        m_collapsed = mod->getSavedValue<bool>("collapsed", mod->getSettingValue<bool>("start-collapsed"));

        this->setAnchorPoint({0, 0});
        this->ignoreAnchorPointForPosition(false);
        this->setScale(scale);

        m_bg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
        m_bg->setColor({0, 0, 0});
        m_bg->setOpacity(opacity);
        m_bg->setAnchorPoint({0, 0});
        this->addChild(m_bg, -1);

        m_title = CCLabelBMFont::create("Quick Panel", "goldFont.fnt");
        m_title->setScale(0.55f);
        m_title->setAnchorPoint({0, 0.5f});
        this->addChild(m_title);

        m_header = CCMenu::create();
        m_header->setPosition({0, 0});
        m_toggleSpr = ButtonSprite::create(m_collapsed ? "+" : "-", 30, true, "bigFont.fnt", "GJ_button_06.png", 28.f, 0.8f);
        m_toggleBtn = CCMenuItemExt::createSpriteExtra(m_toggleSpr, [this](CCMenuItemSpriteExtra*) {
            this->setCollapsed(!m_collapsed);
        });
        m_header->addChild(m_toggleBtn);
        this->addChild(m_header);

        m_body = CCMenu::create();
        m_body->setPosition({0, 0});
        this->addChild(m_body);
        this->buildButtons();

        this->applyLayout();

        auto win = CCDirector::get()->getWinSize();
        float w = COLS * CELL_W + PAD * 2;
        float defX = win.width - w * scale - 10.f;
        float defY = win.height * 0.5f - 60.f;
        this->setPosition({
            mod->getSavedValue<float>("pos-x", defX),
            mod->getSavedValue<float>("pos-y", defY)
        });
        this->clampToScreen();

        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-127);
        this->setTouchEnabled(true);
        return true;
    }

    void addBtn(int idx, char const* text, std::function<void()> cb) {
        int c = idx % COLS;
        int r = idx / COLS;
        auto spr = ButtonSprite::create(text, static_cast<int>(CELL_W - 6), true, "bigFont.fnt", "GJ_button_04.png", CELL_H - 8, 0.55f);
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [cb = std::move(cb)](CCMenuItemSpriteExtra*) { cb(); });
        btn->setPosition({
            PAD + CELL_W * (c + 0.5f),
            PAD + CELL_H * (ROWS - 1 - r + 0.5f)
        });
        m_body->addChild(btn);
    }

    void buildButtons() {
        auto ui = m_ui;
        int i = 0;
        addBtn(i++, "Undo",   [ui] { ui->undoLastAction(nullptr); });
        addBtn(i++, "Redo",   [ui] { ui->redoLastAction(nullptr); });
        addBtn(i++, "Copy",   [ui] { ui->onCopy(nullptr); });
        addBtn(i++, "Paste",  [ui] { ui->onPaste(nullptr); });
        addBtn(i++, "Dupl.",  [ui] { ui->onDuplicate(nullptr); });
        addBtn(i++, "Delete", [ui] { ui->onDeleteSelected(nullptr); });
        addBtn(i++, "Group",  [ui] { ui->onGroupUp(nullptr); });
        addBtn(i++, "Ungrp",  [ui] { ui->onUngroup(nullptr); });
        addBtn(i++, "Rot -45", [ui] { ui->transformObjectCall(EditCommand::RotateCCW45); });
        addBtn(i++, "Rot +45", [ui] { ui->transformObjectCall(EditCommand::RotateCW45); });
        addBtn(i++, "Flip X",  [ui] { ui->transformObjectCall(EditCommand::FlipX); });
        addBtn(i++, "Flip Y",  [ui] { ui->transformObjectCall(EditCommand::FlipY); });
        addBtn(i++, "Desel.",  [ui] { ui->onDeselectAll(nullptr); });
        addBtn(i++, "Snap",    [ui] { ui->toggleSnap(nullptr); });
        addBtn(i++, "Zoom +",  [ui] { ui->zoomIn(nullptr); });
        addBtn(i++, "Zoom -",  [ui] { ui->zoomOut(nullptr); });
    }

    void setCollapsed(bool value) {
        m_collapsed = value;
        Mod::get()->setSavedValue("collapsed", value);
        m_toggleSpr->setString(value ? "+" : "-");
        this->applyLayout();
        this->clampToScreen();
    }

    void applyLayout() {
        float w = COLS * CELL_W + PAD * 2;
        float h = m_collapsed ? HEAD_H : HEAD_H + ROWS * CELL_H + PAD;
        float oldH = this->getContentSize().height;

        this->setContentSize({w, h});
        if (oldH > 0.f) {
            this->setPositionY(this->getPositionY() + (oldH - h) * this->getScale());
        }

        m_bg->setContentSize({w, h});
        m_title->setPosition({PAD, h - HEAD_H / 2});
        m_toggleBtn->setPosition({w - PAD - 15.f, h - HEAD_H / 2});
        m_body->setVisible(!m_collapsed);
        m_body->setEnabled(!m_collapsed);
    }

    void clampToScreen() {
        auto win = CCDirector::get()->getWinSize();
        auto size = this->getContentSize() * this->getScale();
        auto p = this->getPosition();
        p.x = std::clamp(p.x, 0.f, std::max(0.f, win.width - size.width));
        p.y = std::clamp(p.y, 0.f, std::max(0.f, win.height - size.height));
        this->setPosition(p);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!this->isVisible()) return false;
        auto local = this->convertToNodeSpace(touch->getLocation());
        auto size = this->getContentSize();
        if (!CCRect{0, 0, size.width, size.height}.containsPoint(local)) return false;
        if (local.y >= size.height - HEAD_H) {
            m_dragging = true;
            m_grab = this->getPosition() - this->getParent()->convertToNodeSpace(touch->getLocation());
        }
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        this->setPosition(this->getParent()->convertToNodeSpace(touch->getLocation()) + m_grab);
        this->clampToScreen();
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override { this->finishDrag(); }
    void ccTouchCancelled(CCTouch*, CCEvent*) override { this->finishDrag(); }

    void finishDrag() {
        if (!m_dragging) return;
        m_dragging = false;
        Mod::get()->setSavedValue("pos-x", this->getPositionX());
        Mod::get()->setSavedValue("pos-y", this->getPositionY());
    }

public:
    static QuickPanel* create(EditorUI* ui) {
        auto ret = new QuickPanel();
        if (ret && ret->init(ui)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(QuickPanelEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto panel = QuickPanel::create(this);
        if (panel) {
            panel->setID("quick-panel"_spr);
            this->addChild(panel, 100);
        }
        return true;
    }
};
