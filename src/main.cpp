#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "ChatConfigPopup.hpp"

using namespace geode::prelude;

static std::vector<std::string> GD_PLAYERS = {
    // Streamers
    "Michigun", "Viprin", "Riot", "Juniper", "Wulzy",
    "EVW", "Doggie", "Nexus", "AeonAir", "Tride",
    "Cyclic", "Knobbelboy", "Sunix", "Technical49", "Dorami",
    "SpaceUK", "Diamond", "Trick", "Zoink", "Nswish",
    "Cursed", "BlassCFB", "MiKhaXx", "Mullsy", "Luqualizer",
    "Npesta", "xanii", "BTD6", "Cataclysm",
    "Krazyman50", "Zobros", "Sea1997", "Pennutoh", "FunnyGame",
    "TrusTa", "RicoLP", "ViPriN", "ChaSe", "Lemons",
    "Vortrox",
    // Contributors
    "Axiom", "Human", "siniNight"
};

// static std::vector<bool> hasSpoken;
static const float CHAT_WIDTH = 135.0f;
static const float CHAT_HEIGHT = 170.0f;
static const float HEADER_HEIGHT = 16.0f;
static const float MSG_AREA_HEIGHT = CHAT_HEIGHT - HEADER_HEIGHT;
static const float MSG_LINE_HEIGHT = 11.0f;
static const float CHAT_PADDING = 4.0f;
static const float TEXT_SCALE = 0.27f;
static const int MAX_MESSAGES = 16;

static const ccColor3B TWITCH_COLORS[] = {
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
    {178, 70, 255},
    {255, 105, 180},
    {30, 144, 255},
    {0, 255, 127},
    {255, 165, 0},
    {255, 215, 0},
    {255, 127, 80},
    {100, 149, 237},
    {144, 238, 144},
    {255, 20, 147},
    {64, 224, 208},
    {255, 99, 71},
};
static const int TWITCH_COLOR_COUNT = 15;

static ccColor3B colorForUsername(const std::string& name) {
    size_t hash = 0;
    for (char c : name) hash = hash * 31 + c;
    return TWITCH_COLORS[hash % TWITCH_COLOR_COUNT];
}

static std::string randomPlayer() {
    return GD_PLAYERS[rand() % GD_PLAYERS.size()];
}

struct ChatMessage {
    std::string username;
    std::string text;
};

static ChatMessage parseChatMessage(const std::string& raw) {
    size_t colon = raw.find(": ");
    if (colon == std::string::npos) return {"", raw};
    return {raw.substr(0, colon), raw.substr(colon + 2)};
}

static std::string chat(const std::string& msg) {
    return randomPlayer() + ": " + msg;
}

static std::string wrapText(const std::string& msg, int maxChars = 25, int firstLineOffset = 0) {
    std::string result;
    int lineLen = firstLineOffset;
    for (size_t i = 0; i < msg.size(); ) {
        size_t wordEnd = msg.find(' ', i);
        if (wordEnd == std::string::npos) wordEnd = msg.size();
        std::string word = msg.substr(i, wordEnd - i);

        while ((int)word.size() > maxChars) {
            int space = maxChars - lineLen;
            if (space <= 0) {
                if (!result.empty()) result += '\n';
                lineLen = 0;
                continue;
            }
            if (!result.empty() && lineLen > 0) result += '\n';
            result += word.substr(0, space);
            word = word.substr(space);
            result += '\n';
            lineLen = 0;
        }

        if (lineLen > 0 && lineLen + 1 + (int)word.size() > maxChars) {
            result += '\n';
            lineLen = 0;
        }
        if (lineLen > 0) { result += ' '; lineLen++; }
        result += word;
        lineLen += (int)word.size();

        i = wordEnd;
        while (i < msg.size() && msg[i] == ' ') i++;
    }
    return result;
}

static const std::vector<std::string> IDLE_MESSAGES = {
    "gl hf",
    "lets go",
    "you got this",
    "gg",
    "pog",
    "POGGERS",
    "lets gooo",
    "hype",
    "W",
    "W run",
    "bro is cooking",
    "no way",
    "actually insane",
    "craaaazy",
    "bro",
    "what",
    "lol",
    "sheesh",
    "clean",
    "smooth af",
    "frfr",
    "real",
    "godlike",
    "heat check",
    "HEAT",
    "actually going",
    "nah bro",
    "omg",
    "OMGG",
    "go go go",
    "this is it",
    "the run",
    "W player",
    "not dropping",
    "stay focused",
    "in the zone",
    "locked in",
    "monkaS",
    "prayge",
    "copium",
    "he's actually doing it",
    "no shot",
    "bro woke up",
    "diff",
    "cooked",
    "certified W",
    "lets actually go",
    "i believe",
    "trust",
    "real ones watching",
    "OMFG",
    "actually poggers",
    "not missing",
    "clean inputs",
    "GG EZ",
    "he's built different",
    "W grinder",
    "insane player",
    "demon time",
    "he's not stopping",
};

static const std::vector<std::string> START_MESSAGES = {
    "he's starting",
    "here we go",
    "attempt time",
    "lets see it",
    "W incoming",
    "is this the run",
    "woke up and chose violence",
    "good luck bro",
    "gl gl gl",
    "GLHF",
    "you got this fr",
    "the grind continues",
    "back at it",
    "W attempt pls",
    "another one",
    "let him cook",
    "cooking rn",
    "o7",
    "real",
    "pog",
};

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        CCNodeRGBA* m_chatRoot = nullptr;
        CCLayerColor* m_chatBg = nullptr;
        CCLayerColor* m_header = nullptr;
        CCLabelBMFont* m_headerLabel = nullptr;
        CCLabelBMFont* m_liveLabel = nullptr;
        CCLayerColor* m_liveBadge = nullptr;
        CCNode* m_msgContainer = nullptr;
        std::vector<CCNode*> m_messageRows;
        std::vector<int> m_rowHeights;
        float m_randomChatTimer = 0.0f;
        float m_nextChatDelay = 1.8f;
        float m_deathChatTimer = 0.0f;
        float m_deathSpamDuration = 2.0f;
        bool m_isDeathSpamming = false;
        float deathPercent = 5.0f;
        float holdPercent = 22.0f;
        float goPercent = 37.0f;
        float superGoPercent = 80.0f;
        float ggPercent = 99.9999f;
        int att = 0;
        bool enabled = false;
        bool m_echoClipPresent = false;
        bool m_clipMessageFired = false;
        float m_bestPercent = 0.0f;
        float m_idleChatTimer = 0.0f;
        float m_nextIdleDelay = 2.8f;
        int m_numViewers = 69;
        float m_lastAttPercent = 0.0f;
        float m_opacity = 1.0f;
        GJGameLevel* m_lvl;
        std::string m_font = "bigFont";
    };

public:
    void reloadThresholds() {
        if (!m_level) return;
        auto fields = m_fields.self();
        fields->holdPercent = loadPercentForLevel(m_level->m_levelID, "hold-percent", 22.0f);
        fields->goPercent = loadPercentForLevel(m_level->m_levelID, "go-percent", 37.0f);
        fields->superGoPercent = loadPercentForLevel(m_level->m_levelID, "supergo-percent", 80.0f);
        fields->ggPercent = loadPercentForLevel(m_level->m_levelID, "gg-percent", 99.9999f);
        fields->enabled = loadDisabledForLevel(m_level->m_levelID, "enabled", Mod::get()->getSettingValue<int>("enabled-by-default"));
        fields->m_numViewers = Mod::get()->getSettingValue<int>("viewer-count");
        fields->m_font = Mod::get()->getSettingValue<std::string>("font") + ".fnt";
        fields->m_opacity = Mod::get()->getSettingValue<float>("opacity");
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        this->reloadThresholds();

        auto fields = m_fields.self();
        fields->m_lvl = level;
        fields->m_bestPercent = (float)level->getNormalPercent();
        fields->m_echoClipPresent = Loader::get()->isModLoaded("axiom.echoclip");

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        float chatX = winSize.width - CHAT_WIDTH - 5.0f + Mod::get()->getSettingValue<int>("x-off");
        float chatY = 5.0f + Mod::get()->getSettingValue<int>("y-off");

        // chat root node
        fields->m_chatRoot = CCNodeRGBA::create();
        fields->m_chatRoot->setPosition({chatX, chatY});
        fields->m_chatRoot->setZOrder(100);
        this->m_uiLayer->addChild(fields->m_chatRoot);

        // chat box bg
        fields->m_chatBg = CCLayerColor::create({14, 14, 18, 200}, CHAT_WIDTH, CHAT_HEIGHT);
        fields->m_chatBg->setPosition({0, 0});
        fields->m_chatBg->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_chatBg);

        // header bar
        fields->m_header = CCLayerColor::create({100, 55, 200, 255}, CHAT_WIDTH, HEADER_HEIGHT);
        fields->m_header->setPosition({0, CHAT_HEIGHT - HEADER_HEIGHT});
        fields->m_header->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_header);

        // live badge
        fields->m_liveBadge = CCLayerColor::create({230, 30, 30, 255}, 18.0f, 9.0f);
        fields->m_liveBadge->setPosition({CHAT_PADDING, CHAT_HEIGHT - HEADER_HEIGHT + 3.5f});
        fields->m_liveBadge->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_liveBadge);

        // live text
        fields->m_liveLabel = CCLabelBMFont::create("LIVE", fields->m_font.c_str());
        fields->m_liveLabel->setScale(0.14f);
        fields->m_liveLabel->setColor({255, 255, 255});
        fields->m_liveLabel->setAnchorPoint({0.5f, 0.5f});
        fields->m_liveLabel->setPosition({CHAT_PADDING + 9.0f, CHAT_HEIGHT - HEADER_HEIGHT + 8.0f});
        fields->m_liveLabel->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_liveLabel);

        // header label
        fields->m_headerLabel = CCLabelBMFont::create("CHAT", fields->m_font.c_str());
        fields->m_headerLabel->setScale(0.22f);
        fields->m_headerLabel->setColor({255, 255, 255});
        fields->m_headerLabel->setAnchorPoint({0.0f, 0.5f});
        fields->m_headerLabel->setPosition({CHAT_PADDING + 24.0f, CHAT_HEIGHT - HEADER_HEIGHT / 2.0f});
        fields->m_headerLabel->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_headerLabel);

        // viewer count
        fields->m_headerLabel = CCLabelBMFont::create(std::to_string(fields->m_numViewers).c_str(), fields->m_font.c_str()); // this is stupid... for the reviewing staff pls give a better way to do ts (i am very sorry if this causes you pain)
        fields->m_headerLabel->setScale(0.22f);
        fields->m_headerLabel->setColor({151, 18, 17});
        fields->m_headerLabel->setAnchorPoint({0.0f, 0.5f});
        fields->m_headerLabel->setPosition({CHAT_WIDTH - 35.0f, CHAT_HEIGHT - HEADER_HEIGHT / 2.0f});
        fields->m_headerLabel->setOpacity(static_cast<GLubyte>(fields->m_opacity*255));
        fields->m_chatRoot->addChild(fields->m_headerLabel);

        // msg container
        fields->m_msgContainer = CCNode::create();
        fields->m_msgContainer->setPosition({0, 0});
        fields->m_chatRoot->addChild(fields->m_msgContainer);

        // spawn some msgs on level start
        int startCount = 3 + rand() % 3;
        for (int i = 0; i < startCount; i++) {
            addChatMessage(chat(START_MESSAGES[rand() % START_MESSAGES.size()]));
        }

        this->schedule(schedule_selector(MyPlayLayer::checkProgress));

        return true;
    }

    void rebuildLayout() {
        auto fields = m_fields.self();
        float maxY = MSG_AREA_HEIGHT - CHAT_PADDING;
        float y = CHAT_PADDING;

        for (int i = 0; i < (int)fields->m_messageRows.size(); i++) {
            float rowTop = y + fields->m_rowHeights[i] * MSG_LINE_HEIGHT;
            if (rowTop > maxY) {
                fields->m_messageRows[i]->setVisible(false);
            } else {
                fields->m_messageRows[i]->setVisible(true);
                fields->m_messageRows[i]->setPositionY(y);
            }
            y += fields->m_rowHeights[i] * MSG_LINE_HEIGHT;
        }
    }

    void trimOverflow() {
        auto fields = m_fields.self();
        float maxY = MSG_AREA_HEIGHT - CHAT_PADDING * 2;
        float totalHeight = 0.0f;
        for (int h : fields->m_rowHeights) totalHeight += h * MSG_LINE_HEIGHT;

        // lwk fried fix but eh it works lol
        while (totalHeight > maxY && !fields->m_messageRows.empty()) {
            totalHeight -= fields->m_rowHeights.back() * MSG_LINE_HEIGHT;
            fields->m_msgContainer->removeChild(fields->m_messageRows.front(), true);
            fields->m_messageRows.erase(fields->m_messageRows.begin());
            fields->m_rowHeights.erase(fields->m_rowHeights.begin());
        }
    }

    void addChatMessage(const std::string& raw) {
        auto fields = m_fields.self();
        ChatMessage msg = parseChatMessage(raw);

        auto row = CCNode::create();
        row->setPosition({CHAT_PADDING, CHAT_PADDING});

        float cursorX = 0.0f;

        if (!msg.username.empty()) {
            std::string nameStr = msg.username + ": ";
            auto nameLabel = CCLabelBMFont::create(nameStr.c_str(), fields->m_font.c_str());
            nameLabel->setScale(TEXT_SCALE);
            nameLabel->setColor(colorForUsername(msg.username));
            nameLabel->setAnchorPoint({0.0f, 0.0f});
            nameLabel->setPosition({0.0f, 0.0f});
            row->addChild(nameLabel);
            cursorX = nameLabel->getContentSize().width * TEXT_SCALE;
        }

        std::string wrapped = wrapText(msg.text, 25, msg.username.size() + 2);
        auto textLabel = CCLabelBMFont::create(wrapped.c_str(), fields->m_font.c_str());
        textLabel->setScale(TEXT_SCALE);
        textLabel->setColor({210, 210, 210});
        textLabel->setAnchorPoint({0.0f, 0.0f});
        textLabel->setPosition({cursorX, 0.0f});
        row->addChild(textLabel);

        int lineCount = 1;
        for (char c : wrapped) if (c == '\n') lineCount++;

        fields->m_msgContainer->addChild(row);
        //fields->m_messageRows.insert(fields->m_messageRows.begin(), row);
        // fields->m_rowHeights.insert(fields->m_rowHeights.begin(), lineCount);
        fields->m_messageRows.push_back(row);
        fields->m_rowHeights.push_back(lineCount);

        trimOverflow();
        rebuildLayout();
    }

    void checkProgress(float dt) {
        auto fields = m_fields.self();
        float progress = this->getCurrentPercent();
        bool inPractice = this->m_isPracticeMode && !Mod::get()->getSettingValue<bool>("enabled-in-practice");
        bool visible = !inPractice && !fields->enabled;
        fields->m_chatRoot->setVisible(visible);

        if (progress != 0.0f && !fields->m_isDeathSpamming) {
            fields->m_lastAttPercent = progress;
        }
        

        if (!visible) return;

        // idle chat so box feels alive even during normal play
        fields->m_idleChatTimer += dt;
        if (fields->m_idleChatTimer >= fields->m_nextIdleDelay && !fields->m_isDeathSpamming) {
            addChatMessage(chat(IDLE_MESSAGES[rand() % IDLE_MESSAGES.size()]));
            fields->m_idleChatTimer = 0.0f;
            fields->m_nextIdleDelay = 1.4f + (rand() % 25) / 10.0f;
        }

        // NOOOOOOOO
        if (fields->m_isDeathSpamming) {
            fields->m_deathChatTimer += dt;
            if (fields->m_deathChatTimer >= fields->m_deathSpamDuration) {
                fields->m_isDeathSpamming = false;
                fields->m_deathChatTimer = 0;
            } else if (fields->m_lastAttPercent <= (float)fields->m_lvl->getNormalPercent() && fields->m_lastAttPercent <= fields->deathPercent) {
                fields->m_randomChatTimer += dt;
                if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                    std::vector<std::string> deathMessages = {
                        chat("RIP"),
                        chat("NOOOO"),
                        chat("rippp"),
                        chat("F"),
                        chat("NOOOOOO"),
                        chat("rip bozo"),
                        chat("oof"),
                        chat("so close"),
                        chat("unlucky"),
                        chat("F in chat"),
                        chat("rip"),
                        chat("noooo way"),
                        chat("bro"),
                        chat("that was so close"),
                        chat("NOOOOOOOOOO"),
                        chat("next attempt"),
                        chat("you had it"),
                        chat("almost"),
                        chat("not like this"),
                        chat("gg attempt"),
                    };
                    addChatMessage(deathMessages[rand() % deathMessages.size()]);
                    fields->m_randomChatTimer = 0;
                }
            } else if (fields->m_lastAttPercent > (float)fields->m_lvl->getNormalPercent()) {
                    fields->m_randomChatTimer += dt;
                    if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                        std::vector<std::string> messages = {
                            chat("NEW BEST!!!!!"),
                            chat("W NEW BEST!"),
                            chat("W PROGRESS"),
                            chat("GG NEW BEST"),
                            chat("NEW BEST ALREADY?!?!"),
                        };
                        addChatMessage(messages[rand() % messages.size()]);
                        fields->m_randomChatTimer = 0;
                        float t = (progress - fields->goPercent) / (fields->superGoPercent - fields->goPercent);
                        fields->m_nextChatDelay = 0.133f - (t * 0.033f) / 100.0f * abs(fields->m_numViewers);
                    }
                }
            return;
        }

        if (fields->m_echoClipPresent && !fields->m_clipMessageFired) {
            bool newBestPastGo = progress >= fields->goPercent && progress > fields->m_bestPercent - 0.01f && fields->att > 5;
            bool superGoNewBest = progress >= fields->superGoPercent;
            if (newBestPastGo || superGoNewBest) {
                std::vector<std::string> clipMessages = {
                    chat("BRO CLIP THAT"),
                    chat("CLIP IT CLIP IT"),
                    chat("F6 F6 F6 F6"),
                    chat("CLIP THIS RIGHT NOW"),
                    chat("BRO HIT F6"),
                    chat("RECORDING???"),
                    chat("DONT FORGET TO CLIP"),
                    chat("AXIOM.ECHOCLIP MENTIONED"),
                };
                addChatMessage(clipMessages[rand() % clipMessages.size()]);
                fields->m_clipMessageFired = true;
            }
        }

        // hold
        if (progress >= fields->holdPercent && progress < fields->goPercent) {
            fields->m_randomChatTimer += dt;
            if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                std::vector<std::string> messages = {
                    chat("holdlldldldl"),
                    chat("HOLD IT"),
                    chat("HOOOOOOOOLDDDDDDDDDDDD"),
                    chat("HOOOLD"),
                    chat("HOLDDDDDDDDDDDDDDDDDDD"),
                    chat("hold hold hold"),
                    chat("dont let go"),
                    chat("GRIP"),
                    chat("stay on it"),
                    chat("holding..."),
                };
                addChatMessage(messages[rand() % messages.size()]);
                fields->m_randomChatTimer = 0;
                float t = (progress - fields->holdPercent) / (fields->goPercent - fields->holdPercent);
                fields->m_nextChatDelay = 0.2f - (t * 0.067f) / 100.0f * abs(fields->m_numViewers); // DONT ASK WHY IT JUST IS
            }
        }
        // gooo
        else if (progress >= fields->goPercent && progress < fields->superGoPercent) {
            fields->m_randomChatTimer += dt;
            if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                std::vector<std::string> messages = {
                    chat("GOOOO"),
                    chat("LETS GOOOOO"),
                    chat("CMON"),
                    chat("GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"),
                    chat("GO GO GO GO"),
                    chat("YESYESYES"),
                    chat("HES GOING"),
                    chat("ACTUALLY GOING"),
                    chat("DO NOT MISS"),
                    chat("CMON CMON CMON"),
                    chat("W W W W W"),
                    chat("LETSSSSS GOOOOO"),
                    chat("clean"),
                    chat("he is NOT dropping"),
                };
                addChatMessage(messages[rand() % messages.size()]);
                fields->m_randomChatTimer = 0;
                float t = (progress - fields->goPercent) / (fields->superGoPercent - fields->goPercent);
                fields->m_nextChatDelay = 0.133f - (t * 0.033f) / 100.0f * abs(fields->m_numViewers);
            }
        }
        // super go
        else if (progress >= fields->superGoPercent && progress < fields->ggPercent) {
            fields->m_randomChatTimer += dt;
            if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                std::vector<std::string> messages = {
                    chat("SUPER GOOOOOOOOOOOOOOOOOOOOOO"),
                    chat("SUPERGOOOOOOOOOOOOOOOOOOOOOOOO!!!!!!!!!!!!"),
                    chat("I WAS HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"),
                    chat("CMONNNN"),
                    chat("GOOOOO GOOOOOO GOOOOOOOO"),
                    chat("BRO IS ACTUALLY DOING IT"),
                    chat("NO WAY NO WAY NO WAY"),
                    chat("CLEAN CLEAN CLEAN"),
                    chat("DONT MISS NOW"),
                    chat("SO CLOSE TO END"),
                    chat("END GAME BRO"),
                    chat("LITERALLY ENDING"),
                    chat("ALMOST THERE"),
                    chat("PLEASE"),
                    chat("OH MY GOD"),
                    chat("INSANE"),
                };
                addChatMessage(messages[rand() % messages.size()]);
                fields->m_randomChatTimer = 0;
                float t = (progress - fields->superGoPercent) / (99.9999f - fields->superGoPercent);
                fields->m_nextChatDelay = 0.1f - (t * 0.033f) / 100.0f * abs(fields->m_numViewers);
            }
        }
        // 100%: gg
        else if (progress > fields->ggPercent) {
            fields->m_randomChatTimer += dt;
            if (fields->m_randomChatTimer >= fields->m_nextChatDelay) {
                std::vector<std::string> messages = {
                    chat("GG"),
                    chat("GGS"),
                    chat("WWWWWWWWWWWWWWWWWWWWWWWW"),
                    chat("LETS GOOOOO"),
                    chat("WOOOOOOOOOOOOO"),
                    chat("HE DID IT"),
                    chat("NO WAY BRO"),
                    chat("ACTUAL W"),
                    chat("POGGERS"),
                    chat("GG EZ"),
                    chat("CARRIED"),
                    chat("W PLAYER"),
                    chat("LEGENDARY"),
                    chat("GOAT"),
                    chat("BRO IS BUILT DIFFERENT"),
                    chat("CLIP IT"),
                    chat("HALL OF FAME"),
                };
                addChatMessage(messages[rand() % messages.size()]);
                fields->m_randomChatTimer = 0;
                fields->m_nextChatDelay = 0.033f + (rand() % 18) / 1000.0f / 100.0f * abs(fields->m_numViewers);
            }
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        auto fields = m_fields.self();

        float progress = this->getCurrentPercent();
        std::string username = GameManager::sharedState()->m_playerName;

        if (username == "Axiom" && progress >= fields->goPercent && !fields->m_isDeathSpamming) {
            std::vector<std::string> axiomRoasts = {
                "Axiom: skill issue imo",
                "Axiom: bro",
                "Axiom: nah that's crazy",
                randomPlayer() + ": skill issue axiom",
                randomPlayer() + ": axiom diff",
                randomPlayer() + ": bro really died there",
                randomPlayer() + ": AXIOM NOOOO",
                randomPlayer() + ": how did u die there axiom",
                randomPlayer() + ": certified axiom moment",
                randomPlayer() + ": this is why echochoke exists",
                randomPlayer() + ": axiom.echochoke is going to go crazy",
                randomPlayer() + ": dev couldnt beat his own level lmao",
            };
            addChatMessage(axiomRoasts[rand() % axiomRoasts.size()]);
        }

        // lwk fried fix but eh it works lol
        if (fields->att > 16 && !fields->m_isDeathSpamming) {
            fields->m_isDeathSpamming = true;
            float t = progress / 100.0f;
            fields->m_deathSpamDuration = 2.0f + (t * t * 12.0f);
            fields->m_nextChatDelay = 0.5f - (t * 0.49f) / 100 * abs(fields->m_numViewers);
        }
        fields->att += 1;
        fields->m_clipMessageFired = false;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto rightMenu = this->getChildByID("right-button-menu");
        if (!rightMenu) {
            log::error("right-button-menu not found");
            return;
        }

        auto myButtonSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
        auto myButton = CCMenuItemSpriteExtra::create(
            myButtonSprite,
            this,
            menu_selector(MyPauseLayer::onMyButton)
        );

        rightMenu->addChild(myButton);
        static_cast<CCMenu*>(rightMenu)->updateLayout();
    }

    void onMyButton(CCObject*) {
        ChatConfigPopup::create()->show();
    }
};

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        std::thread([]() {
            auto res = web::WebRequest().getSync("https://badges.hiimjasmine00.com/developer");
            if (!res.ok()) {
                log::error("Failed to fetch dev list: {}", res.code());
                return;
            }
            auto arr = res.json().unwrapOr(matjson::Value{})
                .asArray().unwrapOr(std::vector<matjson::Value>{});
            for (auto& entry : arr) {
                GD_PLAYERS.push_back(entry["name"].asString().unwrapOrDefault());
            }
            std::sort(GD_PLAYERS.begin(), GD_PLAYERS.end());

            auto raw = Mod::get()->getSettingValue<std::string>("custom-chatters");
            std::stringstream ss(raw);
            std::string name;
            while (std::getline(ss, name, ' ')) {
                if (!name.empty()) GD_PLAYERS.push_back(name);
            }

            GD_PLAYERS.erase(std::unique(GD_PLAYERS.begin(), GD_PLAYERS.end()), GD_PLAYERS.end());
        }).detach();
        // hasSpoken.assign(GD_PLAYERS.size(), false);
        return true;
    }
};

void reloadPlayLayerThresholds() {
    if (auto pl = PlayLayer::get())
        static_cast<MyPlayLayer*>(pl)->reloadThresholds();
}