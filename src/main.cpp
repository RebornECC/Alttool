#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

// ========================================
// PRISM FORGE — Editor Quick Panel
// Monochrome UI: чёрный фон, белая рамка, Arial
// ========================================

// Плоская кнопка: полупрозрачная белая заливка, тонкая рамка, белый текст.
static CCNode* makeButtonNode(char const* text, float w, float h, float fontSize) {
    auto bg = CCLayerColor::create(ccc4(255, 255, 255, 28), w, h);
    bg->ignoreAnchorPointForPosition(false);
    bg->setAnchorPoint({0.5f, 0.5f});

    auto line = [&](float x, float y, float lw, float lh) {
        auto l = CCLayerColor::create(ccc4(255, 255, 255, 110), lw, lh);
        l->setPosition({x, y});
        bg->addChild(l);
    };
    line(0, 0, w, 1);
    line(0, h - 1, w, 1);
    line(0, 0, 1, h);
    line(w - 1, 0, 1, h);

    auto label = CCLabelTTF::create(text, "Arial", fontSize);
    label->setColor(ccc3(255, 255, 255));
    label->setPosition({w / 2, h / 2});
    label->setID("label");
    bg->addChild(label);
    return bg;
}

class QuickPanel : public CCLayerColor {
protected:
    static constexpr int   COLS   = 4;
    static constexpr int   ROWS   = 4;
    static constexpr float CELL_W = 82.f;
    static constexpr float CELL_H = 32.f;
    static constexpr float GAP    = 4.f;
    static constexpr float PAD    = 8.f;
    static constexpr float HEAD_H = 30.f;

    EditorUI*      m_ui        = nullptr;
    CCLabelTTF*    m_title     = nullptr;
    CCLabelTTF*    m_toggleLbl = nullptr;
    CCMenuItemSpriteExtra* m_toggleBtn = nullptr;
    CCMenu*        m_header    = nullptr;
    CCMenu*        m_body      = nullptr;
    CCLayerColor*  m_border[4] = {nullptr, nullptr, nullptr, nullptr};
    CCLayerColor*  m_divider   = nullptr;
    bool    m_collapsed = false;
    bool    m_dragging  = false;
    CCPoint m_grab      = {0, 0};

    bool init(EditorUI* ui) {
        if (!CCLayerColor::initWithColor(ccc4(0, 0, 0, 255))) return false;
        m_ui = ui;

        auto mod = Mod::get();
        float scale = static_cast<float>(mod->getSettingValue<double>("panel-scale"));
        int opacity = static_cast<int>(mod->getSettingValue<int64_t>("panel-opacity"));
        m_collapsed = mod->getSavedValue<bool>("collapsed", mod->getSettingValue<bool>("start-collapsed"));

        this->setOpacity(static_cast<GLubyte>(opacity));
        this->setAnchorPoint({0, 0});
        this->ignoreAnchorPointForPosition(false);
        this->setScale(scale);

        // Рамка
        for (auto& b : m_border) {
            b = CCLayerColor::create(ccc4(255, 255, 255, 200), 1, 1);
            this->addChild(b, 5);
        }
        m_divider = CCLayerColor::create(ccc4(255, 255, 255, 90), 1, 1);
        this->addChild(m_divider, 5);

        // Заголовок
        m_title = CCLabelTTF::create("PRISM FORGE", "Arial", 12.f);
        m_title->setColor(ccc3(255, 255, 255));
        m_title->setAnchorPoint({0, 0.5f});
        this->addChild(m_title, 6);

        // Кнопка свернуть / развернуть
        m_header = CCMenu::create();
        m_header->setPosition({0, 0});
        auto toggleNode = makeButtonNode(m_collapsed ? "+" : "-", 34.f, 22.f, 14.f);
        m_toggleLbl = static_cast<CCLabelTTF*>(toggleNode->getChildByID("label"));
        m_toggleBtn = CCMenuItemExt::createSpriteExtra(toggleNode, [this](CCMenuItemSpriteExtra*) {
            this->setCollapsed(!m_collapsed);
        });
        m_header->addChild(m_toggleBtn);
        this->addChild(m_header, 6);

        // Кнопки
        m_body = CCMenu::create();
        m_body->setPosition({0, 0});
        this->addChild(m_body, 6);
        this->buildButtons();

        this->applyLayout();

        auto win = CCDirector::get()->getWinSize();
        float w = COLS * CELL_W + PAD * 2;
        this->setPosition({
            mod->getSavedValue<float>("pos-x", win.width - w * scale - 10.f),
            mod->getSavedValue<float>("pos-y", win.height * 0.5f - 60.f)
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
        auto node = makeButtonNode(text, CELL_W - GAP, CELL_H - GAP, 10.f);
        auto btn = CCMenuItemExt::createSpriteExtra(node, [cb = std::move(cb)](CCMenuItemSpriteExtra*) { cb(); });
        btn->setPosition({
            PAD + CELL_W * (c + 0.5f),
            PAD + CELL_H * (ROWS - 1 - r + 0.5f)
        });
        m_body->addChild(btn);
    }

    void buildButtons() {
        auto ui = m_ui;
        int i = 0;
        addBtn(i++, "UNDO",      [ui] { ui->undoLastAction(nullptr); });
        addBtn(i++, "REDO",      [ui] { ui->redoLastAction(nullptr); });
        addBtn(i++, "COPY",      [ui] { ui->onCopy(nullptr); });
        addBtn(i++, "PASTE",     [ui] { ui->onPaste(nullptr); });
        addBtn(i++, "DUPLICATE", [ui] { ui->onDuplicate(nullptr); });
        addBtn(i++, "DELETE",    [ui] { ui->onDeleteSelected(nullptr); });
        addBtn(i++, "GROUP",     [ui] { ui->onGroupUp(nullptr); });
        addBtn(i++, "UNGROUP",   [ui] { ui->onUngroup(nullptr); });
        addBtn(i++, "ROT -45",   [ui] { ui->transformObjectCall(EditCommand::RotateCCW45); });
        addBtn(i++, "ROT +45",   [ui] { ui->transformObjectCall(EditCommand::RotateCW45); });
        addBtn(i++, "FLIP X",    [ui] { ui->transformObjectCall(EditCommand::FlipX); });
        addBtn(i++, "FLIP Y",    [ui] { ui->transformObjectCall(EditCommand::FlipY); });
        addBtn(i++, "DESELECT",  [ui] { ui->onDeselectAll(nullptr); });
        addBtn(i++, "SNAP",      [ui] { ui->toggleSnap(nullptr); });
        addBtn(i++, "ZOOM +",    [ui] { ui->zoomIn(nullptr); });
        addBtn(i++, "ZOOM -",    [ui] { ui->zoomOut(nullptr); });
    }

    void setCollapsed(bool value) {
        m_collapsed = value;
        Mod::get()->setSavedValue("collapsed", value);
        m_toggleLbl->setString(value ? "+" : "-");
        this->applyLayout();
        this->clampToScreen();
    }

    // Пересчёт размеров; верхний край панели остаётся на месте.
    void applyLayout() {
        float w = COLS * CELL_W + PAD * 2;
        float h = m_collapsed ? HEAD_H : HEAD_H + ROWS * CELL_H + PAD;
        float oldH = this->getContentSize().height;

        this->changeWidthAndHeight(w, h);
        if (oldH > 0.f) {
            this->setPositionY(this->getPositionY() + (oldH - h) * this->getScale());
        }

        m_border[0]->changeWidthAndHeight(w, 1);  m_border[0]->setPosition({0, 0});
        m_border[1]->changeWidthAndHeight(w, 1);  m_border[1]->setPosition({0, h - 1});
        m_border[2]->changeWidthAndHeight(1, h);  m_border[2]->setPosition({0, 0});
        m_border[3]->changeWidthAndHeight(1, h);  m_border[3]->setPosition({w - 1, 0});

        m_divider->changeWidthAndHeight(w - PAD * 2, 1);
        m_divider->setPosition({PAD, h - HEAD_H});
        m_divider->setVisible(!m_collapsed);

        m_title->setPosition({PAD, h - HEAD_H / 2});
        m_toggleBtn->setPosition({w - PAD - 17.f, h - HEAD_H / 2});
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
